cmake_minimum_required(VERSION 3.13)

project(GroqAIBuilder VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Geode REQUIRED)

setup_geode_mod()

target_sources(${PROJECT_NAME} PRIVATE
    src/main.cpp
)
