#pragma once

#include <drogon/HttpRequest.h>

#include <cstddef>
#include <string>
#include <vector>

namespace pagination {

struct PageRequest {
  static constexpr int kDefaultPage = 1;
  static constexpr int kDefaultPageSize = 25;
  static constexpr int kMaxPageSize = 100;

  int page{kDefaultPage};
  int pageSize{kDefaultPageSize};

  int offset() const noexcept { return (page - 1) * pageSize; }
  std::string cacheKey(const std::string& resource) const;

  static PageRequest from(const drogon::HttpRequestPtr& request);
};

template <typename T>
struct PageResult {
  std::vector<T> items;
  std::size_t total{0};
  PageRequest request;

  bool hasNext() const noexcept {
    return static_cast<std::size_t>(request.offset() + request.pageSize) <
           total;
  }
};

}  // namespace pagination
