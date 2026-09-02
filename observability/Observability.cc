#include "Observability.h"
#include "ErrorReporter.h"

#include <cstdlib>
#include <drogon/drogon.h>
#include <iomanip>
#include <random>
#include <sstream>

namespace observability {
namespace {
constexpr const char* kRequestId = "observability.request_id";

std::string generateRequestId() {
  static thread_local std::mt19937_64 generator(std::random_device{}());
  std::ostringstream stream;
  stream << std::hex << std::setw(16) << std::setfill('0') << generator();
  return stream.str();
}
} // namespace

Metrics& metrics() {
  static Metrics instance;
  return instance;
}

std::string requestId(const drogon::HttpRequestPtr& request) {
  if (request->attributes()->find(kRequestId)) {
    return request->attributes()->get<std::string>(kRequestId);
  }

  const auto supplied = request->getHeader("X-Request-ID");
  const auto id = supplied.empty() ? generateRequestId() : supplied;
  request->attributes()->insert(kRequestId, id);
  return id;
}

void Metrics::recordRequest(const drogon::HttpRequestPtr& request) {
  ++requests_;
  requestId(request);
}

void Metrics::recordResponse(const drogon::HttpRequestPtr& request,
                             const drogon::HttpResponsePtr& response) {
  (void)request;
  ++responses_;
  if (response && response->statusCode() >= drogon::k500InternalServerError) {
    ++errors_;
  }
}

std::string Metrics::prometheus() const {
  std::ostringstream output;
  output << "# HELP http_requests_total Total HTTP requests received.\n"
         << "# TYPE http_requests_total counter\n"
         << "http_requests_total " << requests_.load() << "\n"
         << "# HELP http_responses_total Total HTTP responses sent.\n"
         << "# TYPE http_responses_total counter\n"
         << "http_responses_total " << responses_.load() << "\n"
         << "# HELP http_errors_total Total HTTP 5xx responses.\n"
         << "# TYPE http_errors_total counter\n"
         << "http_errors_total " << errors_.load() << "\n";
  return output.str();
}

void configure(const std::string& configuredProvider,
               const std::string& configuredDsn) {
  const char* environmentDsn = std::getenv("SENTRY_DSN");
  const auto& sentryDsn = configuredDsn.empty() && environmentDsn != nullptr
                              ? std::string(environmentDsn)
                              : configuredDsn;
  const char* environmentProvider = std::getenv("ERROR_TRACKING_PROVIDER");
  const auto& provider = configuredProvider.empty() && environmentProvider != nullptr
                             ? std::string(environmentProvider)
                             : configuredProvider;
  configureErrorReporter(provider, sentryDsn);
}

} // namespace observability
