cmake_minimum_required(VERSION 3.10)

project(practice C)

add_executable(dummy src/main.c)

target_include_directories(dummy PRIVATE
    "${CMAKE_SOURCE_DIR}/../cglm/include"
    "${CMAKE_SOURCE_DIR}/../freetype/include"
    "${CMAKE_SOURCE_DIR}/../SDL/include"
    "${CMAKE_SOURCE_DIR}/../glad/include"
)

target_compile_definitions(dummy PRIVATE HOT)
