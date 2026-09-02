#include "pagination/PageRequest.h"

#include <iostream>

int main() {
  pagination::PageRequest request;
  request.page = 3;
  request.pageSize = 10;
  if (request.offset() != 20 ||
      request.cacheKey("users") != "users:page=3:size=10") {
    std::cerr << "pagination request calculation is incorrect\n";
    return 1;
  }

  pagination::PageResult<int> result{{1, 2}, 35, request};
  if (!result.hasNext()) return 1;
  request.page = 4;
  result.request = request;
  if (result.hasNext()) return 1;
  return 0;
}
