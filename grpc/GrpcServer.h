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
  struct StartOptions {
    std::string tlsCertFile;
    std::string tlsKeyFile;
    bool allowInsecure{false};
    int maxReceiveMessageBytes{4 * 1024 * 1024};
    int maxSendMessageBytes{4 * 1024 * 1024};
  };

  explicit GrpcServer(std::string secretKey = {});
  ~GrpcServer();

  GrpcServer(const GrpcServer&) = delete;
  GrpcServer& operator=(const GrpcServer&) = delete;

  bool start(const std::string& address, const StartOptions& options);
  void stop();

 private:
  class HealthService;
#ifdef ENABLE_USER_SERVICE
  class UserDirectoryService;
#endif
  std::unique_ptr<HealthService> healthService_;
#ifdef ENABLE_USER_SERVICE
  std::unique_ptr<UserDirectoryService> userDirectoryService_;
#endif
  std::unique_ptr<grpc::Server> server_;
  std::thread waitThread_;
};

}  // namespace grpc_adapter
