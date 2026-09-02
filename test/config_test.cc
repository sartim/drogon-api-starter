#include "config/AppConfig.h"

#include <iostream>
#include <map>
#include <stdexcept>

int main() {
  const std::map<std::string, std::string> values = {
      {"SECRET_KEY", "test-secret"},
      {"DB_HOST", "localhost"},
      {"DB_PORT", "5433"},
      {"DB_NAME", "users"},
      {"DB_USER", "tester"},
      {"DB_PASSWORD", "password"},
      {"ERROR_TRACKING_PROVIDER", "none"},
      {"HTTP_HOST", "127.0.0.1"},
      {"HTTP_PORT", "8080"},
      {"REDIS_ENABLED", "true"},
      {"REDIS_HOST", "cache"},
      {"REDIS_PORT", "6380"},
      {"REDIS_DB", "2"},
  };

  const auto config = config::AppConfig::fromValues(values);
  if (config.dbPort != 5433 || config.httpPort != 8080 ||
      config.databaseConnectionString() !=
          "postgresql://tester:password@localhost:5433/users") {
    std::cerr << "configuration values were not parsed correctly\n";
    return 1;
  }

  const auto drogonConfig = config.toDrogonJson();
  if (drogonConfig["db_clients"][0]["port"].asInt() != 5433) {
    std::cerr << "Drogon configuration was not generated correctly\n";
    return 1;
  }
  if (!config.redisEnabled || config.redisHost != "cache" ||
      config.redisPort != 6380 || config.redisDb != 2 ||
      drogonConfig["redis_clients"][0]["host"].asString() != "cache") {
    std::cerr << "Redis configuration was not generated correctly\n";
    return 1;
  }
  if (config.errorTrackingProvider != "none") {
    std::cerr << "error tracking provider was not parsed correctly\n";
    return 1;
  }

  auto invalid = values;
  invalid["HTTP_PORT"] = "not-a-port";
  try {
    (void)config::AppConfig::fromValues(invalid);
    std::cerr << "invalid port was accepted\n";
    return 1;
  } catch (const std::runtime_error&) {
    return 0;
  }
}
