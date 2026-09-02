#include "models/Roles.h"
#include "services/RoleService.h"

#include <iostream>

int main() {
  drogon_model::drogon_user_service::Roles role;
  role.setId("role-id");
  role.setName("admin");
  role.setDescription("Administrator");

  const auto publicRole = services::RoleService::toPublicJson(role);
  if (publicRole["id"] != "role-id" ||
      publicRole["name"] != "admin" ||
      publicRole["description"] != "Administrator") {
    std::cerr << "public role projection is incorrect\n";
    return 1;
  }
  return 0;
}
