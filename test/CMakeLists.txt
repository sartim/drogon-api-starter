add_executable(drogon_user_service_test test_main.cc
               ${CMAKE_SOURCE_DIR}/schemas/AuthSchema.cc)

target_include_directories(drogon_user_service_test
                           PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(drogon_user_service_test PRIVATE Drogon::Drogon)

ParseAndAddDrogonTests(drogon_user_service_test)

add_executable(api_docs_test docs_test.cc)
target_compile_definitions(api_docs_test
                           PRIVATE DOCS_DIR="${CMAKE_SOURCE_DIR}/docs")
add_test(NAME ApiDocumentationFiles COMMAND api_docs_test)
