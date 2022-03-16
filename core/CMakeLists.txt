cmake_minimum_required(VERSION 3.20)


target_sources(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/Render/Renderer.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Renderer.h"
        "${CMAKE_CURRENT_LIST_DIR}/Utils/UtilsVulkan.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Utils/UtilsVulkan.h"
        "${CMAKE_CURRENT_LIST_DIR}/Utils/UtilsFile.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Utils/UtilsFile.h"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Factory/FactoryVulkan.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Factory/FactoryVulkan.h"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Objects/UniformBuffer.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Objects/UniformBuffer.h"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Objects/ShaderStorageBuffer.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Objects/ShaderStorageBuffer.h"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Objects/Texture.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Objects/Texture.h"
        )