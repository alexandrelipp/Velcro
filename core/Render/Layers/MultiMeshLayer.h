//
// Created by alexa on 2022-03-20.
//

#pragma once

#include "RenderLayer.h"
#include "../Objects/UniformBuffer.h"
#include "../Objects/ShaderStorageBuffer.h"
#include "../Objects/Texture.h"
#include "../../Scene/Scene.h"


class MultiMeshLayer : public RenderLayer {
public:
    MultiMeshLayer(VkRenderPass renderPass);
    virtual ~MultiMeshLayer();

    virtual void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t currentImage) override;
    virtual void update(float dt, uint32_t currentImage, const glm::mat4& pv) override;
    virtual void onImGuiRender() override;

private:
    void createPipelineLayout();
    void createDescriptorSets();

    void displayHierarchy(int entity);

private:
    struct InstanceData{
        glm::mat4 transform;
        uint32_t meshIndex;
        uint32_t materialIndex;
        uint32_t indexOffset;
    };

    // Buffers
    std::array<UniformBuffer, FB_COUNT> _vpUniformBuffers{};
    std::array<ShaderStorageBuffer, FB_COUNT> _meshTransformBuffers{};
    ShaderStorageBuffer _vertices{};
    ShaderStorageBuffer _indices{};
    ShaderStorageBuffer _indirectCommandBuffer{};
    //uint32_t _indexCount = 0;



    std::shared_ptr<Scene> _scene = nullptr;

    //Texture _texture{};
};

