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