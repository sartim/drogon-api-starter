#include "UserController.h"
#include "bcrypt.h"
#include "models/Users.h"
#include "services/UserService.h"
#include "cache/RedisCache.h"
#include "pagination/PageResponse.h"
#include <drogon/HttpResponse.h>
#include <drogon/HttpSimpleController.h>
#include <drogon/HttpViewData.h>
#include <drogon/orm/Mapper.h>
#include <sstream>

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
      const auto pageRequest = pagination::PageRequest::from(req);
      const auto key = pageRequest.cacheKey("users");
      if (const auto cached = cache::RedisCache::get(key); cached) {
        Json::Value cachedResponse;
        Json::CharReaderBuilder reader;
        std::string errors;
        std::istringstream input(*cached);
        if (Json::parseFromStream(reader, input, &cachedResponse, &errors)) {
          callback(handleResponse(cachedResponse, k200OK));
          return;
        }
      }
      services::UserService userService(client);
      const auto users = userService.listUsers(pageRequest);
      const auto userResults = pagination::toJson(
          users, services::UserService::toPublicJson);
      Json::StreamWriterBuilder writer;
      cache::RedisCache::set(key, Json::writeString(writer, userResults), 30);
      shared_ptr<HttpResponse> response = handleResponse(userResults, k200OK);
      callback(response);
    } catch (const invalid_argument &e) {
      Json::Value error;
      error["error"] = e.what();
      callback(handleResponse(error, k400BadRequest));
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
    services::UserService userService(client);
    const auto user = userService.findById(id);
    if (!user) {
      Json::Value error;
      error["error"] = "Record not found";
      callback(handleResponse(error, k404NotFound));
      return;
    }

    shared_ptr<HttpResponse> response = handleResponse(
        services::UserService::toPublicJson(*user), k200OK);
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
    auto jsonBody = req->getJsonObject();
    try {
      services::UserService userService(client);
      auto user = userService.createUser(jsonBody ? *jsonBody : Json::Value());
      cache::RedisCache::erasePrefix("users:");
      callback(handleResponse(user.toJson(), k201Created));
    } catch (const invalid_argument &) {
      Json::Value error;
      error["error"] = "Request body must contain first_name, last_name, email and password";
      callback(handleResponse(error, k400BadRequest));
    } catch (const exception &) {
      Json::Value response;
      response["error"] = "Unable to create user";
      callback(handleResponse(response, k400BadRequest));
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
    auto jsonBody = req->getJsonObject();
    try {
      services::UserService userService(client);
      auto user = userService.updateUser(
          userId, jsonBody ? *jsonBody : Json::Value());
      cache::RedisCache::erasePrefix("users:");
      shared_ptr<HttpResponse> response = handleResponse(user.toJson(), k200OK);
      callback(response);
    } catch (const invalid_argument &) {
      Json::Value error;
      error["error"] = "Request body must contain first_name, last_name, email and password";
      callback(handleResponse(error, k400BadRequest));
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
    services::UserService userService(client);
    auto user = userService.deleteUser(userId);
    cache::RedisCache::erasePrefix("users:");

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
