#include "UserService.h"

#include <drogon/orm/Mapper.h>

#include <stdexcept>
#include <utility>

namespace services {

UserService::UserService(drogon::orm::DbClientPtr client)
    : client_(std::move(client)) {}

std::vector<drogon_model::drogon_user_service::Users> UserService::listUsers(
    int page, int pageSize) const {
  if (page < 1 || pageSize < 1 || pageSize > 100) {
    throw std::invalid_argument(
        "page must be positive and page_size must be between 1 and 100");
  }

  drogon::orm::Mapper<drogon_model::drogon_user_service::Users> mapper(client_);
  return mapper.orderBy(
                    drogon_model::drogon_user_service::Users::Cols::_created_at)
      .limit(pageSize)
      .offset((page - 1) * pageSize)
      .findAll();
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
