#pragma once

#include <json/json.h>

#include <filesystem>
#include <map>
#include <string>

namespace config {

struct AppConfig {
  std::string secretKey;
  std::string dbHost;
  int dbPort{5432};
  int dbConnectionPoolSize{4};
  double dbQueryTimeoutSeconds{10.0};
  std::string dbName;
  std::string dbUser;
  std::string dbPassword;
  std::string sentryDsn;
  std::string errorTrackingProvider{"none"};
  std::string otlpEndpoint;
  double observabilityTimeoutSeconds{1.0};
  int observabilityBatchSize{10};
  double observabilityBatchDelaySeconds{0.1};
  int observabilityMaxQueueSize{1024};
  int observabilityRetryMaxAttempts{3};
  double observabilityRetryBaseDelaySeconds{0.1};
  int observabilityCircuitFailureThreshold{5};
  double observabilityCircuitOpenSeconds{30.0};
  std::string httpHost{"0.0.0.0"};
  int httpPort{8000};
  bool redisEnabled{false};
  std::string redisHost{"127.0.0.1"};
  int redisPort{6379};
  int redisConnectionPoolSize{2};
  double redisCommandTimeoutSeconds{1.0};
  std::string redisPassword;
  int redisDb{0};
  int idleConnectionTimeoutSeconds{60};
  int rateLimitRequests{0};
  int rateLimitWindowSeconds{60};

  static AppConfig fromValues(const std::map<std::string, std::string>& values);
  static AppConfig load(const std::filesystem::path& envFile);

  Json::Value toDrogonJson() const;
  std::string databaseConnectionString() const;
};

std::filesystem::path findEnvFile();

}  // namespace config
