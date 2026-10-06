#include "user.grpc.pb.h"
#include "health.grpc.pb.h"

#include <grpcpp/grpcpp.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>

namespace {
bool requestUser(
    const std::unique_ptr<drogon::api::v1::UserDirectory::Stub>& stub,
    const std::string& userId, const std::string& token,
    const std::string& expectedEmail) {
  grpc::ClientContext context;
  context.set_deadline(std::chrono::system_clock::now() +
                       std::chrono::seconds(5));
  if (!token.empty()) context.AddMetadata("authorization", "Bearer " + token);

  drogon::api::v1::GetUserRequest request;
  request.set_user_id(userId);
  drogon::api::v1::User response;
  const auto status = stub->GetUser(&context, request, &response);
  if (token.empty()) {
    if (status.error_code() != grpc::StatusCode::UNAUTHENTICATED) {
      std::cerr << "Unauthenticated GetUser returned "
                << status.error_code() << ": " << status.error_message()
                << '\n';
      return false;
    }
    return true;
  }
  if (!status.ok()) {
    std::cerr << "Authenticated GetUser failed: " << status.error_code()
              << " " << status.error_message() << '\n';
    return false;
  }
  if (response.id() != userId || response.email() != expectedEmail ||
      response.first_name().empty() || response.last_name().empty() ||
      response.created_at().empty() || response.updated_at().empty()) {
    std::cerr << "GetUser returned an incomplete or unexpected public user\n";
    return false;
  }
  return true;
}

bool requestMissingUser(
    const std::unique_ptr<drogon::api::v1::UserDirectory::Stub>& stub,
    const std::string& token) {
  grpc::ClientContext context;
  context.set_deadline(std::chrono::system_clock::now() +
                       std::chrono::seconds(5));
  context.AddMetadata("authorization", "Bearer " + token);
  drogon::api::v1::GetUserRequest request;
  request.set_user_id("00000000-0000-0000-0000-000000000000");
  drogon::api::v1::User response;
  const auto status = stub->GetUser(&context, request, &response);
  if (status.error_code() != grpc::StatusCode::NOT_FOUND) {
    std::cerr << "Missing user lookup returned " << status.error_code()
              << ": " << status.error_message() << '\n';
    return false;
  }
  return true;
}

bool checkHealth(
    const std::shared_ptr<grpc::Channel>& channel) {
  auto stub = drogon::api::v1::Health::NewStub(channel);
  grpc::ClientContext context;
  context.set_deadline(std::chrono::system_clock::now() +
                       std::chrono::seconds(5));
  drogon::api::v1::HealthCheckRequest request;
  drogon::api::v1::HealthCheckResponse response;
  const auto status = stub->Check(&context, request, &response);
  if (!status.ok() ||
      response.status() !=
          drogon::api::v1::HealthCheckResponse::SERVING) {
    std::cerr << "TLS gRPC health check failed: " << status.error_code()
              << " " << status.error_message() << '\n';
    return false;
  }
  return true;
}
}  // namespace

int main(int argc, char* argv[]) {
  if (argc != 6) {
    std::cerr << "Usage: grpc_integration_client <address> <user-id> "
                 "<bearer-token> <expected-email> <root-cert-file>\n";
    return 2;
  }
  std::ifstream certFile(argv[5], std::ios::binary);
  if (!certFile.is_open()) {
    std::cerr << "Unable to open gRPC root certificate\n";
    return 2;
  }
  grpc::SslCredentialsOptions tlsOptions;
  tlsOptions.pem_root_certs.assign(std::istreambuf_iterator<char>(certFile),
                                   std::istreambuf_iterator<char>());
  auto channel = grpc::CreateChannel(argv[1], grpc::SslCredentials(tlsOptions));
  auto stub = drogon::api::v1::UserDirectory::NewStub(channel);
  if (!checkHealth(channel) ||
      !requestUser(stub, argv[2], argv[3], argv[4]) ||
      !requestUser(stub, argv[2], "", "") ||
      !requestMissingUser(stub, argv[3])) {
    return 1;
  }
  std::cout << "Authenticated GetUser and unauthenticated rejection passed\n";
  return 0;
}
