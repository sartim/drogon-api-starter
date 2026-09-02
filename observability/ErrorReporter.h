#pragma once

#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <string>

namespace observability {
using ErrorContext = std::map<std::string, std::string>;

struct ErrorReporterConfig {
  std::string provider;
  std::string dsn;
  ErrorContext settings;
};

class ErrorReporter {
public:
  virtual ~ErrorReporter() = default;
  virtual void captureException(const std::exception& error,
                                const std::string& requestId,
                                const ErrorContext& context) noexcept = 0;
  virtual std::string provider() const = 0;
};

class NoopErrorReporter final : public ErrorReporter {
public:
  void captureException(const std::exception& error,
                        const std::string& requestId,
                        const ErrorContext& context) noexcept override;
  std::string provider() const override;
};

using ErrorReporterFactory =
    std::function<std::unique_ptr<ErrorReporter>(const ErrorReporterConfig&)>;

// Optional integrations register adapters without coupling the platform to a
// vendor SDK. Unknown providers continue to use the no-op reporter.
void registerErrorReporterProvider(const std::string& provider,
                                   ErrorReporterFactory factory);

ErrorReporter& errorReporter();
void configureErrorReporter(const std::string& provider,
                            const std::string& sentryDsn = {});
void captureException(const std::exception& error,
                      const std::string& requestId = {},
                      const ErrorContext& context = {}) noexcept;
}  // namespace observability
