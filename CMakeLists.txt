cmake_minimum_required(VERSION 3.28)

project(kikai-lib LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_SCAN_FOR_MODULES True)

add_library(${PROJECT_NAME} STATIC)
target_sources(${PROJECT_NAME}
  PUBLIC
    FILE_SET modules TYPE CXX_MODULES FILES
    src/app.cpp
    src/base64.cpp
    src/comms.cpp
    src/io_loop.cpp
    src/logger.cpp
    src/mpscq.cpp
    src/sha1.cpp
    src/websocket.cpp
)
