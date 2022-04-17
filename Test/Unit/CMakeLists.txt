cmake_minimum_required(VERSION 3.20)
project(Tests)

set(CMAKE_CXX_STANDARD 20)

Include(FetchContent)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.0.0-preview3
)

FetchContent_MakeAvailable(Catch2)

add_executable(${PROJECT_NAME}
        TestUtils.cpp
        TestScene.cpp
        TestVertexBuffer.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE Catch2::Catch2WithMain)

include("${CMAKE_CURRENT_LIST_DIR}/../../dep/CmakeLists.txt")
include("${CMAKE_CURRENT_LIST_DIR}/../../core/CmakeLists.txt")

target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/../../"
        )