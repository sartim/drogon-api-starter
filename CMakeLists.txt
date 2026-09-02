cmake_minimum_required(VERSION 3.16)

file(STRINGS "${CMAKE_SOURCE_DIR}/SERVICE_VERSION" SERVICE_VERSION LIMIT_COUNT 1)
if (NOT SERVICE_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "VERSION must contain a semantic version in X.Y.Z format")
endif ()

project(drogon_user_service VERSION ${SERVICE_VERSION} LANGUAGES CXX)

include(CTest)
enable_testing()

set(BCRYPT_CPP_INCLUDE_PATH "Bcrypt.cpp/include")
set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> qc <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_C_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")

include(CheckIncludeFileCXX)
include_directories(${BCRYPT_CPP_INCLUDE_PATH})

check_include_file_cxx(any HAS_ANY)
check_include_file_cxx(string_view HAS_STRING_VIEW)
check_include_file_cxx(coroutine HAS_COROUTINE)
if (NOT "${CMAKE_CXX_STANDARD}" STREQUAL "")
    # Do nothing
elseif (HAS_ANY AND HAS_STRING_VIEW AND HAS_COROUTINE)
    set(CMAKE_CXX_STANDARD 20)
elseif (HAS_ANY AND HAS_STRING_VIEW)
    set(CMAKE_CXX_STANDARD 17)
else ()
    set(CMAKE_CXX_STANDARD 14)
endif ()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Native setup scripts install Drogon and jwt-cpp into .local. Keep CLion and
# command-line CMake builds self-contained without requiring a manual
# CMAKE_PREFIX_PATH setting.
if (EXISTS "${CMAKE_SOURCE_DIR}/.local/lib/cmake/Drogon/DrogonConfig.cmake")
    list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/.local")
endif ()

add_executable(${PROJECT_NAME} main.cc)

# ##############################################################################
# If you include the drogon source code locally in your project, use this method
# to add drogon 
# add_subdirectory(drogon) 
# target_link_libraries(${PROJECT_NAME} PRIVATE drogon)
#
# and comment out the following lines
find_package(Drogon CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Drogon::Drogon)

# ##############################################################################

if (CMAKE_CXX_STANDARD LESS 17)
    message(FATAL_ERROR "c++17 or higher is required")
elseif (CMAKE_CXX_STANDARD LESS 20)
    message(STATUS "use c++17")
else ()
    message(STATUS "use c++20")
endif ()

aux_source_directory(controllers CTL_SRC)
aux_source_directory(filters FILTER_SRC)
aux_source_directory(plugins PLUGIN_SRC)
aux_source_directory(models MODEL_SRC)
aux_source_directory(tables TABLE_SRC)
aux_source_directory(helpers HELPER_SRC)
aux_source_directory(schemas SCHEMA_SRC)
drogon_create_views(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/views
                    ${CMAKE_CURRENT_BINARY_DIR})
# use the following line to create views with namespaces.
# drogon_create_views(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/views
#                     ${CMAKE_CURRENT_BINARY_DIR} TRUE)
# use the following line to create views with namespace CHANGE_ME prefixed
# and path namespaces.
# drogon_create_views(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/views
#                     ${CMAKE_CURRENT_BINARY_DIR} TRUE CHANGE_ME)

target_include_directories(${PROJECT_NAME}
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                                   ${CMAKE_CURRENT_SOURCE_DIR}/models)
target_sources(${PROJECT_NAME}
               PRIVATE
               ${SRC_DIR}
               ${CTL_SRC}
               ${FILTER_SRC}
               ${PLUGIN_SRC}
               ${MODEL_SRC}
               ${TABLE_SRC}
               ${HELPER_SRC}
               ${SCHEMA_SRC})
# ##############################################################################
# uncomment the following line for dynamically loading views 
# set_property(TARGET ${PROJECT_NAME} PROPERTY ENABLE_EXPORTS ON)

# ##############################################################################

add_subdirectory(test)

find_package(OpenSSL REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${OPENSSL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENSSL_LIBRARIES})

if (EXISTS "${CMAKE_SOURCE_DIR}/.local/cmake/jwt-cpp-config.cmake")
    find_package(jwt-cpp CONFIG REQUIRED
                 PATHS "${CMAKE_SOURCE_DIR}/.local/cmake" NO_DEFAULT_PATH)
else ()
    find_package(jwt-cpp CONFIG REQUIRED)
endif ()
target_link_libraries(${PROJECT_NAME} PRIVATE jwt-cpp::jwt-cpp)

#find_package(AWSSDK CONFIG COMPONENTS core dynamodb kinesis s3 REQUIRED)
#target_include_directories(${PROJECT_NAME} PRIVATE ${AWSSDK_INCLUDE_DIRS})
#target_link_libraries(${PROJECT_NAME} PRIVATE ${AWSSDK_LIBRARIES})

add_library(bcrypt Bcrypt.cpp/src/bcrypt.cpp Bcrypt.cpp/src/blowfish.cpp)
target_include_directories(bcrypt PRIVATE Bcrypt.cpp/src/)
target_include_directories(bcrypt PUBLIC Bcrypt.cpp/include/)
target_link_libraries(${PROJECT_NAME} PRIVATE bcrypt)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED IMPORTED_TARGET libpqxx)
target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::PQXX)
