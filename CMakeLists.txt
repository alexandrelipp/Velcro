cmake_minimum_required(VERSION 3.20)
project(Velcro)

set(CMAKE_CXX_STANDARD 20)

option(PROFILE_VELCRO "Use optick to profile velcro" OFF)

if (PROFILE_VELCRO)
    message("Setting up a profile session")
    add_compile_definitions(PROFILE_VELCRO)
endif()

add_executable(${PROJECT_NAME}
        pch/pch.cpp
        pch/pch.h
        Application.cpp
        Application.h
        main.cpp
        )

# add pch
target_precompile_headers(${PROJECT_NAME} PUBLIC pch/pch.h)

# add other cmakeLists
include("${CMAKE_CURRENT_LIST_DIR}/dep/CMakeLists.txt")
include("${CMAKE_CURRENT_LIST_DIR}/core/CMakeLists.txt")