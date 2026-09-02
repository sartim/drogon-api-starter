#include "ErrorReporter.h"

#include <drogon/drogon.h>

namespace observability {
namespace {
std::unique_ptr<ErrorReporter>& configuredReporter() {
  static std::unique_ptr<ErrorReporter> reporter =
      std::make_unique<NoopErrorReporter>();
  return reporter;
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

ErrorReporter& errorReporter() { return *configuredReporter(); }

void configureErrorReporter(const std::string& requestedProvider,
                            const std::string& sentryDsn) {
  const auto provider = requestedProvider.empty() ? "none" : requestedProvider;
  configuredReporter() = std::make_unique<NoopErrorReporter>();
  if (provider == "none" || provider == "noop") {
    LOG_INFO << "Error reporting is disabled";
    return;
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
