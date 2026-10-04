cmake_minimum_required(VERSION 3.13)
project(GroqAIBuilder VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Geode REQUIRED)

setup_geode_mod(
    ${PROJECT_NAME}
    src/main.cpp
)
