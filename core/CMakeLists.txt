cmake_minimum_required(VERSION 3.20)


target_sources(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/Render/Renderer.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Renderer.h"
        )