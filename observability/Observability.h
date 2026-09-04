#pragma once

#include <atomic>
#include <cstdint>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <string>

namespace observability {

class Metrics {
public:
  void recordRequest(const drogon::HttpRequestPtr& request);
  void recordResponse(const drogon::HttpRequestPtr& request,
                      const drogon::HttpResponsePtr& response);
  void recordObservabilityQueued();
  void recordObservabilityDropped();
  void recordObservabilityBatch(std::uint64_t eventCount);
  void recordObservabilityRetry();
  void recordObservabilityFailure();
  void recordObservabilityCircuitOpen();
  std::string prometheus() const;

private:
  std::atomic<std::uint64_t> requests_{0};
  std::atomic<std::uint64_t> responses_{0};
  std::atomic<std::uint64_t> errors_{0};
  std::atomic<std::uint64_t> observabilityQueued_{0};
  std::atomic<std::uint64_t> observabilityDropped_{0};
  std::atomic<std::uint64_t> observabilityBatches_{0};
  std::atomic<std::uint64_t> observabilityBatchEvents_{0};
  std::atomic<std::uint64_t> observabilityRetries_{0};
  std::atomic<std::uint64_t> observabilityFailures_{0};
  std::atomic<std::uint64_t> observabilityCircuitOpen_{0};
};

std::string requestId(const drogon::HttpRequestPtr& request);
std::string traceparent(const drogon::HttpRequestPtr& request);
void configure(const std::string& provider = {},
               const std::string& sentryDsn = {},
               const std::string& otlpEndpoint = {},
               double timeoutSeconds = 1.0,
               int batchSize = 10,
               double batchDelaySeconds = 0.1,
               int maxQueueSize = 1024,
               int retryMaxAttempts = 3,
               double retryBaseDelaySeconds = 0.1,
               int circuitFailureThreshold = 5,
               double circuitOpenSeconds = 30.0);
Metrics& metrics();
void flushErrorReporter() noexcept;

} // namespace observability
