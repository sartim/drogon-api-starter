#include "observability/Observability.h"

#include <drogon/HttpRequest.h>

#include <iostream>

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

  return 0;
}
