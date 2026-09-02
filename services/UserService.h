#pragma once

#include "models/Users.h"

#include <json/json.h>

#include <drogon/orm/DbClient.h>
#include <optional>
#include <vector>

namespace services {

class UserService {
 public:
  explicit UserService(drogon::orm::DbClientPtr client);

  std::vector<drogon_model::drogon_user_service::Users> listUsers(
      int page, int pageSize) const;
  std::optional<drogon_model::drogon_user_service::Users> findById(
      const std::string& id) const;

  // Convert a persistence model to the public API representation.
  // Passwords and internal flags are intentionally excluded.
  static Json::Value toPublicJson(
      const drogon_model::drogon_user_service::Users& user);

 private:
  drogon::orm::DbClientPtr client_;
};

}  // namespace services
