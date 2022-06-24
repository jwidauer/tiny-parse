cmake_minimum_required(VERSION 3.10)

project(tiny-parse)

add_library(${PROJECT_NAME} INTERFACE)
add_library(tiny-parse::tiny-parse ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} INTERFACE include/)
