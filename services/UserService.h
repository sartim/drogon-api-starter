#pragma once

#include "models/Users.h"
#include "pagination/PageRequest.h"

#include <json/json.h>

#include <drogon/orm/DbClient.h>
#include <optional>
#include <vector>

namespace services {

class UserService {
 public:
  explicit UserService(drogon::orm::DbClientPtr client);

  pagination::PageResult<drogon_model::drogon_user_service::Users> listUsers(
      const pagination::PageRequest& request) const;
  std::optional<drogon_model::drogon_user_service::Users> findById(
      const std::string& id) const;
  drogon_model::drogon_user_service::Users createUser(
      const Json::Value& input) const;
  drogon_model::drogon_user_service::Users updateUser(
      const std::string& id, const Json::Value& input) const;
  bool deleteUser(const std::string& id) const;

  // Convert a persistence model to the public API representation.
  // Passwords and internal flags are intentionally excluded.
  static Json::Value toPublicJson(
      const drogon_model::drogon_user_service::Users& user);

 private:
  drogon::orm::DbClientPtr client_;
};

}  // namespace services
