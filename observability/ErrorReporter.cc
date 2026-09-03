#include "ErrorReporter.h"
#include "Observability.h"

#include <drogon/drogon.h>
#include <drogon/HttpClient.h>
#include <json/json.h>
#include <cstdlib>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace observability {
namespace {
std::unique_ptr<ErrorReporter>& configuredReporter() {
  static std::unique_ptr<ErrorReporter> reporter =
      std::make_unique<NoopErrorReporter>();
  return reporter;
}

std::unordered_map<std::string, ErrorReporterFactory>& providerFactories() {
  static std::unordered_map<std::string, ErrorReporterFactory> factories;
  return factories;
}

std::mutex& providerMutex() {
  static std::mutex mutex;
  return mutex;
}

class HttpErrorReporter : public ErrorReporter {
public:
  HttpErrorReporter(std::string provider, std::string endpoint,
                    ErrorContext settings)
      : provider_(std::move(provider)) {
    const auto schemeEnd = endpoint.find("://");
    const auto pathStart = schemeEnd == std::string::npos
                               ? std::string::npos
                               : endpoint.find('/', schemeEnd + 3);
    endpoint_ = pathStart == std::string::npos
                    ? endpoint
                    : endpoint.substr(0, pathStart);
    requestPath_ = pathStart == std::string::npos
                       ? "/"
                       : endpoint.substr(pathStart);
    if (endpoint_.empty()) {
      throw std::invalid_argument("observability endpoint is required");
    }
    if (const auto found = settings.find("timeout_seconds");
        found != settings.end()) {
      timeoutSeconds_ = std::stod(found->second);
    }
    client_ = drogon::HttpClient::newHttpClient(endpoint_);
  }

  void captureException(const std::exception& error,
                        const std::string& requestId,
                        const ErrorContext& context) noexcept override {
    try {
      auto request = drogon::HttpRequest::newHttpRequest();
      request->setMethod(drogon::Post);
      request->setPath(requestPath_);
      request->setContentTypeString(contentType());
      request->setBody(payload(error, requestId, context));
      const auto timeout = timeoutSeconds_;
      client_->sendRequest(
          request,
          [](drogon::ReqResult result,
             const drogon::HttpResponsePtr& response) {
            if (result != drogon::ReqResult::Ok || !response ||
                response->statusCode() >= 400) {
              LOG_DEBUG << "Observability event delivery failed; continuing";
            }
          },
          timeout);
    } catch (...) {
      // Reporting is best effort and must never affect application flow.
    }
  }

  std::string provider() const override { return provider_; }

protected:
  virtual std::string payload(const std::exception& error,
                              const std::string& requestId,
                              const ErrorContext& context) const = 0;
  virtual const char* contentType() const { return "application/json"; }

private:
  std::string provider_;
  std::string endpoint_;
  std::string requestPath_;
  drogon::HttpClientPtr client_;
  double timeoutSeconds_{1.0};
};

class OtlpErrorReporter final : public HttpErrorReporter {
public:
  explicit OtlpErrorReporter(const ErrorReporterConfig& config)
      : HttpErrorReporter("otlp", endpoint(config), config.settings) {}

private:
  static std::string endpoint(const ErrorReporterConfig& config) {
    if (!config.dsn.empty()) return config.dsn;
    if (const auto found = config.settings.find("otlp_endpoint");
        found != config.settings.end()) return found->second;
    if (const auto* value = std::getenv("OTLP_ENDPOINT")) return value;
    return {};
  }

  std::string payload(const std::exception& error,
                      const std::string& requestId,
                      const ErrorContext& context) const override {
    Json::Value body;
    Json::Value record;
    record["body"]["stringValue"] = error.what();
    record["severityText"] = "ERROR";
    record["attributes"] = Json::arrayValue;
    auto addAttribute = [&record](const std::string& key,
                                  const std::string& value) {
      Json::Value attribute;
      attribute["key"] = key;
      attribute["value"]["stringValue"] = value;
      record["attributes"].append(attribute);
    };
    if (!requestId.empty()) addAttribute("request.id", requestId);
    for (const auto& [key, value] : context) addAttribute(key, value);
    body["resourceLogs"][0]["scopeLogs"][0]["logRecords"].append(record);
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, body);
  }
};

class SentryErrorReporter final : public HttpErrorReporter {
public:
  explicit SentryErrorReporter(const ErrorReporterConfig& config)
      : HttpErrorReporter("sentry", endpoint(config), config.settings) {}

private:
  const char* contentType() const override {
    return "application/x-sentry-envelope";
  }

  static std::string endpoint(const ErrorReporterConfig& config) {
    const auto separator = config.dsn.find("@");
    const auto scheme = config.dsn.find("://");
    if (separator == std::string::npos || scheme == std::string::npos) {
      throw std::invalid_argument("SENTRY_DSN must be a URL");
    }
    const auto key = config.dsn.substr(scheme + 3, separator - scheme - 3);
    const auto hostStart = separator + 1;
    const auto pathStart = config.dsn.find('/', hostStart);
    if (pathStart == std::string::npos || pathStart + 1 >= config.dsn.size()) {
      throw std::invalid_argument("SENTRY_DSN must include a project id");
    }
    const auto host = config.dsn.substr(separator + 1,
                                        pathStart - separator - 1);
    const auto project = config.dsn.substr(pathStart + 1);
    return config.dsn.substr(0, scheme + 3) + host +
           "/api/" + project + "/envelope/?sentry_version=7&sentry_key=" + key;
  }

