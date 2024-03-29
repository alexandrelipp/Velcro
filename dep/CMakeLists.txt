cmake_minimum_required(VERSION 3.20)

# set up profiling if requested
if(PROFILE_VELCRO)
    message("Profile velcro from dep")

    # we build optick ourself
    target_sources(${PROJECT_NAME} PUBLIC
            "${CMAKE_CURRENT_LIST_DIR}/optick/src/optick.config.h"
            "${CMAKE_CURRENT_LIST_DIR}/optick/src/optick.h"
            "${CMAKE_CURRENT_LIST_DIR}/optick/src/optick_capi.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/optick/src/optick_capi.h"
            "${CMAKE_CURRENT_LIST_DIR}/optick/src/optick_common.h"
            "${CMAKE_CURRENT_LIST_DIR}/optick/src/optick_core.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/optick/src/optick_core.freebsd.h"
            "${CMAKE_CURRENT_LIST_DIR}/optick/src/optick_core.h"
            "${CMAKE_CURRENT_LIST_DIR}/optick/src/optick_core.linux.h"
            "${CMAKE_CURRENT_LIST_DIR}/optick/src/optick_core.macos.h"
            "${CMAKE_CURRENT_LIST_DIR}/optick/src/optick_core.platform.h"
            "${CMAKE_CURRENT_LIST_DIR}/optick/src/optick_core.win.h"
            "${CMAKE_CURRENT_LIST_DIR}/optick/src/optick_gpu.cpp"
            #"${CMAKE_CURRENT_LIST_DIR}/optick/src/optick_gpu.d3d12.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/optick/src/optick_gpu.h"
            "${CMAKE_CURRENT_LIST_DIR}/optick/src/optick_gpu.vulkan.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/optick/src/optick_memory.h"
            "${CMAKE_CURRENT_LIST_DIR}/optick/src/optick_message.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/optick/src/optick_message.h"
            "${CMAKE_CURRENT_LIST_DIR}/optick/src/optick_miniz.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/optick/src/optick_miniz.h"
            "${CMAKE_CURRENT_LIST_DIR}/optick/src/optick_serialization.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/optick/src/optick_serialization.h"
            "${CMAKE_CURRENT_LIST_DIR}/optick/src/optick_server.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/optick/src/optick_server.h"
            )

    target_include_directories(${PROJECT_NAME} PUBLIC
            "${CMAKE_CURRENT_LIST_DIR}/optick/src")

    # use this to link dynamically (requires the dll)
    #add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/optick")
    #target_link_libraries(${PROJECT_NAME} PUBLIC OptickCore)
endif()

target_sources(${PROJECT_NAME} PUBLIC
        # imgui base
        "${CMAKE_CURRENT_LIST_DIR}/imgui/imgui.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/imgui/imgui.h"
        "${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_demo.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_draw.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_internal.h"
        "${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_tables.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_widgets.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/imgui/imstb_rectpack.h"
        "${CMAKE_CURRENT_LIST_DIR}/imgui/imstb_textedit.h"
        "${CMAKE_CURRENT_LIST_DIR}/imgui/imstb_truetype.h"

        # imgui with vulkan + glfw
        "${CMAKE_CURRENT_LIST_DIR}/imgui/backends/imgui_impl_vulkan.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/imgui/backends/imgui_impl_vulkan.h"
        "${CMAKE_CURRENT_LIST_DIR}/imgui/backends/imgui_impl_glfw.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/imgui/backends/imgui_impl_glfw.h"

        # imguizmo
        "${CMAKE_CURRENT_LIST_DIR}/ImGuizmo/ImGuizmo.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/ImGuizmo/ImGuizmo.h"
)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/glfw" libGLFW)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/glm" libGLM)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/spdlog" libSpdlog)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/assimp" libAssimp)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/msdf-atlas-gen" libMsdf)

# we build freetype dynamically to solve the following problems:
# 1. Both Assimp and free type depend on zlib. If we link both statically we will get duplicated symbol link error (LNK 2005)
# 2. Both Assimp and free type made small changes to zlib meaning we can't make both link with the same zlib
# 3. It is probably possible to link statically, but would probably require to fork assimp and free type (their might be an easier way)

# Building one of the two library dynamically solves all of our problems

# set the flag to build as DLL
set(BUILD_SHARED_LIBS TRUE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/freetype" libFreetype)
# unset the flag for potential other usage
set(BUILD_SHARED_LIBS FALSE)


target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/glfw/include"
        "${CMAKE_CURRENT_LIST_DIR}/glm"
        "${CMAKE_CURRENT_LIST_DIR}/spdlog/include"
        "${CMAKE_CURRENT_LIST_DIR}/Vulkan/Include"
        "${CMAKE_CURRENT_LIST_DIR}/Vulkan/Include"
        "${CMAKE_CURRENT_LIST_DIR}/stbi"
        "${CMAKE_CURRENT_LIST_DIR}/assimp/include"
        "${CMAKE_CURRENT_LIST_DIR}/freetype/include"
        "${CMAKE_CURRENT_LIST_DIR}/msdf-atlas-gen"
        "${CMAKE_CURRENT_LIST_DIR}"
        "${CMAKE_CURRENT_LIST_DIR}/imgui"
    )

target_link_libraries(${PROJECT_NAME} PUBLIC
        glfw
        spdlog
        assimp
        freetype
        msdf-atlas-gen
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
