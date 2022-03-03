cmake_minimum_required(VERSION 3.20)
project(Velcro)

set(CMAKE_CXX_STANDARD 20)


add_executable(${PROJECT_NAME}
        pch/pch.cpp
        pch/pch.h
        Application.cpp
        Application.h
        main.cpp
        )

# add pch and add them to include directories
target_precompile_headers(${PROJECT_NAME} PUBLIC pch/pch.h)
##target_include_directories(${PROJECT_NAME} PUBLIC pch)

include("${CMAKE_CURRENT_LIST_DIR}/dep/CMakeLists.txt")
include("${CMAKE_CURRENT_LIST_DIR}/core/CMakeLists.txt")