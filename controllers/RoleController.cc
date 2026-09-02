#include "RoleController.h"
#include "models/Roles.h"
#include "services/RoleService.h"
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

void RoleController::connect() {
  if (client == nullptr) {
    if (drogon::app().isRunning()) {
      client = drogon::app().getDbClient();
    };
  }
}

void RoleController::getRoles(
    const HttpRequestPtr &req,
    function<void(const HttpResponsePtr &)> &&callback) {
  string method = req->methodString();
  string reqPath = req->path();
  LOG_DEBUG << "Received request: " << method << " " << req->path();

  connect(); // connect to db

  if (client) {
    try {
      const auto pageRequest = pagination::PageRequest::from(req);
      const auto key = pageRequest.cacheKey("roles");
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
      services::RoleService roleService(client);
      const auto roles = roleService.listRoles(pageRequest);
      const auto roleResults = pagination::toJson(
          roles, services::RoleService::toPublicJson);
      Json::StreamWriterBuilder writer;
      cache::RedisCache::set(key, Json::writeString(writer, roleResults), 30);
      shared_ptr<HttpResponse> response = handleResponse(roleResults, k200OK);
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

void RoleController::getRoleById(
    const HttpRequestPtr &req,
    function<void(const HttpResponsePtr &)> &&callback, string id) {
  string method = req->methodString();
  string reqPath = req->path();
  LOG_DEBUG << "Received request: " << method << " " << req->path();

  connect(); // connect to db

  if (client) {
    services::RoleService roleService(client);
    const auto role = roleService.findById(id);
    if (!role) {
      Json::Value error;
      error["error"] = "Record not found";
      callback(handleResponse(error, k404NotFound));
      return;
    }

    shared_ptr<HttpResponse> response = handleResponse(
        services::RoleService::toPublicJson(*role), k200OK);
    callback(response);
  } else {
    Json::Value error;
    error["error"] = "Unable to connect to database";
    shared_ptr<HttpResponse> response =
        handleResponse(error, k500InternalServerError);
    callback(response);
  }
}

void RoleController::createRole(
    const HttpRequestPtr &req,
    function<void(const HttpResponsePtr &)> &&callback) {
  string method = req->methodString();
  string reqPath = req->path();

  LOG_DEBUG << "Received request: " << method << " " << req->path();

  connect(); // connect to db

  if (client) {
    auto jsonBody = req->getJsonObject();
    try {
      services::RoleService roleService(client);
      const auto role = roleService.createRole(jsonBody ? *jsonBody : Json::Value());
      cache::RedisCache::erasePrefix("roles:");
      callback(handleResponse(role.toJson(), k201Created));
    } catch (const invalid_argument &) {
      Json::Value error;
      error["error"] = "Missing data in JSON body";
      callback(handleResponse(error, k400BadRequest));
    } catch (const exception &e) {
      cerr << "Exception caught: " << typeid(e).name() << " - " << e.what()
           << endl;
      Json::Value error;
      error["error"] = "Unable to create role";
      callback(handleResponse(error, k500InternalServerError));
      return;
    }
  } else {
    Json::Value error;
    error["error"] = "Unable to connect to database";
    shared_ptr<HttpResponse> response =
        handleResponse(error, k500InternalServerError);
    callback(response);
  }
}

void RoleController::updateRoleById(
    const HttpRequestPtr &req,
    function<void(const HttpResponsePtr &)> &&callback, const string &roleId) {
  string method = req->methodString();
  string reqPath = req->path();
  LOG_DEBUG << "Received request: " << method << " " << req->path();

  connect(); // connect to db

  if (client) {
    auto jsonBody = req->getJsonObject();
    try {
      services::RoleService roleService(client);
      const auto role = roleService.updateRole(
          roleId, jsonBody ? *jsonBody : Json::Value());
      cache::RedisCache::erasePrefix("roles:");
      shared_ptr<HttpResponse> response = handleResponse(role.toJson(), k200OK);
      callback(response);
    } catch (const invalid_argument &) {
      Json::Value error;
      error["error"] = "Request body must contain name and description";
      callback(handleResponse(error, k400BadRequest));
    } catch (const exception &e) {
      LOG_ERROR << "Failed to update role: " << e.what();
      Json::Value error;
      error["error"] = "Unable to update role";
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

void RoleController::deleteRoleById(
    const HttpRequestPtr &req,
    function<void(const HttpResponsePtr &)> &&callback, const string &roleId) {
  string method = req->methodString();
  string reqPath = req->path();
  LOG_DEBUG << "Received request: " << method << " " << req->path();

  connect(); // connect to db

  if (client) {
    services::RoleService roleService(client);
    auto role = roleService.deleteRole(roleId);
    cache::RedisCache::erasePrefix("roles:");

    if (role) {
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
