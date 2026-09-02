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
      {"HTTP_HOST", "127.0.0.1"},
      {"HTTP_PORT", "8080"},
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
