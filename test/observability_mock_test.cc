#include "observability/ErrorReporter.h"
#include "observability/Observability.h"

#include <drogon/drogon.h>

#include <condition_variable>
#include <chrono>
#include <cstdint>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {
class ReceivedEvent {
public:
  int record(const std::string& body) {
    int requestCount;
    {
      std::lock_guard lock(mutex_);
      body_ = body;
      requestCount = ++requests_;
    }
    condition_.notify_one();
    return requestCount;
  }

  bool waitForRequests(const int expected) {
    std::unique_lock lock(mutex_);
    return condition_.wait_for(lock, std::chrono::seconds(2),
                               [this, expected] { return requests_ >= expected; });
  }

  std::string body() const {
    std::lock_guard lock(mutex_);
    return body_;
  }

  int requests() const {
    std::lock_guard lock(mutex_);
    return requests_;
  }

private:
  mutable std::mutex mutex_;
  std::condition_variable condition_;
  std::string body_;
  int requests_{0};
};

std::uint16_t availablePort() {
  const auto socketHandle = socket(AF_INET, SOCK_STREAM, 0);
  if (socketHandle < 0) {
    std::cerr << "socket: " << std::strerror(errno) << "\n";
    return 0;
  }
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  address.sin_port = htons(0);
  const auto bound = bind(socketHandle, reinterpret_cast<sockaddr*>(&address),
                          sizeof(address));
  socklen_t length = sizeof(address);
  const auto resolved = getsockname(socketHandle,
                                    reinterpret_cast<sockaddr*>(&address),
                                    &length);
  const auto port = bound == 0 && resolved == 0 ? ntohs(address.sin_port) : 0;
  if (port == 0) {
    std::cerr << "bind/getsockname: " << std::strerror(errno) << "\n";
  }
  close(socketHandle);
  return port;
}

std::uint64_t metricValue(const std::string& metrics,
                          const std::string& name) {
  const auto marker = name + " ";
  const auto position = metrics.find(marker);
  if (position == std::string::npos) return 0;
  return std::stoull(metrics.substr(position + marker.size()));
}
}  // namespace

