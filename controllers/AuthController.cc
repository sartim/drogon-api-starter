#include "AuthController.h"
#include "../helpers/AuthToken.h"
#include "../helpers/Env.h"
#include "../models/Users.h"
#include "bcrypt.h"
#include <drogon/HttpController.h>
#include <drogon/HttpResponse.h>
#include <drogon/orm/Mapper.h>

using namespace std;
using namespace drogon;
using namespace drogon::orm;
using namespace drogon_model::drogon_user_service;

void AuthController::asyncHandleHttpRequest(
    const HttpRequestPtr &req,
    function<void(const HttpResponsePtr &)> &&callback) {
  string method = req->methodString();
  string reqPath = req->path();
  LOG_DEBUG << "Received request: " << method << " " << req->path();

  auto client = drogon::app().getDbClient();
  if (client) {
    auto jsonBody = req->getJsonObject();
    if (!jsonBody || !(*jsonBody)["email"].isString() ||
        !(*jsonBody)["password"].isString()) {
      Json::Value error;
      error["error"] = "Request body must contain email and password";
      callback(handleResponse(error, k400BadRequest));
      return;
    }

    auto email = (*jsonBody)["email"].asString();
    auto password = (*jsonBody)["password"].asString();

    Mapper<Users> mp(client);
    auto user = mp.findFutureBy(Criteria(Users::Cols::_email, email));
    auto result = user.get();
    Json::Value usersJson(Json::arrayValue);

    // Check to handle invalid credentials

    if (!result.empty()) {
      for (const auto &user : result) {
        string hashedPassword = user.getValueOfPassword();
        bool isVerified = bcrypt::validatePassword(password, hashedPassword);
        if (!isVerified) {
          Json::Value error;
          error["error"] = "Invalid credentials";
          shared_ptr<HttpResponse> resp =
              handleResponse(error, k401Unauthorized);
          callback(resp);
          return;
        }
        Json::Value userJson;
        userJson["id"] = user.getValueOfId();
        userJson["first_name"] = user.getValueOfFirstName();
        userJson["last_name"] = user.getValueOfLastName();
        userJson["email"] = user.getValueOfEmail();
        usersJson.append(userJson);

        // Generate token for user found here and return as json
        const string secretKey = getEnv("SECRET_KEY");
        string access = generateJWT(secretKey, email);
        Json::Value response;
        response["access"] = access;
        response["user"] = usersJson;
        shared_ptr<HttpResponse> resp = handleResponse(response, k200OK);
        callback(resp);
      }
    } else {
      Json::Value error;
      error["error"] = "Invalid credentials";
      shared_ptr<HttpResponse> resp = handleResponse(error, k401Unauthorized);
      callback(resp);
    }
  } else {
    Json::Value error;
    error["error"] = "Unable to connect to the database";
    auto resp = HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}
