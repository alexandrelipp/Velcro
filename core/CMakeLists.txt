cmake_minimum_required(VERSION 3.20)

# add pch
target_precompile_headers(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/pch/pch.h")


target_sources(${PROJECT_NAME} PUBLIC
        # PCH + BASE FILES
        "${CMAKE_CURRENT_LIST_DIR}/pch/pch.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/pch/pch.h"
        "${CMAKE_CURRENT_LIST_DIR}/Application.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Application.h"

        # UTILS
        "${CMAKE_CURRENT_LIST_DIR}/Utils/UtilsVulkan.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Utils/UtilsVulkan.h"
        "${CMAKE_CURRENT_LIST_DIR}/Utils/UtilsFile.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Utils/UtilsFile.h"
        "${CMAKE_CURRENT_LIST_DIR}/Utils/UtilsMath.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Utils/UtilsMath.h"
        "${CMAKE_CURRENT_LIST_DIR}/Utils/UtilsTemplate.h"

        # FACTORY
        "${CMAKE_CURRENT_LIST_DIR}/Render/Factory/FactoryVulkan.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Factory/FactoryVulkan.h"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Factory/FactoryModel.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Factory/FactoryModel.h"

        # RENDER OBJECTS
        "${CMAKE_CURRENT_LIST_DIR}/Render/Renderer.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Renderer.h"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Objects/UniformBuffer.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Objects/UniformBuffer.h"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Objects/ShaderStorageBuffer.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Objects/ShaderStorageBuffer.h"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Objects/Texture.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Objects/Texture.h"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Objects/VertexBuffer.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Objects/VertexBuffer.h"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Objects/IndexBuffer.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Objects/IndexBuffer.h"

        # RENDER LAYERS
        "${CMAKE_CURRENT_LIST_DIR}/Render/Layers/RenderLayer.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Layers/RenderLayer.h"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Layers/MultiMeshLayer.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Layers/MultiMeshLayer.h"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Layers/ModelLayer.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Layers/ModelLayer.h"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Layers/LineLayer.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Layers/LineLayer.h"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Layers/FlipbookLayer.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Layers/FlipbookLayer.h"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Layers/SelectedMeshLayer.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Layers/SelectedMeshLayer.h"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Layers/ImGuiLayer.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Layers/ImGuiLayer.h"

        # CAMERA
        "${CMAKE_CURRENT_LIST_DIR}/Render/Camera/InterfaceCamera.h"
        #"${CMAKE_CURRENT_LIST_DIR}/Render/Camera/FirstPersonCamera.cpp"
        #"${CMAKE_CURRENT_LIST_DIR}/Render/Camera/FirstPersonCamera.h"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Camera/Camera.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Render/Camera/Camera.h"

        # SCENE
        "${CMAKE_CURRENT_LIST_DIR}/Scene/Scene.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Scene/Scene.h"
        )