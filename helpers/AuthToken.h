#ifndef DROGON_USER_SERVICE_AUTHTOKEN_H
#define DROGON_USER_SERVICE_AUTHTOKEN_H

#include <string>
#include <iostream>
#include <chrono>
#include <jwt-cpp/jwt.h>

using namespace std;

static string generateJWT(const string &secretKey, const string &email) {
  auto token =
      jwt::create()
          .set_issuer("auth0")
          .set_type("JWS")
          .set_payload_claim("identity", jwt::claim(email))
          .set_issued_at(chrono::system_clock::now())
          .set_expires_at(chrono::system_clock::now() + chrono::seconds{3600})
          .sign(jwt::algorithm::hs256{secretKey});
  return token;
}

static bool verifyJWT(const std::string &secretKey, const std::string &tokenString) {
  if (!secretKey.empty() && !tokenString.empty()) {
    try {
      std::string token = tokenString;
      if (token.compare(0, 7, "Bearer ") == 0) {
        token = token.substr(7);
      }
      auto decodedToken = jwt::decode(token);
      auto verifier = jwt::verify()
                          .allow_algorithm(jwt::algorithm::hs256{secretKey})
                          .with_issuer("auth0");
      verifier.verify(decodedToken);
      return true;
    } catch (const std::exception &e) {
      LOG_ERROR << "Failed to verify token " << e.what();
      return false;
    }
  } else {
    LOG_ERROR << "Token missing ";
    return false;
  }
}

#endif // DROGON_USER_SERVICE_AUTHTOKEN_H
