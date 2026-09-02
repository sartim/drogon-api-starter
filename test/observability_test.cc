#include "observability/Observability.h"
#include "observability/ErrorReporter.h"

#include <drogon/HttpRequest.h>

#include <iostream>
#include <stdexcept>

int main() {
  auto request = drogon::HttpRequest::newHttpRequest();
  request->addHeader("X-Request-ID", "test-request-id");

  if (observability::requestId(request) != "test-request-id" ||
      observability::requestId(request) != "test-request-id") {
    std::cerr << "request ID propagation failed\n";
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

  return 0;
}
