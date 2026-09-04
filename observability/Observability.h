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
  std::string prometheus() const;

private:
  std::atomic<std::uint64_t> requests_{0};
  std::atomic<std::uint64_t> responses_{0};
  std::atomic<std::uint64_t> errors_{0};
  std::atomic<std::uint64_t> observabilityQueued_{0};
  std::atomic<std::uint64_t> observabilityDropped_{0};
  std::atomic<std::uint64_t> observabilityBatches_{0};
  std::atomic<std::uint64_t> observabilityBatchEvents_{0};
};

std::string requestId(const drogon::HttpRequestPtr& request);
std::string traceparent(const drogon::HttpRequestPtr& request);
void configure(const std::string& provider = {},
               const std::string& sentryDsn = {},
               const std::string& otlpEndpoint = {},
               double timeoutSeconds = 1.0,
               int batchSize = 10,
               double batchDelaySeconds = 0.1,
               int maxQueueSize = 1024);
Metrics& metrics();
void flushErrorReporter() noexcept;

} // namespace observability
