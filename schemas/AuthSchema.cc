#include "AuthSchema.h"
#include <functional>
#include <map>
#include <string>
#include <json/json.h>

using namespace std;

std::vector<std::string>
AuthSchema::validate(const Json::Value &jsonBody) const {
  std::vector<std::string> errors;

  for (const auto &field : schema_) {
    const std::string &fieldName = field.first;
    bool required = field.second;

     if (required && (!jsonBody.isMember(fieldName) ||
                      !jsonBody[fieldName].isString() ||
                      jsonBody[fieldName].asString().empty())) {
         errors.push_back("Field '" + fieldName + "' must be a non-empty string.");
     }
  }

  return errors;
};
