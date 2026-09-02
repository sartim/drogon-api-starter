#pragma once

#include "PageRequest.h"

#include <json/json.h>

namespace pagination {

template <typename T, typename Project>
Json::Value toJson(const PageResult<T>& page, Project project) {
  Json::Value response;
  response["results"] = Json::arrayValue;
  for (const auto& item : page.items) response["results"].append(project(item));
  response["page"] = page.request.page;
  response["page_size"] = page.request.pageSize;
  response["total"] = static_cast<Json::UInt64>(page.total);
  response["has_next"] = page.hasNext();
  return response;
}

}  // namespace pagination
