#include "UserController.h"
#include "bcrypt.h"
#include "models/Users.h"
#include <drogon/HttpResponse.h>
#include <drogon/HttpSimpleController.h>
#include <drogon/HttpViewData.h>
#include <drogon/orm/Mapper.h>

using namespace std;
using namespace drogon;
using namespace drogon::orm;
using namespace drogon_model::drogon_user_service;

void UserController::connect() {
  if (client == nullptr) {
    if (drogon::app().isRunning()) {
      client = drogon::app().getDbClient();
    };
  }
}

void UserController::getUsers(
    const HttpRequestPtr &req,
    function<void(const HttpResponsePtr &)> &&callback) {
  string method = req->methodString();
  string reqPath = req->path();
  LOG_DEBUG << "Received request: " << method << " " << req->path();

  connect(); // connect to db

  if (client) {
    try {
      // Get query parameters
      auto query = req->getParameters();
      int page = 1;
      auto page_it = query.find("page");
      if (page_it != query.end()) {
        page = stoi(page_it->second);
      }
      int page_size = 25;
      auto page_size_it = query.find("page_size");
      if (page_size_it != query.end()) {
        page_size = stoi(page_size_it->second);
      }

      if (page < 1 || page_size < 1 || page_size > 100) {
        Json::Value error;
        error["error"] = "page must be positive and page_size must be between 1 and 100";
        callback(handleResponse(error, k400BadRequest));
        return;
      }

      // Calculate offset and limit
      int offset = (page - 1) * page_size;
      int limit = page_size;

      Mapper<Users> mp(client);

      auto users = mp.orderBy(Users::Cols::_created_at)
                       .limit(limit)
                       .offset(offset)
                       .findAll();
      Json::Value usersJson(Json::arrayValue);
      for (const auto &user : users) {
        Json::Value userJson;
        userJson["id"] = user.getValueOfId();
        userJson["first_name"] = user.getValueOfFirstName();
        userJson["last_name"] = user.getValueOfLastName();
        userJson["email"] = user.getValueOfEmail();
        userJson["created_at"] = user.getValueOfCreatedAt().toDbString();
        userJson["updated_at"] = user.getValueOfUpdatedAt().toDbString();
        usersJson.append(userJson);
      }
      Json::Value userResults;
      userResults["results"] = usersJson;
      shared_ptr<HttpResponse> response = handleResponse(usersJson, k200OK);
      callback(response);
    } catch (const exception &e) {
      Json::Value error;
      error["error"] = e.what();
      shared_ptr<HttpResponse> response =
          handleResponse(error, k500InternalServerError);
      callback(response);
    }
  } else {
    Json::Value error;
    error["error"] = "Unable to connect to database";
    shared_ptr<HttpResponse> response =
        handleResponse(error, k500InternalServerError);
    callback(response);
  }
}

void UserController::getUserById(
    const HttpRequestPtr &req,
    function<void(const HttpResponsePtr &)> &&callback, string id) {
  string method = req->methodString();
  string reqPath = req->path();
  LOG_DEBUG << "Received request: " << method << " " << req->path();

  connect(); // connect to db

  if (client) {
    Mapper<Users> mp(client);
    auto user = mp.findByPrimaryKey(id);

    if (!user.getId()) {
      Json::Value error;
      error["error"] = "Record not found";
      callback(handleResponse(error, k404NotFound));
      return;
    }

    Json::Value usersJson(Json::arrayValue);
    Json::Value userJson;
    userJson["id"] = user.getValueOfId();
    userJson["first_name"] = user.getValueOfFirstName();
    userJson["last_name"] = user.getValueOfLastName();
    userJson["email"] = user.getValueOfEmail();
    userJson["created_at"] = user.getValueOfCreatedAt().toDbString();
    userJson["updated_at"] = user.getValueOfUpdatedAt().toDbString();
    usersJson.append(userJson);

    shared_ptr<HttpResponse> response = handleResponse(usersJson, k200OK);
    callback(response);
  } else {
    Json::Value error;
    error["error"] = "Unable to connect to database";
    shared_ptr<HttpResponse> response =
        handleResponse(error, k500InternalServerError);
    callback(response);
  }
}