  std::string payload(const std::exception& error,
                      const std::string& requestId,
                      const ErrorContext& context) const override {
    Json::Value header;
    header["type"] = "event";
    Json::Value event;
    event["exception"]["values"][0]["type"] = "std::exception";
    event["exception"]["values"][0]["value"] = error.what();
    if (!requestId.empty()) event["tags"]["request_id"] = requestId;
    for (const auto& [key, value] : context) event["extra"][key] = value;
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    Json::Value itemHeader;
    itemHeader["type"] = "event";
    return Json::writeString(builder, header) + "\n" +
           Json::writeString(builder, itemHeader) + "\n" +
           Json::writeString(builder, event);
  }
};

void registerBuiltInProviders() {
  static std::once_flag once;
  std::call_once(once, [] {
    registerErrorReporterProvider(
        "otlp", [](const ErrorReporterConfig& config) {
          auto endpoint = config.dsn;
          if (endpoint.empty()) {
            const auto configured = config.settings.find("otlp_endpoint");
            if (configured != config.settings.end()) endpoint = configured->second;
          }
          if (endpoint.empty()) {
            if (const auto* value = std::getenv("OTLP_ENDPOINT")) endpoint = value;
          }
          if (endpoint.empty()) return std::unique_ptr<ErrorReporter>{};
          return std::unique_ptr<ErrorReporter>(
              std::make_unique<OtlpErrorReporter>(config));
        });
    registerErrorReporterProvider(
        "sentry", [](const ErrorReporterConfig& config) {
          if (config.dsn.empty()) return std::unique_ptr<ErrorReporter>{};
          return std::unique_ptr<ErrorReporter>(
              std::make_unique<SentryErrorReporter>(config));
        });
  });
}
}  // namespace

void NoopErrorReporter::captureException(const std::exception& error,
                                         const std::string& requestId,
                                         const ErrorContext& context) noexcept {
  (void)error;
  (void)requestId;
  (void)context;
}

std::string NoopErrorReporter::provider() const { return "none"; }

void registerErrorReporterProvider(const std::string& provider,
                                   ErrorReporterFactory factory) {
  if (provider.empty() || !factory) {
    throw std::invalid_argument(
        "error reporter provider and factory are required");
  }
  std::lock_guard lock(providerMutex());
  providerFactories()[provider] = std::move(factory);
}

ErrorReporter& errorReporter() { return *configuredReporter(); }

void configureErrorReporter(const std::string& requestedProvider,
                            const std::string& sentryDsn,
                            const ErrorContext& settings) {
  registerBuiltInProviders();
  const auto provider = requestedProvider.empty() ? "none" : requestedProvider;
  configuredReporter() = std::make_unique<NoopErrorReporter>();
  if (provider == "none" || provider == "noop") {
    LOG_INFO << "Error reporting is disabled";
    return;
  }

  ErrorReporterConfig configuration;
  configuration.provider = provider;
  configuration.dsn = sentryDsn;
  configuration.settings = settings;
  if (const auto* timeout = std::getenv("OBSERVABILITY_TIMEOUT_SECONDS")) {
    configuration.settings["timeout_seconds"] = timeout;
  }
  ErrorReporterFactory factory;
  {
    std::lock_guard lock(providerMutex());
    const auto found = providerFactories().find(provider);
    if (found != providerFactories().end()) {
      factory = found->second;
    }
  }
  if (factory) {
    try {
      auto reporter = factory(configuration);
      if (reporter) {
        configuredReporter() = std::move(reporter);
        LOG_INFO << "Error reporting provider '" << provider << "' enabled";
        return;
      }
    } catch (const std::exception& error) {
      LOG_WARN << "Error reporting provider '" << provider
               << "' failed during initialization: " << error.what();
    } catch (...) {
      LOG_WARN << "Error reporting provider '" << provider
               << "' failed during initialization";
    }
  }

  LOG_WARN << "Error reporting provider '" << provider
           << "' is not enabled in this build; using no-op reporter"
           << (sentryDsn.empty() ? "" : " (DSN configured)");
}

void captureException(const std::exception& error, const std::string& requestId,
                      const ErrorContext& context) noexcept {
  try {
    errorReporter().captureException(error, requestId, context);
  } catch (...) {
    // Observability must never change application control flow.
  }
}

void captureException(const std::exception& error,
                      const drogon::HttpRequestPtr& request,
                      const ErrorContext& context) noexcept {
  try {
    if (!request) {
      captureException(error, std::string{}, context);
      return;
    }
    ErrorContext requestContext{{"http.method", request->methodString()},
                                {"http.path", request->path()},
                                {"traceparent", traceparent(request)}};
    requestContext.emplace("request.id", requestId(request));
    for (const auto& [key, value] : context) requestContext[key] = value;
    captureException(error, requestId(request), requestContext);
  } catch (...) {
    // Context enrichment is best effort and must never affect application flow.
  }
}
}  // namespace observability
