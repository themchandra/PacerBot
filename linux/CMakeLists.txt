cmake_minimum_required(VERSION 3.18)
project(PacerBot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(hal)
add_subdirectory(app)

add_executable(pacerbot
    app/src/main.cpp
)
target_link_libraries(pacerbot PRIVATE app_core)