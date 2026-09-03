#include "Observability.h"
#include "ErrorReporter.h"

#include <cstdlib>
#include <cctype>
#include <drogon/drogon.h>
#include <iomanip>
#include <random>
#include <sstream>

namespace observability {
namespace {
constexpr const char* kRequestId = "observability.request_id";
constexpr const char* kTraceparent = "observability.traceparent";

std::string generateRequestId() {
  static thread_local std::mt19937_64 generator(std::random_device{}());
  std::ostringstream stream;
  stream << std::hex << std::setw(16) << std::setfill('0') << generator();
  return stream.str();
}

std::string randomHex(std::size_t length) {
  static thread_local std::mt19937_64 generator(std::random_device{}());
  std::ostringstream stream;
  while (stream.tellp() < static_cast<std::streamoff>(length)) {
    stream << std::hex << std::setw(16) << std::setfill('0') << generator();
  }
  return stream.str().substr(0, length);
}

bool validHex(const std::string& value) {
  for (const auto character : value) {
    if (!std::isxdigit(static_cast<unsigned char>(character))) return false;
  }
  return true;
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

std::string traceparent(const drogon::HttpRequestPtr& request) {
  if (request->attributes()->find(kTraceparent)) {
    return request->attributes()->get<std::string>(kTraceparent);
  }

  const auto supplied = request->getHeader("traceparent");
  std::string context;
  if (supplied.size() == 55 && supplied[2] == '-' && supplied[35] == '-' &&
      supplied[52] == '-' && validHex(supplied.substr(0, 2)) &&
      validHex(supplied.substr(3, 32)) && validHex(supplied.substr(36, 16)) &&
      validHex(supplied.substr(53, 2))) {
    context = supplied;
  } else {
    context = "00-" + randomHex(32) + "-" + randomHex(16) + "-01";
  }
  request->attributes()->insert(kTraceparent, context);
  return context;
}

void Metrics::recordRequest(const drogon::HttpRequestPtr& request) {
  ++requests_;
  requestId(request);
  traceparent(request);
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
               const std::string& configuredDsn,
               const std::string& configuredOtlpEndpoint,
               const double timeoutSeconds,
               const int batchSize,
               const double batchDelaySeconds) {
  const char* environmentDsn = std::getenv("SENTRY_DSN");
  const auto& sentryDsn = configuredDsn.empty() && environmentDsn != nullptr
                              ? std::string(environmentDsn)
                              : configuredDsn;
  const char* environmentProvider = std::getenv("ERROR_TRACKING_PROVIDER");
  const auto& provider = configuredProvider.empty() && environmentProvider != nullptr
                             ? std::string(environmentProvider)
                             : configuredProvider;
  ErrorContext settings;
  if (!configuredOtlpEndpoint.empty()) {
    settings["otlp_endpoint"] = configuredOtlpEndpoint;
  }
  settings["timeout_seconds"] = std::to_string(timeoutSeconds);
  settings["batch_size"] = std::to_string(batchSize);
  settings["batch_delay_seconds"] = std::to_string(batchDelaySeconds);
  configureErrorReporter(provider, sentryDsn, settings);
}

} // namespace observability
