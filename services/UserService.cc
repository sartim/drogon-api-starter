#include "UserService.h"

namespace services {

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
