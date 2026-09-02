#include "RedisCache.h"

#include <drogon/HttpAppFramework.h>
#include <trantor/utils/Logger.h>

namespace cache {

bool RedisCache::enabled_ = false;

void RedisCache::configure(const bool enabled) { enabled_ = enabled; }

drogon::nosql::RedisClientPtr RedisCache::client() {
  if (!enabled_) return nullptr;
  try {
    return drogon::app().getRedisClient("default");
  } catch (const std::exception& error) {
    LOG_WARN << "Redis cache unavailable: " << error.what();
    return nullptr;
  }
}

std::optional<std::string> RedisCache::get(const std::string& key) {
  const auto redis = client();
  if (!redis) return std::nullopt;
  try {
    auto value = redis->execCommandSync<std::string>(
        [](const drogon::nosql::RedisResult& result) {
          return result.isNil() ? std::string{} : result.asString();
        },
        "GET %s", key.c_str());
    return value.empty() ? std::nullopt : std::optional<std::string>(value);
  } catch (const std::exception& error) {
    LOG_WARN << "Redis GET failed: " << error.what();
    return std::nullopt;
  }
}

void RedisCache::set(const std::string& key, const std::string& value,
                     const int ttlSeconds) {
  const auto redis = client();
  if (!redis) return;
  try {
    redis->execCommandSync<std::string>(
        [](const drogon::nosql::RedisResult& result) {
          return result.getStringForDisplaying();
        },
        "SETEX %s %d %s", key.c_str(), ttlSeconds, value.c_str());
  } catch (const std::exception& error) {
    LOG_WARN << "Redis SET failed: " << error.what();
  }
}

void RedisCache::erase(const std::string& key) {
  const auto redis = client();
  if (!redis) return;
  try {
    redis->execCommandSync<long long>(
        [](const drogon::nosql::RedisResult& result) {
          return result.asInteger();
        },
        "DEL %s", key.c_str());
  } catch (const std::exception& error) {
    LOG_WARN << "Redis DEL failed: " << error.what();
  }
}

void RedisCache::erasePrefix(const std::string& prefix) {
  const auto redis = client();
  if (!redis) return;
  try {
    const auto keys = redis->execCommandSync<std::vector<std::string>>(
        [](const drogon::nosql::RedisResult& result) {
          std::vector<std::string> values;
          for (const auto& key : result.asArray()) values.push_back(key.asString());
          return values;
        },
        "KEYS %s*", prefix.c_str());
    for (const auto& key : keys) erase(key);
  } catch (const std::exception& error) {
    LOG_WARN << "Redis invalidation failed: " << error.what();
  }
}

}  // namespace cache
