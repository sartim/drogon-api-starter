#include "config/AppConfig.h"

#include <iostream>
#include <map>
#include <stdexcept>

int main() {
  const std::map<std::string, std::string> values = {
      {"SECRET_KEY", "test-secret"},
      {"DB_HOST", "localhost"},
      {"DB_PORT", "5433"},
      {"DB_CONNECTION_POOL_SIZE", "6"},
      {"DB_QUERY_TIMEOUT_SECONDS", "7.5"},
      {"DB_NAME", "users"},
      {"DB_USER", "tester"},
      {"DB_PASSWORD", "password"},
      {"ERROR_TRACKING_PROVIDER", "none"},
      {"OTLP_ENDPOINT", "http://collector:4318/v1/logs"},
      {"OBSERVABILITY_TIMEOUT_SECONDS", "2"},
      {"OBSERVABILITY_BATCH_SIZE", "20"},
      {"OBSERVABILITY_BATCH_DELAY_SECONDS", "0.25"},
      {"OBSERVABILITY_MAX_QUEUE_SIZE", "200"},
      {"OBSERVABILITY_RETRY_MAX_ATTEMPTS", "4"},
      {"OBSERVABILITY_RETRY_BASE_DELAY_SECONDS", "0.2"},
      {"OBSERVABILITY_CIRCUIT_FAILURE_THRESHOLD", "7"},
      {"OBSERVABILITY_CIRCUIT_OPEN_SECONDS", "45"},
      {"HTTP_HOST", "127.0.0.1"},
      {"HTTP_PORT", "8080"},
      {"HTTP_IDLE_CONNECTION_TIMEOUT_SECONDS", "45"},
      {"REDIS_ENABLED", "true"},
      {"REDIS_HOST", "cache"},
      {"REDIS_PORT", "6380"},
      {"REDIS_CONNECTION_POOL_SIZE", "3"},
      {"REDIS_COMMAND_TIMEOUT_SECONDS", "1.5"},
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
  if (config.dbConnectionPoolSize != 6 ||
      config.dbQueryTimeoutSeconds != 7.5 ||
      config.idleConnectionTimeoutSeconds != 45 ||
      drogonConfig["db_clients"][0]["connection_number"].asInt() != 6 ||
      drogonConfig["db_clients"][0]["timeout"].asDouble() != 7.5) {
    std::cerr << "pool and timeout configuration was not generated correctly\n";
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
  if (config.otlpEndpoint != "http://collector:4318/v1/logs" ||
      config.observabilityTimeoutSeconds != 2.0 ||
      config.observabilityBatchSize != 20 ||
      config.observabilityBatchDelaySeconds != 0.25 ||
      config.observabilityMaxQueueSize != 200 ||
      config.observabilityRetryMaxAttempts != 4 ||
      config.observabilityRetryBaseDelaySeconds != 0.2 ||
      config.observabilityCircuitFailureThreshold != 7 ||
      config.observabilityCircuitOpenSeconds != 45.0) {
    std::cerr << "observability batching configuration was not parsed correctly\n";
    return 1;
  }
  if (config.redisConnectionPoolSize != 3 ||
      config.redisCommandTimeoutSeconds != 1.5 ||
      drogonConfig["redis_clients"][0]["connection_number"].asInt() != 3 ||
      drogonConfig["redis_clients"][0]["timeout"].asDouble() != 1.5) {
    std::cerr << "Redis pool and timeout configuration was not generated correctly\n";
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
