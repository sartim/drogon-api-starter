#include "RoleController.h"
#include "models/Roles.h"
#include "services/RoleService.h"
#include <drogon/HttpResponse.h>
#include <drogon/HttpSimpleController.h>
#include <drogon/HttpViewData.h>
#include <drogon/orm/Mapper.h>

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

      services::RoleService roleService(client);
      auto roles = roleService.listRoles(page, page_size);
      Json::Value RolesJson(Json::arrayValue);
      for (const auto &role : roles) {
        RolesJson.append(services::RoleService::toPublicJson(role));
      }
      Json::Value roleResults;
      roleResults["results"] = RolesJson;
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