void UserController::createUser(
    const HttpRequestPtr &req,
    function<void(const HttpResponsePtr &)> &&callback) {
  string method = req->methodString();
  string reqPath = req->path();
  LOG_DEBUG << "Received request: " << method << " " << req->path();

  connect(); // connect to db

  if (client) {
    Mapper<Users> mp(client);

    auto jsonBody = req->getJsonObject();

    if (jsonBody && (*jsonBody)["first_name"].isString() &&
        (*jsonBody)["last_name"].isString() &&
        (*jsonBody)["email"].isString() &&
        (*jsonBody)["password"].isString()) {
      Users user;
      user.setFirstName((*jsonBody)["first_name"].asString());
      user.setLastName((*jsonBody)["last_name"].asString());
      user.setEmail((*jsonBody)["email"].asString());
      user.setIsDeleted(true);
      string password = (*jsonBody)["password"].asString();
      string hashedPassword = bcrypt::generateHash(password);
      user.setPassword(hashedPassword);
      auto currDate = trantor::Date::now();
      user.setCreatedAt(currDate);
      user.setUpdatedAt(currDate);

      try {
        auto result = mp.insertFuture(user);
        auto r = result.get();
        shared_ptr<HttpResponse> resp = handleResponse(r.toJson(), k201Created);
        callback(resp);
      } catch (const exception &e) {
        cerr << "Exception caught: " << typeid(e).name() << " - " << e.what()
             << endl;

        Json::Value response;
        response["error"] = "Unable to create user";
        shared_ptr<HttpResponse> resp =
            handleResponse(response, k400BadRequest);
        callback(resp);
      }

    } else {
      Json::Value error;
      error["error"] = "Request body must contain first_name, last_name, email and password";
      shared_ptr<HttpResponse> response = handleResponse(error, k400BadRequest);
      callback(response);
    }
  } else {
    Json::Value error;
    error["error"] = "Unable to connect to database";
    shared_ptr<HttpResponse> response =
        handleResponse(error, k500InternalServerError);
    callback(response);
  }
}

void UserController::updateUserById(
    const HttpRequestPtr &req,
    function<void(const HttpResponsePtr &)> &&callback, const string &userId) {
  string method = req->methodString();
  string reqPath = req->path();
  LOG_DEBUG << "Received request: " << method << " " << req->path();

  connect(); // connect to db

  if (client) {
    Mapper<Users> mp(client);

    auto jsonBody = req->getJsonObject();
    if (!jsonBody || !(*jsonBody)["first_name"].isString() ||
        !(*jsonBody)["last_name"].isString() ||
        !(*jsonBody)["email"].isString() ||
        !(*jsonBody)["password"].isString()) {
      Json::Value error;
      error["error"] = "Request body must contain first_name, last_name, email and password";
      callback(handleResponse(error, k400BadRequest));
      return;
    }
    string firstName = jsonBody->get("first_name", "").asString();
    string lastName = jsonBody->get("last_name", "").asString();
    string email = jsonBody->get("email", "").asString();
    string password = jsonBody->get("password", "").asString();

    Users user;

    user.setId(userId);
    user.setFirstName(firstName);
    user.setLastName(lastName);
    user.setEmail(email);
    user.setEmail(email);

    string hashedPassword = bcrypt::generateHash(password);
    user.setPassword(hashedPassword);

    auto currDate = trantor::Date::now();
    user.setUpdatedAt(currDate);

    try {
      // Update the user in the database
      auto result = mp.update(user);
      shared_ptr<HttpResponse> response = handleResponse(user.toJson(), k200OK);
      callback(response);
    } catch (const exception &e) {
      cerr << "Exception caught: " << typeid(e).name() << " - " << e.what()
           << endl;
      Json::Value error;
      error["error"] = "Unable to update user";
      callback(handleResponse(error, k500InternalServerError));
    }
  } else {
    Json::Value error;
    error["error"] = "Unable to connect to database";
    shared_ptr<HttpResponse> response =
        handleResponse(error, k500InternalServerError);
    callback(response);
  }
}

void UserController::deleteUserById(
    const HttpRequestPtr &req,
    function<void(const HttpResponsePtr &)> &&callback, const string &userId) {
  string method = req->methodString();
  string reqPath = req->path();
  LOG_DEBUG << "Received request: " << method << " " << req->path();

  connect(); // connect to db

  if (client) {
    Mapper<Users> mp(client);
    auto user = mp.deleteByPrimaryKey(userId);

    if (user) {
      auto resp = HttpResponse::newHttpResponse();
      resp->setStatusCode(k204NoContent);
      resp->setContentTypeCode(CT_APPLICATION_JSON);
      resp->addHeader("Access-Control-Allow-Origin", "*");
      callback(resp);
    } else {
      Json::Value error;
      error["error"] = "Record not found";
      shared_ptr<HttpResponse> response = handleResponse(error, k400BadRequest);
      callback(response);
    }
  } else {
    Json::Value error;
    error["error"] = "Unable to connect to database";
    shared_ptr<HttpResponse> response =
        handleResponse(error, k500InternalServerError);
    callback(response);
  }
}
