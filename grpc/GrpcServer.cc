#include "grpc/GrpcServer.h"

#include "health.grpc.pb.h"
#ifdef ENABLE_USER_SERVICE
#include "user.grpc.pb.h"

#include <drogon/HttpAppFramework.h>

#include "helpers/AuthToken.h"
#include "services/UserService.h"
#endif

#include <grpcpp/grpcpp.h>
#include <exception>
#include <fstream>
#include <iterator>
#include <utility>

namespace grpc_adapter {

class GrpcServer::HealthService final : public drogon::api::v1::Health::Service {
 public:
  grpc::Status Check(grpc::ServerContext*,
                     const drogon::api::v1::HealthCheckRequest*,
                     drogon::api::v1::HealthCheckResponse* response) override {
    response->set_status(drogon::api::v1::HealthCheckResponse::SERVING);
    return grpc::Status::OK;
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
    if (context->IsCancelled()) {
      return {grpc::StatusCode::CANCELLED, "Request was cancelled"};
    }

    const auto authorization = context->client_metadata().find("authorization");
    if (authorization == context->client_metadata().end() ||
        !verifyJWT(secretKey_, std::string(authorization->second.data(),
                                          authorization->second.length()))) {
      return {grpc::StatusCode::UNAUTHENTICATED,
              "A valid bearer token is required"};
    }

    try {
      const auto client = drogon::app().getDbClient();
      if (!client) {
        return {grpc::StatusCode::UNAVAILABLE,
                "The user service is not available"};
      }

      services::UserService userService(client);
      const auto user = userService.findById(request->user_id());
      if (!user) {
        return {grpc::StatusCode::NOT_FOUND, "User was not found"};
      }

      const auto publicUser = services::UserService::toPublicJson(*user);
      response->set_id(publicUser["id"].asString());
      response->set_first_name(publicUser["first_name"].asString());
      response->set_last_name(publicUser["last_name"].asString());
      response->set_email(publicUser["email"].asString());
      response->set_created_at(publicUser["created_at"].asString());
      response->set_updated_at(publicUser["updated_at"].asString());
      return grpc::Status::OK;
    } catch (const std::exception&) {
      return {grpc::StatusCode::INTERNAL,
              "Unable to retrieve the requested user"};
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
