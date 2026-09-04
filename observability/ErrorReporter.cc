#include "ErrorReporter.h"
#include "Observability.h"

#include <drogon/drogon.h>
#include <drogon/HttpClient.h>
#include <json/json.h>
#include <cstdlib>
#include <chrono>
#include <deque>
#include <iterator>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

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

struct CircuitState {
  explicit CircuitState(const std::size_t threshold,
                        const double cooldownSeconds)
      : failureThreshold(threshold), cooldown(cooldownSeconds) {}

  bool allow() {
    std::lock_guard lock(mutex);
    const auto now = std::chrono::steady_clock::now();
    if (openUntil > now) return false;
    if (openUntil != std::chrono::steady_clock::time_point{} && probeInFlight)
      return false;
    if (openUntil != std::chrono::steady_clock::time_point{}) probeInFlight = true;
    return true;
  }

  void success() {
    std::lock_guard lock(mutex);
    failures = 0;
    openUntil = {};
    probeInFlight = false;
  }

  void failure() {
    std::lock_guard lock(mutex);
    ++failures;
    if (failures >= failureThreshold) {
      openUntil = std::chrono::steady_clock::now() +
                  std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                      cooldown);
      probeInFlight = false;
    }
  }

  std::mutex mutex;
  std::size_t failures{0};
  std::size_t failureThreshold;
  std::chrono::steady_clock::time_point openUntil{};
  std::chrono::duration<double> cooldown;
  bool probeInFlight{false};
};

struct Delivery {
  drogon::HttpClientPtr client;
  std::string path;
  std::string contentType;
  std::string body;
  double timeout;
  std::size_t maxAttempts;
  double baseDelay;
  std::size_t attempt{0};
  std::shared_ptr<CircuitState> circuit;
};

void deliver(const std::shared_ptr<Delivery>& delivery) {
  try {
    ++delivery->attempt;
    auto request = drogon::HttpRequest::newHttpRequest();
    request->setMethod(drogon::Post);
    request->setPath(delivery->path);
    request->setContentTypeString(delivery->contentType);
    request->setBody(delivery->body);
    delivery->client->sendRequest(
        request,
        [delivery](drogon::ReqResult result,
                   const drogon::HttpResponsePtr& response) {
          const bool success = result == drogon::ReqResult::Ok && response &&
                               response->statusCode() < 400;
          if (success) {
            delivery->circuit->success();
            return;
          }
          if (delivery->attempt < delivery->maxAttempts) {
            metrics().recordObservabilityRetry();
            const auto exponent = delivery->attempt - 1;
            const auto delay = delivery->baseDelay * (1ULL << exponent);
            delivery->client->getLoop()->runAfter(
                delay, [delivery] { deliver(delivery); });
            return;
          }
          metrics().recordObservabilityFailure();
          delivery->circuit->failure();
        },
        delivery->timeout);
  } catch (...) {
    metrics().recordObservabilityFailure();
    delivery->circuit->failure();
  }
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
    if (const auto found = settings.find("batch_size");
        found != settings.end()) {
      batchSize_ = std::stoul(found->second);
    }
    if (const auto found = settings.find("batch_delay_seconds");
        found != settings.end()) {
      batchDelaySeconds_ = std::stod(found->second);
    }
    if (const auto found = settings.find("max_queue_size");
        found != settings.end()) {
      maxQueueSize_ = std::stoul(found->second);
    }
    if (const auto found = settings.find("retry_max_attempts");
        found != settings.end()) {
      maxAttempts_ = std::stoul(found->second);
    }
    if (const auto found = settings.find("retry_base_delay_seconds");
        found != settings.end()) {
      retryBaseDelaySeconds_ = std::stod(found->second);
    }
    if (const auto found = settings.find("circuit_failure_threshold");
        found != settings.end()) {
      circuitFailureThreshold_ = std::stoul(found->second);
    }
    if (const auto found = settings.find("circuit_open_seconds");
        found != settings.end()) {
      circuitOpenSeconds_ = std::stod(found->second);
    }
    if (batchSize_ == 0 || maxQueueSize_ == 0 || batchDelaySeconds_ <= 0.0 ||
        maxAttempts_ == 0 || maxAttempts_ > 10 || retryBaseDelaySeconds_ <= 0.0 ||
        circuitFailureThreshold_ == 0 || circuitOpenSeconds_ <= 0.0) {
      throw std::invalid_argument("invalid observability batch settings");
    }
    client_ = drogon::HttpClient::newHttpClient(endpoint_);
    circuit_ = std::make_shared<CircuitState>(circuitFailureThreshold_,
                                               circuitOpenSeconds_);
  }

  ~HttpErrorReporter() override {
    if (timerId_ != trantor::InvalidTimerId && client_ && client_->getLoop()) {
      client_->getLoop()->invalidateTimer(timerId_);
    }
  }

  void captureException(const std::exception& error,
                        const std::string& requestId,
                        const ErrorContext& context) noexcept override {
    try {
      bool flushImmediately = false;
      {
        std::lock_guard lock(queueMutex_);
        if (queue_.size() >= maxQueueSize_) {
          metrics().recordObservabilityDropped();
          return;
        }
        queue_.push_back({error.what(), requestId, context});
        metrics().recordObservabilityQueued();
        flushImmediately = queue_.size() >= batchSize_;
        if (!flushImmediately && timerId_ == trantor::InvalidTimerId) {
          timerId_ = client_->getLoop()->runAfter(
              batchDelaySeconds_, [this] { flushQueue(); });
        }
      }
      if (flushImmediately) flushQueue();
    } catch (...) {
      // Reporting is best effort and must never affect application flow.
    }
  }

  std::string provider() const override { return provider_; }

  void flush() noexcept override { flushQueue(); }

