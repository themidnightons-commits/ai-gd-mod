cmake_minimum_required(VERSION 3.13)
project(GroqAIBuilder VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)

include($ENV{GEODE_SDK}/cmake/GeodeFile.cmake)

add_library(${PROJECT_NAME} SHARED src/main.cpp)

setup_geode_mod(${PROJECT_NAME})
