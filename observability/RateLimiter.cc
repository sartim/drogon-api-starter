#include "RateLimiter.h"

namespace observability {

RateLimiter::RateLimiter(const std::size_t limit,
                         const std::chrono::seconds window)
    : limit_(limit), window_(window) {}

bool RateLimiter::allow(const std::string& key) {
  if (limit_ == 0) return true;
  const auto now = std::chrono::steady_clock::now();
  std::lock_guard lock(mutex_);
  auto& timestamps = requests_[key];
  while (!timestamps.empty() && now - timestamps.front() >= window_) {
    timestamps.pop_front();
  }
  if (timestamps.size() >= limit_) return false;
  timestamps.push_back(now);
  return true;
}

}  // namespace observability
