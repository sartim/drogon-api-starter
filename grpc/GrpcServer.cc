#include "grpc/GrpcServer.h"

#include "health.grpc.pb.h"
#include "observability/ErrorReporter.h"
#include "observability/Observability.h"
#ifdef ENABLE_USER_SERVICE
#include "user.grpc.pb.h"

#include <drogon/HttpAppFramework.h>

#include "helpers/AuthToken.h"
#include "services/UserService.h"
#endif

#include <grpcpp/grpcpp.h>
#include <trantor/utils/Logger.h>
#include <exception>
#include <fstream>
#include <iterator>
#include <string_view>
#include <utility>

namespace grpc_adapter {
namespace {

std::string clientMetadata(const grpc::ServerContext& context,
                           const std::string_view name) {
  for (const auto& [key, value] : context.client_metadata()) {
    if (std::string_view(key.data(), key.length()) == name) {
      return {value.data(), value.length()};
    }
  }
  return {};
}

class RpcObservation {
 public:
  RpcObservation(grpc::ServerContext* context, std::string method)
      : context_(context), method_(std::move(method)),
        requestId_(observability::normalizeRequestId(
            clientMetadata(*context_, "x-request-id"))),
        traceparent_(observability::normalizeTraceparent(
            clientMetadata(*context_, "traceparent"))) {
    context_->AddTrailingMetadata("x-request-id", requestId_);
    context_->AddTrailingMetadata("traceparent", traceparent_);
    LOG_INFO << "grpc_request_started request_id=" << requestId_
             << " traceparent=" << traceparent_ << " method=" << method_;
  }

  const std::string& requestId() const { return requestId_; }
  const std::string& traceparent() const { return traceparent_; }

  grpc::Status finish(grpc::Status status) const {
    observability::metrics().recordGrpcResponse(status.ok());
    LOG_INFO << "grpc_request_completed request_id=" << requestId_
             << " traceparent=" << traceparent_ << " method=" << method_
             << " status=" << status.error_code();
    return status;
  }

 private:
  grpc::ServerContext* context_;
  std::string method_;
  std::string requestId_;
  std::string traceparent_;
};

}  // namespace

class GrpcServer::HealthService final : public drogon::api::v1::Health::Service {
 public:
  grpc::Status Check(grpc::ServerContext* context,
                     const drogon::api::v1::HealthCheckRequest*,
                     drogon::api::v1::HealthCheckResponse* response) override {
    RpcObservation observation(context, "drogon.api.v1.Health/Check");
    response->set_status(drogon::api::v1::HealthCheckResponse::SERVING);
    return observation.finish(grpc::Status::OK);
  }
};

#ifdef ENABLE_USER_SERVICE
class GrpcServer::UserDirectoryService final
    : public drogon::api::v1::UserDirectory::Service {
 public:
  explicit UserDirectoryService(std::string secretKey)
      : secretKey_(std::move(secretKey)) {}

  grpc::Status GetUser(grpc::ServerContext* context,
                       const drogon::api::v1::GetUserRequest* request,
                       drogon::api::v1::User* response) override {
    RpcObservation observation(context, "drogon.api.v1.UserDirectory/GetUser");
    if (context->IsCancelled()) {
      return observation.finish(
          {grpc::StatusCode::CANCELLED, "Request was cancelled"});
    }

    const auto authorization = context->client_metadata().find("authorization");
    if (authorization == context->client_metadata().end() ||
        !verifyJWT(secretKey_, std::string(authorization->second.data(),
                                          authorization->second.length()))) {
      return observation.finish({grpc::StatusCode::UNAUTHENTICATED,
                                 "A valid bearer token is required"});
    }

    try {
      const auto client = drogon::app().getDbClient();
      if (!client) {
        return observation.finish({grpc::StatusCode::UNAVAILABLE,
                                   "The user service is not available"});
      }

      services::UserService userService(client);
      const auto user = userService.findById(request->user_id());
      if (!user) {
        return observation.finish(
            {grpc::StatusCode::NOT_FOUND, "User was not found"});
      }

      const auto publicUser = services::UserService::toPublicJson(*user);
      response->set_id(publicUser["id"].asString());
      response->set_first_name(publicUser["first_name"].asString());
      response->set_last_name(publicUser["last_name"].asString());
      response->set_email(publicUser["email"].asString());
      response->set_created_at(publicUser["created_at"].asString());
      response->set_updated_at(publicUser["updated_at"].asString());
      return observation.finish(grpc::Status::OK);
    } catch (const std::exception& error) {
      observability::captureException(
          error, observation.requestId(),
          {{"rpc.method", "drogon.api.v1.UserDirectory/GetUser"},
           {"traceparent", observation.traceparent()}});
      return observation.finish({grpc::StatusCode::INTERNAL,
                                 "Unable to retrieve the requested user"});
    }
  }

 private:
  const std::string secretKey_;
};
#endif

GrpcServer::GrpcServer(std::string secretKey)
    : healthService_(std::make_unique<HealthService>()) {
#ifdef ENABLE_USER_SERVICE
  userDirectoryService_ =
      std::make_unique<UserDirectoryService>(std::move(secretKey));
#else
  (void)secretKey;
#endif
}

GrpcServer::~GrpcServer() { stop(); }

bool GrpcServer::start(const std::string& address,
                       const StartOptions& options) {
  if (server_) return false;

  std::shared_ptr<grpc::ServerCredentials> credentials;
  if (!options.tlsCertFile.empty() && !options.tlsKeyFile.empty()) {
    std::ifstream certFile(options.tlsCertFile, std::ios::binary);
    std::ifstream keyFile(options.tlsKeyFile, std::ios::binary);
    if (!certFile.is_open() || !keyFile.is_open()) return false;

    const std::string cert((std::istreambuf_iterator<char>(certFile)),
                           std::istreambuf_iterator<char>());
    const std::string key((std::istreambuf_iterator<char>(keyFile)),
                          std::istreambuf_iterator<char>());
    if (cert.empty() || key.empty()) return false;

    grpc::SslServerCredentialsOptions tlsOptions;
    tlsOptions.pem_key_cert_pairs.push_back({key, cert});
    credentials = grpc::SslServerCredentials(tlsOptions);
  } else if (options.allowInsecure) {
    credentials = grpc::InsecureServerCredentials();
  } else {
    return false;
  }

  grpc::ServerBuilder builder;
  int selectedPort = 0;
  builder.SetMaxReceiveMessageSize(options.maxReceiveMessageBytes);
  builder.SetMaxSendMessageSize(options.maxSendMessageBytes);
  builder.AddListeningPort(address, std::move(credentials), &selectedPort);
  builder.RegisterService(healthService_.get());
#ifdef ENABLE_USER_SERVICE
  if (userDirectoryService_) {
    builder.RegisterService(userDirectoryService_.get());
  }
#endif
  server_ = builder.BuildAndStart();
  if (!server_ || selectedPort == 0) {
    server_.reset();
    return false;
  }

  waitThread_ = std::thread([this] { server_->Wait(); });
  return true;
}

void GrpcServer::stop() {
  if (server_) {
    server_->Shutdown();
  }
  if (waitThread_.joinable()) {
    waitThread_.join();
  }
  server_.reset();
}

}  // namespace grpc_adapter
