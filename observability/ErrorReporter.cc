#include "ErrorReporter.h"

#include <drogon/drogon.h>
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
                            const std::string& sentryDsn) {
  const auto provider = requestedProvider.empty() ? "none" : requestedProvider;
  configuredReporter() = std::make_unique<NoopErrorReporter>();
  if (provider == "none" || provider == "noop") {
    LOG_INFO << "Error reporting is disabled";
    return;
  }

  ErrorReporterConfig configuration;
  configuration.provider = provider;
  configuration.dsn = sentryDsn;
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
}  // namespace observability
