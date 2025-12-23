cmake_minimum_required(VERSION 3.10)

project(practice C)

file(
	GLOB
	SOURCES
	"src/main.c"
)

add_executable(dummy ${SOURCES})

target_include_directories(
	dummy
	PRIVATE
	"${CMAKE_SOURCE_DIR}/../cglm/include" "${CMAKE_SOURCE_DIR}/../freetype/include" "${CMAKE_SOURCE_DIR}/../SDL/include" "${CMAKE_SOURCE_DIR}/../glad/include"
)

target_compile_definitions(dummy PRIVATE HOT)
