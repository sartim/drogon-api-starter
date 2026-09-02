set(PLATFORM_TEST_SOURCES minimal_test_main.cc)
if (ENABLE_USER_SERVICE)
    set(PLATFORM_TEST_SOURCES test_main.cc
        ${CMAKE_SOURCE_DIR}/schemas/AuthSchema.cc)
endif ()
add_executable(drogon_user_service_test ${PLATFORM_TEST_SOURCES})

target_include_directories(drogon_user_service_test
                           PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(drogon_user_service_test PRIVATE Drogon::Drogon)

ParseAndAddDrogonTests(drogon_user_service_test)

add_executable(api_docs_test docs_test.cc)
target_compile_definitions(api_docs_test
                           PRIVATE DOCS_DIR="${CMAKE_SOURCE_DIR}/docs")
add_test(NAME ApiDocumentationFiles COMMAND api_docs_test)

add_executable(observability_test observability_test.cc
               ${CMAKE_SOURCE_DIR}/observability/Observability.cc)
target_include_directories(observability_test PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(observability_test PRIVATE Drogon::Drogon)
add_test(NAME ObservabilityFoundation COMMAND observability_test)

add_executable(config_test config_test.cc
               ${CMAKE_SOURCE_DIR}/config/AppConfig.cc)
target_include_directories(config_test PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(config_test PRIVATE Drogon::Drogon)
add_test(NAME AppConfigValidation COMMAND config_test)

if (ENABLE_USER_SERVICE)
    add_executable(user_service_test user_service_test.cc
                   ${CMAKE_SOURCE_DIR}/services/UserService.cc
                   ${CMAKE_SOURCE_DIR}/models/Users.cc)
    target_include_directories(user_service_test PRIVATE ${CMAKE_SOURCE_DIR})
    target_link_libraries(user_service_test PRIVATE Drogon::Drogon bcrypt)
    add_test(NAME UserPublicProjection COMMAND user_service_test)

    add_executable(role_service_test role_service_test.cc
                   ${CMAKE_SOURCE_DIR}/services/RoleService.cc
                   ${CMAKE_SOURCE_DIR}/models/Roles.cc)
    target_include_directories(role_service_test PRIVATE ${CMAKE_SOURCE_DIR})
    target_link_libraries(role_service_test PRIVATE Drogon::Drogon)
    add_test(NAME RolePublicProjection COMMAND role_service_test)
endif ()
