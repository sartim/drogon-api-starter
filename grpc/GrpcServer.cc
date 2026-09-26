#include "grpc/GrpcServer.h"

#include "health.grpc.pb.h"

#include <grpcpp/grpcpp.h>

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

GrpcServer::GrpcServer() : healthService_(std::make_unique<HealthService>()) {}

GrpcServer::~GrpcServer() { stop(); }

bool GrpcServer::start(const std::string& address) {
  if (server_) return false;

  grpc::ServerBuilder builder;
  int selectedPort = 0;
  builder.AddListeningPort(address, grpc::InsecureServerCredentials(),
                           &selectedPort);
  builder.RegisterService(healthService_.get());
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
