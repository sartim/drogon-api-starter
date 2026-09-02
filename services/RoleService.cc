#include "RoleService.h"

#include <drogon/orm/Mapper.h>

#include <utility>

namespace services {

RoleService::RoleService(drogon::orm::DbClientPtr client)
    : client_(std::move(client)) {}

pagination::PageResult<drogon_model::drogon_user_service::Roles>
RoleService::listRoles(const pagination::PageRequest& request) const {
  drogon::orm::Mapper<drogon_model::drogon_user_service::Roles> mapper(client_);
  pagination::PageResult<drogon_model::drogon_user_service::Roles> result;
  result.request = request;
  result.total = mapper.count();
  result.items = mapper.orderBy(
                         drogon_model::drogon_user_service::Roles::Cols::_created_at)
                     .limit(request.pageSize)
                     .offset(request.offset())
                     .findAll();
  return result;
}

std::optional<drogon_model::drogon_user_service::Roles> RoleService::findById(
    const std::string& id) const {
  drogon::orm::Mapper<drogon_model::drogon_user_service::Roles> mapper(client_);
  auto role = mapper.findByPrimaryKey(id);
  if (!role.getId()) return std::nullopt;
  return role;
}

namespace {
void validateRoleInput(const Json::Value& input) {
  if (!input.isObject() || !input["name"].isString() ||
      !input["description"].isString()) {
    throw std::invalid_argument("Request body must contain name and description");
  }
}
}  // namespace

drogon_model::drogon_user_service::Roles RoleService::createRole(
    const Json::Value& input) const {
  validateRoleInput(input);
  drogon_model::drogon_user_service::Roles role;
  role.setName(input["name"].asString());
  role.setDescription(input["description"].asString());
  role.setIsDeleted(true);
  const auto now = trantor::Date::now();
  role.setCreatedAt(now);
  role.setUpdatedAt(now);
  drogon::orm::Mapper<drogon_model::drogon_user_service::Roles> mapper(client_);
  return mapper.insertFuture(role).get();
}

drogon_model::drogon_user_service::Roles RoleService::updateRole(
    const std::string& id, const Json::Value& input) const {
  validateRoleInput(input);
  drogon_model::drogon_user_service::Roles role;
  role.setId(id);
  role.setName(input["name"].asString());
  role.setDescription(input["description"].asString());
  role.setUpdatedAt(trantor::Date::now());
  drogon::orm::Mapper<drogon_model::drogon_user_service::Roles> mapper(client_);
  mapper.update(role);
  return role;
}

bool RoleService::deleteRole(const std::string& id) const {
  drogon::orm::Mapper<drogon_model::drogon_user_service::Roles> mapper(client_);
  return mapper.deleteByPrimaryKey(id);
}

Json::Value RoleService::toPublicJson(
    const drogon_model::drogon_user_service::Roles& role) {
  Json::Value result;
  result["id"] = role.getValueOfId();
  result["name"] = role.getValueOfName();
  result["description"] = role.getValueOfDescription();
  result["created_at"] = role.getValueOfCreatedAt().toDbString();
  result["updated_at"] = role.getValueOfUpdatedAt().toDbString();
  return result;
}

}  // namespace services
