cmake_minimum_required(VERSION 3.13)

if(POLICY CMP0175)
  cmake_policy(SET CMP0175 OLD)
endif()

project(GroqAIBuilder VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    Geode
    GIT_REPOSITORY https://github.com/geode-sdk/geode
    GIT_TAG v5.10.1
)

FetchContent_MakeAvailable(Geode)

setup_geode_mod(
    BUILD_DIR build
)

add_geode_mod(
    GroqAIBuilder
    SOURCES
        src/main.cpp
)
