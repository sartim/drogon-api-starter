#include "controllers/AuthController.h"
#include "controllers/RoleController.h"
#include "controllers/UserController.h"
#include "config/AppConfig.h"
#include "models/Users.h"
#include "observability/Observability.h"
#include "tables/PermissionTable.h"
#include "tables/RolePermissionTable.h"
#include "tables/RoleTable.h"
#include "tables/UserPermissionTable.h"
#include "tables/UserTable.h"
#include <drogon/HttpAppFramework.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace std;
using namespace drogon;
using namespace drogon::orm;
using namespace drogon_model::drogon_user_service;

void createTables(const string &connectionString) {
  // User table
  UserTable userTable;
  userTable.create(connectionString);

  // Role  table
  RoleTable roleTable;
  roleTable.create(connectionString);

  // Permission  table
  PermissionTable permissionTable;
  permissionTable.create(connectionString);

  // User permission  table
  UserPermissionTable userPermissionTable;
  userPermissionTable.create(connectionString);

  // Role permission  table
  RolePermissionTable rolePermissionTable;
  rolePermissionTable.create(connectionString);
}

void registerRoutes() {
  auto healthHandler = [](const HttpRequestPtr &req,
                          function<void(const HttpResponsePtr &)> &&callback) {
    Json::Value response;
    response["status"] = "up";
    callback(HttpResponse::newHttpJsonResponse(response));
  };

  // Application-level liveness endpoint. It intentionally does not require
  // the database so orchestration can distinguish process health from DB
  // readiness.
  drogon::app().registerHandler("/health", healthHandler, {Get});
  drogon::app().registerHandler("/", healthHandler, {Get});

  drogon::app().registerHandler(
      "/ready",
      [](const HttpRequestPtr &,
         function<void(const HttpResponsePtr &)> &&callback) {
        Json::Value response;
        const bool databaseReady = drogon::app().hasDbClient("default") &&
                                    drogon::app().areAllDbClientsAvailable();
        response["status"] = databaseReady ? "ready" : "not_ready";
        response["database"] = databaseReady ? "up" : "down";
        auto result = HttpResponse::newHttpJsonResponse(response);
        result->setStatusCode(databaseReady ? k200OK : k503ServiceUnavailable);
        callback(result);
      },
      {Get});

  drogon::app().registerHandler(
      "/metrics",
      [](const HttpRequestPtr &,
         function<void(const HttpResponsePtr &)> &&callback) {
        auto response = HttpResponse::newHttpResponse();
        response->setStatusCode(k200OK);
        response->setContentTypeCodeAndCustomString(
            CT_CUSTOM, "text/plain; version=0.0.4; charset=utf-8");
        response->setBody(observability::metrics().prometheus());
        callback(response);
      },
      {Get});

  drogon::app().registerHandler(
      "/docs",
      [](const HttpRequestPtr &,
         function<void(const HttpResponsePtr &)> &&callback) {
        callback(HttpResponse::newRedirectionResponse("/swagger.html"));
      },
      {Get});

  drogon::app().registerHandler(
      "/openapi.yaml",
      [](const HttpRequestPtr &req,
         function<void(const HttpResponsePtr &)> &&callback) {
        callback(HttpResponse::newFileResponse(
            "./docs/openapi.yaml", "", CT_CUSTOM, "application/yaml", req));
      },
      {Get});

  drogon::app().registerHandler(
      "/swagger.html",
      [](const HttpRequestPtr &req,
         function<void(const HttpResponsePtr &)> &&callback) {
        callback(HttpResponse::newFileResponse(
            "./docs/swagger.html", "", CT_TEXT_HTML, "", req));
      },
      {Get});

  // Register generate jwt token
  auto authController = make_shared<AuthController>();
  drogon::app().registerHandler(
      "/api/v1/generate-jwt",
      [authController](const HttpRequestPtr &req,
                       function<void(const HttpResponsePtr &)> &&callback) {
        if (req->method() == Options) {
          authController->getHeaders(req, std::move(callback));
        } else if (req->method() == Post) {
          authController->asyncHandleHttpRequest(req, std::move(callback));
        }
      },
      {Options, Post});

  // Register user routes
  auto userController = make_shared<UserController>();
  drogon::app().registerHandler(
      "/api/v1/user",
      [userController](const HttpRequestPtr &req,
                       function<void(const HttpResponsePtr &)> &&callback) {
        if (req->method() == Options) {
          userController->getHeaders(req, std::move(callback));
        } else if (req->method() == Get) {
          userController->getUsers(req, std::move(callback));
        } else if (req->method() == Post) {
          userController->createUser(req, std::move(callback));
        }
      },
      {Options, Get, Post, "AuthFilter"});

  app().registerHandler(
      "/api/v1/user/{id}",
      [userController](const HttpRequestPtr &req,
                       function<void(const HttpResponsePtr &)> &&callback,
                       const string &id) {
        if (req->method() == Options) {
          userController->getByIdHeaders(req, std::move(callback), id);
        } else if (req->method() == Get) {
          userController->getUserById(req, std::move(callback), id);
        } else if (req->method() == Put) {
          userController->updateUserById(req, std::move(callback), id);
          ;
        } else if (req->method() == Delete) {
          userController->deleteUserById(req, std::move(callback), id);
          ;
        }
      },
      {Options, Get, Put, Delete, "AuthFilter"});

  // Register role routes
  auto roleController = make_shared<RoleController>();
  app().registerHandler(
      "/api/v1/role",
      [roleController](const HttpRequestPtr &req,
                       function<void(const HttpResponsePtr &)> &&callback) {
        if (req->method() == Options) {
          roleController->getHeaders(req, std::move(callback));
        } else if (req->method() == Get) {
          roleController->getRoles(req, std::move(callback));
        } else if (req->method() == Post) {
          roleController->createRole(req, std::move(callback));
        }
      },
      {Options, Get, Post, "AuthFilter"});
  app().registerHandler(
      "/api/v1/role/{id}",
      [roleController](const HttpRequestPtr &req,
                       function<void(const HttpResponsePtr &)> &&callback,
                       const string &id) {
        if (req->method() == Options) {
          roleController->getByIdHeaders(req, std::move(callback), id);
        } else if (req->method() == Get) {
          roleController->getRoleById(req, std::move(callback), id);
        } else if (req->method() == Put) {
          roleController->updateRoleById(req, std::move(callback), id);
          ;
        } else if (req->method() == Delete) {
          roleController->deleteRoleById(req, std::move(callback), id);
          ;
        }
      },
      {Options, Get, Put, Delete, "AuthFilter"});
}

