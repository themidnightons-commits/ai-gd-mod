cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 20)

project(GroqAIBuilder VERSION 1.0.0)

include($ENV{GEODE_SDK}/cmake/GeodeFile.cmake)

add_geode_mod(
    GroqAIBuilder
    src/main.cpp
)

setup_geode_mod(GroqAIBuilder)
