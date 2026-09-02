#include "models/Users.h"
#include "services/UserService.h"

#include <iostream>

using drogon_model::drogon_user_service::Users;

int main() {
  Users user;
  user.setId("user-id");
  user.setFirstName("Ada");
  user.setLastName("Lovelace");
  user.setEmail("ada@example.com");
  user.setPassword("must-not-be-exposed");
  const auto publicUser = services::UserService::toPublicJson(user);

  if (publicUser["id"] != "user-id" ||
      publicUser["email"] != "ada@example.com" ||
      publicUser.isMember("password")) {
    std::cerr << "public user projection is incorrect\n";
    return 1;
  }
  return 0;
}
