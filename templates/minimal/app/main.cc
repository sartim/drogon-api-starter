#include <drogon/drogon.h>

int main() {
  drogon::app().registerHandler(
      "/health",
      [](const drogon::HttpRequestPtr&,
         std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        Json::Value body;
        body["status"] = "up";
        callback(drogon::HttpResponse::newHttpJsonResponse(body));
      },
      {drogon::Get});
  drogon::app().addListener("0.0.0.0", 8000);
  drogon::app().run();
}