protected:
  struct CapturedException {
    std::string message;
    std::string requestId;
    ErrorContext context;
  };

  virtual std::string payload(
      const std::vector<CapturedException>& events) const = 0;
  virtual const char* contentType() const { return "application/json"; }

private:
  void flushQueue() noexcept {
    try {
      std::vector<CapturedException> events;
      {
        std::lock_guard lock(queueMutex_);
        timerId_ = trantor::InvalidTimerId;
        events.assign(std::make_move_iterator(queue_.begin()),
                      std::make_move_iterator(queue_.end()));
        queue_.clear();
      }
      if (events.empty()) return;
      metrics().recordObservabilityBatch(events.size());
      if (!circuit_->allow()) {
        metrics().recordObservabilityCircuitOpen();
        metrics().recordObservabilityDropped();
        return;
      }
      auto delivery = std::make_shared<Delivery>();
      delivery->client = client_;
      delivery->path = requestPath_;
      delivery->contentType = contentType();
      delivery->body = payload(events);
      delivery->timeout = timeoutSeconds_;
      delivery->maxAttempts = maxAttempts_;
      delivery->baseDelay = retryBaseDelaySeconds_;
      delivery->circuit = circuit_;
      deliver(delivery);
    } catch (...) {
      // Reporting is best effort and must never affect application flow.
    }
  }

  std::string provider_;
  std::string endpoint_;
  std::string requestPath_;
  drogon::HttpClientPtr client_;
  double timeoutSeconds_{1.0};
  std::size_t batchSize_{10};
  std::size_t maxQueueSize_{1024};
  double batchDelaySeconds_{0.1};
  std::size_t maxAttempts_{3};
  double retryBaseDelaySeconds_{0.1};
  std::size_t circuitFailureThreshold_{5};
  double circuitOpenSeconds_{30.0};
  std::shared_ptr<CircuitState> circuit_;
  std::mutex queueMutex_;
  std::deque<CapturedException> queue_;
  trantor::TimerId timerId_{trantor::InvalidTimerId};
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

  std::string payload(const std::vector<CapturedException>& events) const override {
    Json::Value body;
    for (const auto& event : events) {
      Json::Value record;
      record["body"]["stringValue"] = event.message;
      record["severityText"] = "ERROR";
      record["attributes"] = Json::arrayValue;
      auto addAttribute = [&record](const std::string& key,
                                    const std::string& value) {
        Json::Value attribute;
        attribute["key"] = key;
        attribute["value"]["stringValue"] = value;
        record["attributes"].append(attribute);
      };
      if (!event.requestId.empty()) addAttribute("request.id", event.requestId);
      for (const auto& [key, value] : event.context) addAttribute(key, value);
      body["resourceLogs"][0]["scopeLogs"][0]["logRecords"].append(record);
    }
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

  std::string payload(const std::vector<CapturedException>& events) const override {
    Json::Value header;
    header["type"] = "event";
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    std::string envelope = Json::writeString(builder, header);
    for (const auto& captured : events) {
      Json::Value event;
      event["exception"]["values"][0]["type"] = "std::exception";
      event["exception"]["values"][0]["value"] = captured.message;
      if (!captured.requestId.empty())
        event["tags"]["request_id"] = captured.requestId;
      for (const auto& [key, value] : captured.context)
        event["extra"][key] = value;
      Json::Value itemHeader;
      itemHeader["type"] = "event";
      envelope += "\n" + Json::writeString(builder, itemHeader) + "\n" +
                  Json::writeString(builder, event);
    }
    return envelope;
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