void dropTables() {}

void runServer(const config::AppConfig &appConfig) {
  observability::configure(appConfig.sentryDsn);

  app().registerPreRoutingAdvice([](const HttpRequestPtr &request) {
    const auto id = observability::requestId(request);
    observability::metrics().recordRequest(request);
    LOG_INFO << "request_started request_id=" << id
             << " method=" << request->methodString()
             << " path=" << request->path();
  });

  app().registerPostHandlingAdvice(
      [](const HttpRequestPtr &request, const HttpResponsePtr &response) {
        observability::metrics().recordResponse(request, response);
        const auto id = observability::requestId(request);
        const auto status = response ? static_cast<int>(response->statusCode())
                                     : static_cast<int>(k500InternalServerError);
        if (response) {
          response->addHeader("X-Request-ID", id);
        }
        LOG_INFO << "request_completed request_id=" << id
                 << " method=" << request->methodString()
                 << " path=" << request->path() << " status=" << status;
  });

  // Set log level
  drogon::app().setLogLevel(trantor::Logger::kTrace);
  const auto port = appConfig.httpPort;
  // Set HTTP listener address and port
  drogon::app().addListener(appConfig.httpHost, port);

  // Load Drogon configuration directly from the values loaded from .env.
  try {
    drogon::app().loadConfigJson(appConfig.toDrogonJson());
    drogon::app().setDocumentRoot("./docs");
  } catch (const exception &e) {
    cerr << "Exception caught: " << typeid(e).name() << " - " << e.what()
         << endl;
  }

  // Register routes
  registerRoutes();

  // Run server
  LOG_INFO << "Server running on 127.0.0.1:" << port;
  drogon::app().run();
}

int main(int argc, char *argv[]) {
  config::AppConfig appConfig;
  try {
    // CLion commonly launches the binary from build/, while the development
    // configuration lives in the project root. Use the .env directory as the
    // process working directory so docs resolve consistently.
    const auto envFile = config::findEnvFile();
    std::filesystem::current_path(envFile.parent_path());
    appConfig = config::AppConfig::load(envFile);
  } catch (const exception &e) {
    cerr << "Unable to initialize configuration: " << e.what() << endl;
    return 1;
  }

  // Check if the correct number of command-line arguments is provided
  if (argc != 2) {
    cerr << "Usage: "
            "--action=run-server|create-tables|drop-tables"
         << endl;
    return 1;
  }

  // Parse the command-line argument
  string action = argv[1];

  // Extract the action from the argument
  size_t equalsPos = action.find('=');
  if (equalsPos == string::npos || equalsPos == action.length() - 1) {
    cerr << "Invalid argument format" << endl;
    return 1;
  }

  string key = action.substr(0, equalsPos);
  string value = action.substr(equalsPos + 1);

  // Database config to database cli interface
  // Check the action and perform the corresponding operation
  if (key == "--action") {
    if (value == "run-server") {
      runServer(appConfig);
    } else if (value == "create-tables") {
      createTables(appConfig.databaseConnectionString());
    } else if (value == "drop-tables") {
      dropTables();
    } else {
      cerr << "Invalid action" << endl;
      return 1;
    }
  } else {
    cerr << "Invalid argument" << endl;
    return 1;
  }

  return 0;
}
