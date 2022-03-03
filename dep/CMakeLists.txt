cmake_minimum_required(VERSION 3.20)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/glfw")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/glm")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/spdlog")

target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/glfw/include"
        "${CMAKE_CURRENT_LIST_DIR}/glm"
        "${CMAKE_CURRENT_LIST_DIR}/spdlog/include"
        "${CMAKE_CURRENT_LIST_DIR}/Vulkan/Include"
    )

target_link_libraries(${PROJECT_NAME} PUBLIC
        glfw
        spdlog
    )

target_link_directories(${PROJECT_NAME} PUBLIC
        "C:/VulkanSDK/1.3.204.1/Lib" ## only works if the vulkan sdk version 1.3.204.1 is installed at default path
        )