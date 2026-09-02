#include "PageRequest.h"

#include <stdexcept>

namespace pagination {

PageRequest PageRequest::from(const drogon::HttpRequestPtr& request) {
  PageRequest result;
  const auto parameters = request->getParameters();

  try {
    if (const auto it = parameters.find("page"); it != parameters.end()) {
      result.page = std::stoi(it->second);
    }
    if (const auto it = parameters.find("page_size"); it != parameters.end()) {
      result.pageSize = std::stoi(it->second);
    }
  } catch (const std::exception&) {
    throw std::invalid_argument("page and page_size must be integers");
  }

  if (result.page < 1 || result.pageSize < 1 ||
      result.pageSize > kMaxPageSize) {
    throw std::invalid_argument(
        "page must be positive and page_size must be between 1 and 100");
  }
  return result;
}

std::string PageRequest::cacheKey(const std::string& resource) const {
  return resource + ":page=" + std::to_string(page) + ":size=" +
         std::to_string(pageSize);
}

}  // namespace pagination
