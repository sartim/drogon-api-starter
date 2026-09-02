#pragma once

#include "models/Users.h"

#include <json/json.h>

namespace services {

class UserService {
 public:
  // Convert a persistence model to the public API representation.
  // Passwords and internal flags are intentionally excluded.
  static Json::Value toPublicJson(
      const drogon_model::drogon_user_service::Users& user);
};

}  // namespace services
