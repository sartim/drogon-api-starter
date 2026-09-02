#define DROGON_TEST_MAIN
#include <drogon/drogon_test.h>
#include <drogon/drogon.h>
#include "schemas/AuthSchema.h"
#include "helpers/AuthToken.h"

DROGON_TEST(BasicTest)
{
    CHECK(1 + 1 == 2);
}

DROGON_TEST(AuthSchemaAcceptsValidCredentials)
{
    AuthSchema schema;
    Json::Value body;
    body["email"] = "user@example.com";
    body["password"] = "correct horse battery staple";

    CHECK(schema.validate(body).empty());
}

DROGON_TEST(AuthSchemaReportsMissingFields)
{
    AuthSchema schema;
    const auto errors = schema.validate(Json::Value(Json::objectValue));

    CHECK(errors.size() == 2);
}

DROGON_TEST(AuthSchemaRejectsInvalidFieldTypesAndEmptyValues)
{
    AuthSchema schema;
    Json::Value body;
    body["email"] = "";
    body["password"] = 123;

    CHECK(schema.validate(body).size() == 2);
}

DROGON_TEST(JwtAcceptsGeneratedToken)
{
    const std::string secret = "unit-test-secret";
    const auto token = generateJWT(secret, "user@example.com");

    CHECK(verifyJWT(secret, token));
    CHECK(verifyJWT(secret, "Bearer " + token));
}

DROGON_TEST(JwtRejectsWrongSecretAndMalformedToken)
{
    const std::string token = generateJWT("unit-test-secret", "user@example.com");

    CHECK(!verifyJWT("wrong-secret", token));
    CHECK(!verifyJWT("unit-test-secret", "not-a-jwt"));
}

int main(int argc, char** argv) 
{
    using namespace drogon;

    std::promise<void> p1;
    std::future<void> f1 = p1.get_future();

    // Start the main loop on another thread
    std::thread thr([&]() {
        // Queues the promise to be fulfilled after starting the loop
        app().getLoop()->queueInLoop([&p1]() { p1.set_value(); });
        app().run();
    });

    // The future is only satisfied after the event loop started
    f1.get();
    int status = test::run(argc, argv);

    // Ask the event loop to shutdown and wait
    app().getLoop()->queueInLoop([]() { app().quit(); });
    thr.join();
    return status;
}
