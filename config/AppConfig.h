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
  std::string dbName;
  std::string dbUser;
  std::string dbPassword;
  std::string sentryDsn;
  std::string httpHost{"0.0.0.0"};
  int httpPort{8000};
  bool redisEnabled{false};
  std::string redisHost{"127.0.0.1"};
  int redisPort{6379};
  std::string redisPassword;
  int redisDb{0};

  static AppConfig fromValues(const std::map<std::string, std::string>& values);
  static AppConfig load(const std::filesystem::path& envFile);

  Json::Value toDrogonJson() const;
  std::string databaseConnectionString() const;
};

std::filesystem::path findEnvFile();

}  // namespace config
