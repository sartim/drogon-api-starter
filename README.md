# User Service

[![Language](https://img.shields.io/badge/language-cpp-green.svg)](https://github.com/sartim/drogon_user_service)
[![Build Status](https://github.com/sartim/drogon_user_service/workflows/build/badge.svg)](https://github.com/sartim/drogon_user_service)

User service running on Drogon Framework which handles RBAC management. Make sure to add models.json and config.json.

## Requirements

* [Drogon](https://github.com/drogonframework/drogon)
* [PostgreSQL](https://www.postgresql.org)
* [JWT-CPP](https://github.com/Thalhammer/jwt-cpp)
* [Bcrypt](https://git@github.com:hilch/Bcrypt.cpp.git)
* [OpenSSL](https://github.com/openssl/openssl.git)

## Local development setup

On macOS, the repository includes a bootstrap script for the supported local
workflow. It installs the Homebrew dependencies, fetches Bcrypt.cpp, configures
the project, builds the service and test binary, and runs the unit tests:

    $ ./scripts/setup_local.sh

The script requires [Homebrew](https://brew.sh/) and does not create or modify
database data. To run the service itself, copy `.env.example` to `.env`, fill
in the values, start PostgreSQL, then run:

    $ cmake --build build --parallel
    $ ./build/drogon_user_service --action=run-server

To run tests after the initial setup:

    $ ctest --test-dir build --output-on-failure

On Linux, install Drogon, PostgreSQL development headers, OpenSSL, jwt-cpp,
libpqxx, JsonCpp, CMake, and a C++17 compiler using your distribution's
package manager. Then clone Bcrypt.cpp into the project root and use the same
`cmake`, `cmake --build`, and `ctest` commands. Docker remains available for a
fully isolated Linux build.

## Create .env file

    SECRET_KEY={SECRET_KEY}
    DB_NAME={DB_NAME}
    DB_USER={DB_USER}
    DB_PASSWORD={DB_PASSWORD}

## Setup bcrypt

On the project root:

    $ git clone https://github.com/hilch/Bcrypt.cpp.git

## Install jwt-cpp

    $ cd build
    $ cmake ..
    $ make
    $ sudo make install
    

## Install OpenSSL

    $ cd openssl
    $ ./config shared no-ssl2
    $ make
    $ make install


## Running server

    $ cd build
    $ cmake ..
    $ make
    $ ./drogoncore_user_service --action=run-server

# Create tables

    $ ./drogon_user_service --action=create-tables

# Drop tables

    $ ./drogon_user_service --action=drop-tables

## Running with docker
    
    $ docker compose up --build
