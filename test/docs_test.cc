#include <fstream>
#include <iostream>
#include <string>

namespace {
bool contains(const std::string &path, const std::string &value) {
  std::ifstream file(path);
  if (!file) {
    return false;
  }
  const std::string contents((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
  return contents.find(value) != std::string::npos;
}
} // namespace

int main() {
  const std::string docsDir = DOCS_DIR;
  if (!contains(docsDir + "/openapi.yaml", "openapi: 3.0.3") ||
      !contains(docsDir + "/openapi.yaml", "/api/v1/user") ||
      !contains(docsDir + "/swagger.html", "SwaggerUIBundle") ||
      !contains(docsDir + "/swagger.html", "/openapi.yaml")) {
    std::cerr << "OpenAPI or Swagger UI documentation is incomplete\n";
    return 1;
  }
  return 0;
}
