#pragma once

#include <exception>
#include <map>
#include <memory>
#include <string>

namespace observability {
using ErrorContext = std::map<std::string, std::string>;

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

ErrorReporter& errorReporter();
void configureErrorReporter(const std::string& provider,
                            const std::string& sentryDsn = {});
void captureException(const std::exception& error,
                      const std::string& requestId = {},
                      const ErrorContext& context = {}) noexcept;
}  // namespace observability
