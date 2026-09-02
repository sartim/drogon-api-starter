#pragma once

#include "BaseController.h"
#include <drogon/HttpController.h>
#include <drogon/HttpResponse.h>
#include <drogon/orm/DbClient.h>

using namespace std;
using namespace drogon;
using namespace drogon::orm;

class RoleController : public HttpController<RoleController, false>,
                       public BaseController {
public:
  METHOD_LIST_BEGIN
  METHOD_ADD(RoleController::getHeaders, "/api/v1/role", Options);
  METHOD_ADD(RoleController::getByIdHeaders, "/api/v1/role/{id}", Options);
  METHOD_ADD(RoleController::getRoles, "/api/v1/role", Get, "AuthFilter");
  METHOD_ADD(RoleController::getRoleById, "/api/v1/role/{id}", Get,
             "AuthFilter");
  METHOD_ADD(RoleController::createRole, "/api/v1/role", Post, "AuthFilter");
  METHOD_ADD(RoleController::updateRoleById, "/api/v1/role/{id}", Put,
             "AuthFilter");
  METHOD_ADD(RoleController::deleteRoleById, "/api/v1/role/{id}", Delete,
             "AuthFilter");
  METHOD_LIST_END

public:
  RoleController() { connect(); }

public:
  // Get a list of all roles
  void getRoles(const HttpRequestPtr &req,
                function<void(const HttpResponsePtr &)> &&callback);

  // Get a role by their ID
  void getRoleById(const HttpRequestPtr &req,
                   function<void(const HttpResponsePtr &)> &&callback,
                   string id);

  // Create a new role
  void createRole(const HttpRequestPtr &req,
                  function<void(const HttpResponsePtr &)> &&callback);

  // Update a role by their ID
  void updateRoleById(const HttpRequestPtr &req,
                      function<void(const HttpResponsePtr &)> &&callback,
                      const string &id);

  // Delete a role by their ID
  void deleteRoleById(const HttpRequestPtr &req,
                      function<void(const HttpResponsePtr &)> &&callback,
                      const string &id);

private:
  drogon::orm::DbClientPtr client;
  void connect();
};
