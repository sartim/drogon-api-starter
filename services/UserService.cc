#include "UserService.h"

#include "bcrypt.h"
#include <drogon/orm/Mapper.h>

#include <utility>

namespace services {

UserService::UserService(drogon::orm::DbClientPtr client)
    : client_(std::move(client)) {}

pagination::PageResult<drogon_model::drogon_user_service::Users>
UserService::listUsers(const pagination::PageRequest& request) const {

  drogon::orm::Mapper<drogon_model::drogon_user_service::Users> mapper(client_);
  pagination::PageResult<drogon_model::drogon_user_service::Users> result;
  result.request = request;
  result.total = mapper.count();
  result.items = mapper.orderBy(
                         drogon_model::drogon_user_service::Users::Cols::_created_at)
                     .limit(request.pageSize)
                     .offset(request.offset())
                     .findAll();
  return result;
}

std::optional<drogon_model::drogon_user_service::Users> UserService::findById(
    const std::string& id) const {
  drogon::orm::Mapper<drogon_model::drogon_user_service::Users> mapper(client_);
  auto user = mapper.findByPrimaryKey(id);
  if (!user.getId()) {
    return std::nullopt;
  }
  return user;
}

namespace {
void validateUserInput(const Json::Value& input) {
  if (!input.isObject() || !input["first_name"].isString() ||
      !input["last_name"].isString() || !input["email"].isString() ||
      !input["password"].isString()) {
    throw std::invalid_argument(
        "Request body must contain first_name, last_name, email and password");
  }
}
}  // namespace

drogon_model::drogon_user_service::Users UserService::createUser(
    const Json::Value& input) const {
  validateUserInput(input);
  drogon_model::drogon_user_service::Users user;
  user.setFirstName(input["first_name"].asString());
  user.setLastName(input["last_name"].asString());
  user.setEmail(input["email"].asString());
  user.setIsDeleted(true);
  user.setPassword(bcrypt::generateHash(input["password"].asString()));
  const auto now = trantor::Date::now();
  user.setCreatedAt(now);
  user.setUpdatedAt(now);

  drogon::orm::Mapper<drogon_model::drogon_user_service::Users> mapper(client_);
  return mapper.insertFuture(user).get();
}

drogon_model::drogon_user_service::Users UserService::updateUser(
    const std::string& id, const Json::Value& input) const {
  validateUserInput(input);
  drogon_model::drogon_user_service::Users user;
  user.setId(id);
  user.setFirstName(input["first_name"].asString());
  user.setLastName(input["last_name"].asString());
  user.setEmail(input["email"].asString());
  user.setPassword(bcrypt::generateHash(input["password"].asString()));
  user.setUpdatedAt(trantor::Date::now());

  drogon::orm::Mapper<drogon_model::drogon_user_service::Users> mapper(client_);
  mapper.update(user);
  return user;
}

bool UserService::deleteUser(const std::string& id) const {
  drogon::orm::Mapper<drogon_model::drogon_user_service::Users> mapper(client_);
  return mapper.deleteByPrimaryKey(id);
}

Json::Value UserService::toPublicJson(
    const drogon_model::drogon_user_service::Users& user) {
  Json::Value result;
  result["id"] = user.getValueOfId();
  result["first_name"] = user.getValueOfFirstName();
  result["last_name"] = user.getValueOfLastName();
  result["email"] = user.getValueOfEmail();
  result["created_at"] = user.getValueOfCreatedAt().toDbString();
  result["updated_at"] = user.getValueOfUpdatedAt().toDbString();
  return result;
}

}  // namespace services
