//
// Created by alexa on 2022-03-22.
//

#include "ImGuiLayer.h"
#include "../Factory/FactoryVulkan.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_vulkan.h>

ImGuiLayer::ImGuiLayer(VkRenderPass renderPass) {
    ImGui::CreateContext();

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
            .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
            .Allocator = nullptr
    };
    //ImGui
    VK_ASSERT(ImGui_ImplVulkan_Init(&initInfo, renderPass), "Failed to init imgui for vulkan");
}

ImGuiLayer::~ImGuiLayer() {

}

void ImGuiLayer::fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t currentImage) {
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void ImGuiLayer::update(float dt, uint32_t currentImage) {

}

void ImGuiLayer::begin() {
    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    //ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::end() {
    ImGui::Render();
}
