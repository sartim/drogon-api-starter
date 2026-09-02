cmake_minimum_required(VERSION 3.16)
project(@PROJECT_NAME@ LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Drogon CONFIG REQUIRED)

add_executable(@PROJECT_NAME@ app/main.cc)
target_link_libraries(@PROJECT_NAME@ PRIVATE Drogon::Drogon)

enable_testing()
add_executable(@PROJECT_NAME@_test tests/smoke_test.cc)
add_test(NAME SmokeTest COMMAND @PROJECT_NAME@_test)
