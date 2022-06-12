cmake_minimum_required(VERSION 3.20)
project(Velcro)

set(CMAKE_CXX_STANDARD 20)

option(PROFILE_VELCRO "Use optick to profile velcro" OFF)

if (PROFILE_VELCRO)
    message("Setting up a profile session")
    add_compile_definitions(PROFILE_VELCRO)
endif()

add_executable(${PROJECT_NAME}
        main.cpp
        )

# add other cmakeLists
include("${CMAKE_CURRENT_LIST_DIR}/dep/CMakeLists.txt")
include("${CMAKE_CURRENT_LIST_DIR}/core/CMakeLists.txt")

file(GLOB_RECURSE ASSETS "${CMAKE_SOURCE_DIR}/core/Assets/*")

#foreach(ASSET IN LISTS ASSETS)
#    message("Asset ${ASSET}")
#endforeach()


add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${ASSETS}
        ${CMAKE_CURRENT_BINARY_DIR}/core/Assets/)