#include "AppConfig.h"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace config {
namespace {

std::string required(const std::map<std::string, std::string>& values,
                    const std::string& key) {
  const auto value = values.find(key);
  if (value == values.end() || value->second.empty()) {
    throw std::runtime_error("Missing required configuration: " + key);
  }
  return value->second;
}

int number(const std::map<std::string, std::string>& values,
           const std::string& key, int fallback) {
  const auto value = values.find(key);
  if (value == values.end() || value->second.empty()) {
    return fallback;
  }

  try {
    const auto parsed = std::stoi(value->second);
    if (parsed < 1 || parsed > 65535) {
      throw std::out_of_range("port range");
    }
    return parsed;
  } catch (const std::exception&) {
    throw std::runtime_error("Invalid numeric configuration: " + key);
  }
}

bool flag(const std::map<std::string, std::string>& values,
          const std::string& key, bool fallback) {
  const auto value = values.find(key);
  if (value == values.end() || value->second.empty()) return fallback;
  return value->second == "1" || value->second == "true" ||
         value->second == "TRUE";
}

std::map<std::string, std::string> readEnvFile(
    const std::filesystem::path& envFile) {
  std::ifstream file(envFile);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open .env file: " + envFile.string());
  }

  std::map<std::string, std::string> values;
  std::string line;
  while (std::getline(file, line)) {
    if (line.empty() || line.front() == '#') {
      continue;
    }

    const auto separator = line.find('=');
    if (separator == std::string::npos || separator == 0) {
      continue;
    }
    values[line.substr(0, separator)] = line.substr(separator + 1);
  }

  return values;
}

void overrideFromEnvironment(std::map<std::string, std::string>& values,
                             const std::string& key) {
  if (const auto* value = std::getenv(key.c_str()); value != nullptr) {
    values[key] = value;
  }
}

}  // namespace

std::filesystem::path findEnvFile() {
  const auto currentDirectory = std::filesystem::current_path();
  const std::filesystem::path candidates[] = {
      currentDirectory / ".env", currentDirectory.parent_path() / ".env"};

  for (const auto& candidate : candidates) {
    if (std::filesystem::is_regular_file(candidate)) {
      return candidate;
    }
  }

  throw std::runtime_error("Failed to find .env in " +
                           (currentDirectory / ".env").string() + " or " +
                           (currentDirectory.parent_path() / ".env").string());
}

AppConfig AppConfig::fromValues(const std::map<std::string, std::string>& values) {
  AppConfig config;
  config.secretKey = required(values, "SECRET_KEY");
  config.dbHost = required(values, "DB_HOST");
  config.dbPort = number(values, "DB_PORT", 5432);
  config.dbName = required(values, "DB_NAME");
  config.dbUser = required(values, "DB_USER");
  config.dbPassword = values.count("DB_PASSWORD") ? values.at("DB_PASSWORD") : "";
  config.sentryDsn = values.count("SENTRY_DSN") ? values.at("SENTRY_DSN") : "";
  config.errorTrackingProvider = values.count("ERROR_TRACKING_PROVIDER")
                                     ? values.at("ERROR_TRACKING_PROVIDER")
                                     : "none";
  config.httpHost = values.count("HTTP_HOST") ? values.at("HTTP_HOST") : "0.0.0.0";
  config.httpPort = number(values, "HTTP_PORT", 8000);
  config.redisEnabled = flag(values, "REDIS_ENABLED", false);
  config.redisHost = values.count("REDIS_HOST") ? values.at("REDIS_HOST") : "127.0.0.1";
  config.redisPort = number(values, "REDIS_PORT", 6379);
  config.redisPassword = values.count("REDIS_PASSWORD") ? values.at("REDIS_PASSWORD") : "";
  if (values.count("REDIS_DB") && !values.at("REDIS_DB").empty()) {
    try {
      config.redisDb = std::stoi(values.at("REDIS_DB"));
      if (config.redisDb < 0) throw std::out_of_range("redis db");
    } catch (const std::exception&) {
      throw std::runtime_error("Invalid numeric configuration: REDIS_DB");
    }
  }
  return config;
}

AppConfig AppConfig::load(const std::filesystem::path& envFile) {
  auto values = readEnvFile(envFile);
  for (const auto& key : {"SECRET_KEY", "DB_HOST", "DB_PORT", "DB_NAME",
                          "DB_USER", "DB_PASSWORD", "SENTRY_DSN",
                          "ERROR_TRACKING_PROVIDER", "HTTP_HOST",
                          "HTTP_PORT", "REDIS_ENABLED", "REDIS_HOST", "REDIS_PORT",
                          "REDIS_PASSWORD", "REDIS_DB"}) {
    overrideFromEnvironment(values, key);
  }
  return fromValues(values);
}

Json::Value AppConfig::toDrogonJson() const {
  Json::Value config;
  config["secret_key"] = secretKey;
  config["db_clients"] = Json::arrayValue;
  auto& client = config["db_clients"][0];
  client["name"] = "default";
  client["rdbms"] = "postgresql";
  client["host"] = dbHost;
  client["port"] = dbPort;
  client["dbname"] = dbName;
  client["user"] = dbUser;
  client["passwd"] = dbPassword;
  client["is_fast"] = false;
  client["connection_number"] = 1;
  client["filename"] = "";
  if (redisEnabled) {
    config["redis_clients"] = Json::arrayValue;
    auto& redis = config["redis_clients"][0];
    redis["name"] = "default";
    redis["host"] = redisHost;
    redis["port"] = redisPort;
    redis["passwd"] = redisPassword;
    redis["db"] = redisDb;
    redis["connection_number"] = 2;
    redis["timeout"] = 0.25;
  }
  return config;
}

std::string AppConfig::databaseConnectionString() const {
  return "postgresql://" + dbUser + ":" + dbPassword + "@" + dbHost + ":" +
         std::to_string(dbPort) + "/" + dbName;
}

}  // namespace config
