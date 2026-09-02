#pragma once

#include <chrono>
#include <cstddef>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>

namespace observability {

class RateLimiter {
public:
  RateLimiter(std::size_t limit, std::chrono::seconds window);
  bool allow(const std::string& key);

private:
  const std::size_t limit_;
  const std::chrono::seconds window_;
  std::mutex mutex_;
  std::unordered_map<std::string,
                     std::deque<std::chrono::steady_clock::time_point>>
      requests_;
};

}  // namespace observability
