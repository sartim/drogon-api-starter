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

double seconds(const std::map<std::string, std::string>& values,
               const std::string& key, double fallback) {
  const auto value = values.find(key);
  if (value == values.end() || value->second.empty()) return fallback;
  try {
    const auto parsed = std::stod(value->second);
    if (parsed <= 0.0 || parsed > 3600.0) throw std::out_of_range("seconds");
    return parsed;
  } catch (const std::exception&) {
    throw std::runtime_error("Invalid duration configuration: " + key);
  }
}

int nonNegativeNumber(const std::map<std::string, std::string>& values,
                      const std::string& key, int fallback) {
  const auto value = values.find(key);
  if (value == values.end() || value->second.empty()) return fallback;
  try {
    const auto parsed = std::stoi(value->second);
    if (parsed < 0 || parsed > 65535) throw std::out_of_range("number range");
    return parsed;
  } catch (const std::exception&) {
    throw std::runtime_error("Invalid numeric configuration: " + key);
  }
}

int grpcMessageLimit(const std::map<std::string, std::string>& values,
                     const std::string& key, int fallback) {
  const auto value = values.find(key);
  if (value == values.end() || value->second.empty()) return fallback;
  try {
    std::size_t parsedLength = 0;
    const auto parsed = std::stoi(value->second, &parsedLength);
    if (parsedLength != value->second.size() || parsed < 1024 ||
        parsed > 64 * 1024 * 1024) {
      throw std::out_of_range("gRPC message size");
    }
    return parsed;
  } catch (const std::exception&) {
    throw std::runtime_error("Invalid gRPC message size configuration: " + key);
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
  config.dbConnectionPoolSize = number(values, "DB_CONNECTION_POOL_SIZE", 4);
  config.dbQueryTimeoutSeconds = seconds(values, "DB_QUERY_TIMEOUT_SECONDS", 10.0);
  config.dbName = required(values, "DB_NAME");
  config.dbUser = required(values, "DB_USER");
  config.dbPassword = values.count("DB_PASSWORD") ? values.at("DB_PASSWORD") : "";
  config.sentryDsn = values.count("SENTRY_DSN") ? values.at("SENTRY_DSN") : "";
  config.errorTrackingProvider = values.count("ERROR_TRACKING_PROVIDER")
                                     ? values.at("ERROR_TRACKING_PROVIDER")
                                     : "none";
  config.otlpEndpoint = values.count("OTLP_ENDPOINT") ? values.at("OTLP_ENDPOINT") : "";
  config.observabilityTimeoutSeconds =
      seconds(values, "OBSERVABILITY_TIMEOUT_SECONDS", 1.0);
  config.observabilityBatchSize =
      number(values, "OBSERVABILITY_BATCH_SIZE", 10);
  config.observabilityBatchDelaySeconds =
      seconds(values, "OBSERVABILITY_BATCH_DELAY_SECONDS", 0.1);
  config.observabilityMaxQueueSize =
      number(values, "OBSERVABILITY_MAX_QUEUE_SIZE", 1024);
  config.observabilityRetryMaxAttempts =
      number(values, "OBSERVABILITY_RETRY_MAX_ATTEMPTS", 3);
  if (config.observabilityRetryMaxAttempts > 10) {
    throw std::runtime_error("Invalid numeric configuration: OBSERVABILITY_RETRY_MAX_ATTEMPTS");
  }
  config.observabilityRetryBaseDelaySeconds =
      seconds(values, "OBSERVABILITY_RETRY_BASE_DELAY_SECONDS", 0.1);
  config.observabilityCircuitFailureThreshold =
      number(values, "OBSERVABILITY_CIRCUIT_FAILURE_THRESHOLD", 5);
  config.observabilityCircuitOpenSeconds =
      seconds(values, "OBSERVABILITY_CIRCUIT_OPEN_SECONDS", 30.0);
  config.httpHost = values.count("HTTP_HOST") ? values.at("HTTP_HOST") : "0.0.0.0";
  config.httpPort = number(values, "HTTP_PORT", 8000);
  config.grpcEnabled = flag(values, "GRPC_ENABLED", false);
  config.grpcHost = values.count("GRPC_HOST") ? values.at("GRPC_HOST") : "0.0.0.0";
  config.grpcPort = number(values, "GRPC_PORT", 9000);
  config.grpcTlsCertFile = values.count("GRPC_TLS_CERT_FILE")
                               ? values.at("GRPC_TLS_CERT_FILE")
                               : "";
  config.grpcTlsKeyFile = values.count("GRPC_TLS_KEY_FILE")
                              ? values.at("GRPC_TLS_KEY_FILE")
                              : "";
  config.grpcAllowInsecure = flag(values, "GRPC_ALLOW_INSECURE", false);
  config.grpcMaxReceiveMessageBytes = grpcMessageLimit(
      values, "GRPC_MAX_RECEIVE_MESSAGE_BYTES", 4 * 1024 * 1024);
  config.grpcMaxSendMessageBytes = grpcMessageLimit(
      values, "GRPC_MAX_SEND_MESSAGE_BYTES", 4 * 1024 * 1024);
  if (config.grpcTlsCertFile.empty() != config.grpcTlsKeyFile.empty()) {
    throw std::runtime_error(
        "GRPC_TLS_CERT_FILE and GRPC_TLS_KEY_FILE must be set together");
  }
  if (config.grpcEnabled && config.grpcTlsCertFile.empty() &&
      !config.grpcAllowInsecure) {
    throw std::runtime_error(
        "gRPC requires TLS certificate/key files; set GRPC_ALLOW_INSECURE=true "
        "only for trusted development networks");
  }
  if (config.grpcEnabled && !config.grpcTlsCertFile.empty() &&
      config.grpcAllowInsecure) {
    throw std::runtime_error(
        "Choose TLS or GRPC_ALLOW_INSECURE, not both");
  }
  config.redisEnabled = flag(values, "REDIS_ENABLED", false);
  config.redisHost = values.count("REDIS_HOST") ? values.at("REDIS_HOST") : "127.0.0.1";
  config.redisPort = number(values, "REDIS_PORT", 6379);
  config.redisConnectionPoolSize =
      number(values, "REDIS_CONNECTION_POOL_SIZE", 2);
  config.redisCommandTimeoutSeconds =
      seconds(values, "REDIS_COMMAND_TIMEOUT_SECONDS", 1.0);
  config.redisPassword = values.count("REDIS_PASSWORD") ? values.at("REDIS_PASSWORD") : "";
  config.idleConnectionTimeoutSeconds =
      number(values, "HTTP_IDLE_CONNECTION_TIMEOUT_SECONDS", 60);
  config.rateLimitRequests =
      nonNegativeNumber(values, "RATE_LIMIT_REQUESTS", 0);
  config.rateLimitWindowSeconds =
      number(values, "RATE_LIMIT_WINDOW_SECONDS", 60);
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
                          "ERROR_TRACKING_PROVIDER", "OTLP_ENDPOINT",
                          "OBSERVABILITY_TIMEOUT_SECONDS", "OBSERVABILITY_BATCH_SIZE",
                          "OBSERVABILITY_BATCH_DELAY_SECONDS", "HTTP_HOST",
                          "OBSERVABILITY_MAX_QUEUE_SIZE",
                          "OBSERVABILITY_RETRY_MAX_ATTEMPTS",
                          "OBSERVABILITY_RETRY_BASE_DELAY_SECONDS",
                          "OBSERVABILITY_CIRCUIT_FAILURE_THRESHOLD",
                          "OBSERVABILITY_CIRCUIT_OPEN_SECONDS",
                          "HTTP_PORT", "GRPC_ENABLED", "GRPC_HOST", "GRPC_PORT",
                          "GRPC_TLS_CERT_FILE", "GRPC_TLS_KEY_FILE",
                          "GRPC_ALLOW_INSECURE",
                          "GRPC_MAX_RECEIVE_MESSAGE_BYTES",
                          "GRPC_MAX_SEND_MESSAGE_BYTES",
                          "HTTP_IDLE_CONNECTION_TIMEOUT_SECONDS",
                          "RATE_LIMIT_REQUESTS", "RATE_LIMIT_WINDOW_SECONDS",
                          "DB_CONNECTION_POOL_SIZE", "DB_QUERY_TIMEOUT_SECONDS",
                          "REDIS_ENABLED", "REDIS_HOST", "REDIS_PORT",
                          "REDIS_CONNECTION_POOL_SIZE", "REDIS_COMMAND_TIMEOUT_SECONDS",
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
  client["connection_number"] = dbConnectionPoolSize;
  client["timeout"] = dbQueryTimeoutSeconds;
  client["filename"] = "";
  if (redisEnabled) {
    config["redis_clients"] = Json::arrayValue;
    auto& redis = config["redis_clients"][0];
    redis["name"] = "default";
    redis["host"] = redisHost;
    redis["port"] = redisPort;
    redis["passwd"] = redisPassword;
    redis["db"] = redisDb;
    redis["connection_number"] = redisConnectionPoolSize;
    redis["timeout"] = redisCommandTimeoutSeconds;
  }
  return config;
}

std::string AppConfig::databaseConnectionString() const {
  return "postgresql://" + dbUser + ":" + dbPassword + "@" + dbHost + ":" +
         std::to_string(dbPort) + "/" + dbName;
}

}  // namespace config
