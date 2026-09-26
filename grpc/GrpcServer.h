#pragma once

#include <memory>
#include <string>
#include <thread>

namespace grpc {
class Server;
}

namespace grpc_adapter {

class GrpcServer {
 public:
  GrpcServer();
  ~GrpcServer();

  GrpcServer(const GrpcServer&) = delete;
  GrpcServer& operator=(const GrpcServer&) = delete;

  bool start(const std::string& address);
  void stop();

 private:
  class HealthService;
  std::unique_ptr<HealthService> healthService_;
  std::unique_ptr<grpc::Server> server_;
  std::thread waitThread_;
};

}  // namespace grpc_adapter
