#include "BaseController.h"
#include <drogon/HttpSimpleController.h>

using namespace std;
using namespace drogon;

class AuthController : public drogon::HttpSimpleController<AuthController>,
                       public BaseController {
public:
  virtual void asyncHandleHttpRequest(
      const HttpRequestPtr &req,
      function<void(const HttpResponsePtr &)> &&callback) override;
  PATH_LIST_BEGIN
  PATH_ADD("/api/v1/generate-jwt", Post);
  PATH_LIST_END
};
