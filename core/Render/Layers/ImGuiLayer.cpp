//
// Created by alexa on 2022-03-22.
//

#include "ImGuiLayer.h"
#include "../Factory/FactoryVulkan.h"
#include "../../Utils/UtilsVulkan.h"
#include "../../Application.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <ImGuizmo/ImGuizmo.h>

ImGuiLayer::ImGuiLayer(VkRenderPass renderPass) {
    ImGui::CreateContext();
    VK_ASSERT(ImGui_ImplGlfw_InitForVulkan(Application::getApp()->getWindow(), true), "Failed to init glfw");

    // Create Descriptor Pool. Taken from imgui vulkan exmaple. The number of descriptor might be extremely excessive
    {
        VkDescriptorPoolSize pool_sizes[] =
                {
                        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
                };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        VK_CHECK(vkCreateDescriptorPool(_vrd->device, &pool_info, nullptr, &_descriptorPool));
    }

    ImGui_ImplVulkan_InitInfo initInfo = {
            .Instance = _vrd->instance,
            .PhysicalDevice = _vrd->physicalDevice,
            .Device = _vrd->device,
            .QueueFamily = _vrd->graphicsQueueFamilyIndex,
            .Queue = _vrd->graphicsQueue,
            .DescriptorPool = _descriptorPool,
            .Subpass = 0,
            .MinImageCount = 2,
            .ImageCount = FB_COUNT,
            .MSAASamples = _vrd->sampleCount,
            .Allocator = nullptr
    };

    // init
    VK_ASSERT(ImGui_ImplVulkan_Init(&initInfo, renderPass), "Failed to init imgui for vulkan");

    //execute a gpu command to upload imgui font textures
    utils::executeOnQueueSync(_vrd->graphicsQueue, _vrd->device, _vrd->commandPool, [&](VkCommandBuffer cmd) {
        ImGui_ImplVulkan_CreateFontsTexture(cmd);
    });

    //clear font textures from cpu data
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

ImGuiLayer::~ImGuiLayer() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiLayer::fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) {
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void ImGuiLayer::update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) {}


void ImGuiLayer::onEvent(Event& event) {}


void ImGuiLayer::onImGuiRender() {
    bool show = false;
    //ImGui::ShowDemoWindow(&show);
}


void ImGuiLayer::begin() {
    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
}

void ImGuiLayer::end() {
    ImGui::Render();
}