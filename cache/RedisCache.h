#pragma once

#include <drogon/nosql/RedisClient.h>

#include <optional>
#include <string>
#include <vector>

namespace cache {

// Redis is deliberately fail-open: an unavailable optional cache must never
// make a database-backed request fail.
class RedisCache {
 public:
  static void configure(bool enabled);
  static std::optional<std::string> get(const std::string& key);
  static void set(const std::string& key, const std::string& value,
                  int ttlSeconds);
  static void erase(const std::string& key);
  static void erasePrefix(const std::string& prefix);

 private:
  static drogon::nosql::RedisClientPtr client();
  static bool enabled_;
};

}  // namespace cache
