cmake_minimum_required(VERSION 3.20)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/glfw")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/glm")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/spdlog")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/assimp")

target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/glfw/include"
        "${CMAKE_CURRENT_LIST_DIR}/glm"
        "${CMAKE_CURRENT_LIST_DIR}/spdlog/include"
        "${CMAKE_CURRENT_LIST_DIR}/Vulkan/Include"
        "${CMAKE_CURRENT_LIST_DIR}/Vulkan/Include"
        "${CMAKE_CURRENT_LIST_DIR}/stbi"
        "${CMAKE_CURRENT_LIST_DIR}/assimp/include"
    )

target_link_libraries(${PROJECT_NAME} PUBLIC
        glfw
        spdlog
        assimp
    )

# we could also use the findVulkan cmake command
set(ENV{VULKAN_SDK} "C:/VulkanSDK/1.3.204.1")
find_package(Vulkan REQUIRED)
if(NOT Vulkan_FOUND)
    message("Vulkan not found. Cannot generate")
    return()
endif()

message("Linking with ${Vulkan_LIBRARIES}")
target_link_libraries(${PROJECT_NAME} PUBLIC ${Vulkan_LIBRARIES})