int main() {
  const auto port = availablePort();
  if (port == 0) {
    std::cerr << "Skipping mock endpoint test: socket binding is unavailable\n";
    return 77;
  }
  ReceivedEvent otlpEvent;
  ReceivedEvent sentryEvent;
  ReceivedEvent flakyEvent;
  ReceivedEvent circuitEvent;

  drogon::app().registerHandler(
      "/mock/otlp",
      [&otlpEvent](const drogon::HttpRequestPtr& request,
                   drogon::AdviceCallback&& callback) {
        otlpEvent.record(std::string(request->getBody()));
        callback(drogon::HttpResponse::newHttpResponse());
      });
  drogon::app().registerHandler(
      "/mock/flaky",
      [&flakyEvent](const drogon::HttpRequestPtr& request,
                    drogon::AdviceCallback&& callback) {
        const auto requestCount = flakyEvent.record(std::string(request->getBody()));
        auto response = drogon::HttpResponse::newHttpResponse();
        if (requestCount < 3) response->setStatusCode(drogon::k500InternalServerError);
        callback(response);
      });
  drogon::app().registerHandler(
      "/mock/circuit",
      [&circuitEvent](const drogon::HttpRequestPtr& request,
                      drogon::AdviceCallback&& callback) {
        circuitEvent.record(std::string(request->getBody()));
        auto response = drogon::HttpResponse::newHttpResponse();
        response->setStatusCode(drogon::k500InternalServerError);
        callback(response);
      });
  drogon::app().registerHandler(
      "/api/42/envelope/",
      [&sentryEvent](const drogon::HttpRequestPtr& request,
                     drogon::AdviceCallback&& callback) {
        sentryEvent.record(std::string(request->getBody()));
        callback(drogon::HttpResponse::newHttpResponse());
      });
  drogon::app().addListener("127.0.0.1", port);

  std::thread server([] { drogon::app().run(); });

  std::runtime_error error("mock observability error");
  const observability::ErrorContext batching{{"batch_size", "10"},
                                             {"batch_delay_seconds", "0.1"}};
  observability::configureErrorReporter(
      "otlp", "http://127.0.0.1:" + std::to_string(port) + "/mock/otlp",
      batching);
  observability::captureException(error, "otlp-request");
  observability::captureException(error, "otlp-request-2");
  if (!otlpEvent.waitForRequests(1) ||
      otlpEvent.body().find("mock observability error") == std::string::npos ||
      otlpEvent.body().find("otlp-request") == std::string::npos ||
      otlpEvent.body().find("otlp-request-2") == std::string::npos) {
    std::cerr << "OTLP mock payload validation failed\n";
    drogon::app().quit();
    server.join();
    return 1;
  }

  const observability::ErrorContext retrying{{"batch_size", "1"},
                                             {"retry_max_attempts", "3"},
                                             {"retry_base_delay_seconds", "0.01"},
                                             {"circuit_failure_threshold", "5"}};
  observability::configureErrorReporter(
      "otlp", "http://127.0.0.1:" + std::to_string(port) + "/mock/flaky",
      retrying);
  observability::captureException(error, "retry-request");
  if (!flakyEvent.waitForRequests(3)) {
    std::cerr << "OTLP retry policy did not retry failed delivery\n";
    drogon::app().quit();
    server.join();
    return 1;
  }

  const auto failuresBeforeCircuit = metricValue(
      observability::metrics().prometheus(), "observability_failures_total");
  const auto openBeforeCircuit = metricValue(
      observability::metrics().prometheus(), "observability_circuit_open_total");
  const observability::ErrorContext circuitSettings{
      {"batch_size", "1"},
      {"retry_max_attempts", "1"},
      {"circuit_failure_threshold", "1"},
      {"circuit_open_seconds", "0.2"}};
  observability::configureErrorReporter(
      "otlp", "http://127.0.0.1:" + std::to_string(port) + "/mock/circuit",
      circuitSettings);
  observability::captureException(error, "circuit-open-request");
  if (!circuitEvent.waitForRequests(1)) {
    std::cerr << "circuit breaker did not deliver the initial request\n";
    drogon::app().quit();
    server.join();
    return 1;
  }
  for (int attempt = 0; attempt < 20; ++attempt) {
    if (metricValue(observability::metrics().prometheus(),
                    "observability_failures_total") > failuresBeforeCircuit) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  observability::captureException(error, "circuit-rejected-request");
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  if (circuitEvent.requests() != 1 ||
      metricValue(observability::metrics().prometheus(),
                  "observability_circuit_open_total") <= openBeforeCircuit) {
    std::cerr << "open circuit did not reject a batch\n";
    drogon::app().quit();
    server.join();
    return 1;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(250));
  observability::captureException(error, "circuit-recovery-probe");
  if (!circuitEvent.waitForRequests(2)) {
    std::cerr << "circuit breaker did not permit a recovery probe\n";
    drogon::app().quit();
    server.join();
    return 1;
  }

  observability::configureErrorReporter(
      "sentry", "http://public@127.0.0.1:" + std::to_string(port) + "/42",
      batching);
  observability::captureException(error, "sentry-request");
  observability::captureException(error, "sentry-request-2");
  if (!sentryEvent.waitForRequests(1) ||
      sentryEvent.body().find("mock observability error") == std::string::npos ||
      sentryEvent.body().find("sentry-request") == std::string::npos ||
      sentryEvent.body().find("sentry-request-2") == std::string::npos) {
    std::cerr << "Sentry mock payload validation failed\n";
    drogon::app().quit();
    server.join();
    return 1;
  }

  drogon::app().quit();
  server.join();
  return 0;
}
