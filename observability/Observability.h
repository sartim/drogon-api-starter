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
  std::string prometheus() const;

private:
  std::atomic<std::uint64_t> requests_{0};
  std::atomic<std::uint64_t> responses_{0};
  std::atomic<std::uint64_t> errors_{0};
};

std::string requestId(const drogon::HttpRequestPtr& request);
std::string traceparent(const drogon::HttpRequestPtr& request);
void configure(const std::string& provider = {},
               const std::string& sentryDsn = {});
Metrics& metrics();

} // namespace observability
