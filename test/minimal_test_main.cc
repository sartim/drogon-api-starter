#define DROGON_TEST_MAIN
#include <drogon/drogon_test.h>
#include <drogon/drogon.h>

#include <future>
#include <thread>

DROGON_TEST(BasicTest) { CHECK(1 + 1 == 2); }

int main(int argc, char** argv) {
  std::promise<void> started;
  auto ready = started.get_future();
  std::thread loopThread([&started]() {
    drogon::app().getLoop()->queueInLoop(
        [&started]() { started.set_value(); });
    drogon::app().run();
  });
  ready.get();
  const auto status = drogon::test::run(argc, argv);
  drogon::app().getLoop()->queueInLoop([]() { drogon::app().quit(); });
  loopThread.join();
  return status;
}
