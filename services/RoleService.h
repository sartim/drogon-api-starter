#pragma once

#include "models/Roles.h"
#include "pagination/PageRequest.h"

#include <drogon/orm/DbClient.h>
#include <json/json.h>

#include <optional>
#include <string>
#include <vector>

namespace services {

class RoleService {
 public:
  explicit RoleService(drogon::orm::DbClientPtr client);
  pagination::PageResult<drogon_model::drogon_user_service::Roles> listRoles(
      const pagination::PageRequest& request) const;
  std::optional<drogon_model::drogon_user_service::Roles> findById(
      const std::string& id) const;
  drogon_model::drogon_user_service::Roles createRole(
      const Json::Value& input) const;
  drogon_model::drogon_user_service::Roles updateRole(
      const std::string& id, const Json::Value& input) const;
  bool deleteRole(const std::string& id) const;
  static Json::Value toPublicJson(
      const drogon_model::drogon_user_service::Roles& role);

 private:
  drogon::orm::DbClientPtr client_;
};

}  // namespace services
