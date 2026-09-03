#include "observability/Observability.h"
#include "observability/ErrorReporter.h"
#include "observability/RateLimiter.h"

#include <drogon/HttpRequest.h>

#include <iostream>
#include <stdexcept>
#include <chrono>

namespace {
class RecordingReporter final : public observability::ErrorReporter {
public:
  void captureException(
      const std::exception& error, const std::string& requestId,
      const observability::ErrorContext& context) noexcept override {
    (void)error;
    lastRequestId = requestId;
    lastContext = context;
    ++captures;
  }

  std::string provider() const override { return "test"; }

  int captures{0};
  std::string lastRequestId;
  observability::ErrorContext lastContext;
};
}  // namespace

int main() {
  auto request = drogon::HttpRequest::newHttpRequest();
  request->addHeader("X-Request-ID", "test-request-id");

  if (observability::requestId(request) != "test-request-id" ||
      observability::requestId(request) != "test-request-id") {
    std::cerr << "request ID propagation failed\n";
    return 1;
  }

  request->addHeader("traceparent",
                     "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01");
  if (observability::traceparent(request) !=
      "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01") {
    std::cerr << "trace context propagation failed\n";
    return 1;
  }

  observability::metrics().recordRequest(request);
  const auto metrics = observability::metrics().prometheus();
  if (metrics.find("http_requests_total") == std::string::npos) {
    std::cerr << "request metric is missing\n";
    return 1;
  }

  observability::configureErrorReporter("none");
  if (observability::errorReporter().provider() != "none") {
    std::cerr << "default error reporter is not the no-op provider\n";
    return 1;
  }
  try {
    throw std::runtime_error("test error");
  } catch (const std::exception& error) {
    observability::captureException(error, "test-request-id");
  }

  auto* recordingReporter = static_cast<RecordingReporter*>(nullptr);
  observability::registerErrorReporterProvider(
      "test", [&](const observability::ErrorReporterConfig& configuration)
          -> std::unique_ptr<observability::ErrorReporter> {
        if (configuration.provider != "test") {
          return std::unique_ptr<observability::ErrorReporter>{};
        }
        auto reporter = std::make_unique<RecordingReporter>();
        recordingReporter = reporter.get();
        return reporter;
      });
  observability::configureErrorReporter("test", "adapter-config");
  if (observability::errorReporter().provider() != "test" ||
      recordingReporter == nullptr) {
    std::cerr << "registered error reporter was not enabled\n";
    return 1;
  }
  try {
    throw std::runtime_error("adapter error");
  } catch (const std::exception& error) {
    observability::captureException(error, "adapter-request");
  }
  if (recordingReporter->captures != 1 ||
      recordingReporter->lastRequestId != "adapter-request") {
    std::cerr << "registered error reporter did not receive the event\n";
    return 1;
  }

  observability::configureErrorReporter("test");
  request->addHeader(
      "traceparent",
      "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01");
  try {
    throw std::runtime_error("request context error");
  } catch (const std::exception& error) {
    observability::captureException(error, request,
                                    {{"service.version", "test"}});
  }
  if (recordingReporter->captures != 1 ||
      recordingReporter->lastContext.at("request.id") != "test-request-id" ||
      recordingReporter->lastContext.at("traceparent") !=
          "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01" ||
      recordingReporter->lastContext.at("http.method") != "GET" ||
      recordingReporter->lastContext.at("service.version") != "test") {
    std::cerr << "request observability context was not propagated\n";
    return 1;
  }

  observability::configureErrorReporter("otlp", "http://127.0.0.1:4318/v1/logs");
  if (observability::errorReporter().provider() != "otlp") {
    std::cerr << "OTLP provider was not enabled\n";
    return 1;
  }
  observability::configureErrorReporter("sentry",
                                        "https://public@example.com/42");
  if (observability::errorReporter().provider() != "sentry") {
    std::cerr << "Sentry provider was not enabled\n";
    return 1;
  }
  observability::configureErrorReporter("sentry", "not-a-dsn");
  if (observability::errorReporter().provider() != "none") {
    std::cerr << "invalid Sentry DSN did not fail open\n";
    return 1;
  }

  observability::RateLimiter limiter(2, std::chrono::seconds(60));
  if (!limiter.allow("client") || !limiter.allow("client") ||
      limiter.allow("client") || !limiter.allow("other")) {
    std::cerr << "rate limiter did not enforce per-client limits\n";
    return 1;
  }

  return 0;
}
