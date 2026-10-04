cmake_minimum_required(VERSION 3.13)
project(GroqAIBuilder VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_PREFIX_PATH $ENV{GEODE_SDK})
find_package(Geode REQUIRED)

add_library(${PROJECT_NAME} SHARED src/main.cpp)

setup_geode_mod(${PROJECT_NAME})
