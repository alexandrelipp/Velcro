cmake_minimum_required(VERSION 3.20)
project(Velcro)

set(CMAKE_CXX_STANDARD 20)


add_executable(${PROJECT_NAME}
        main.cpp
        )

include("${CMAKE_CURRENT_LIST_DIR}/dep/CMakeLists.txt")
