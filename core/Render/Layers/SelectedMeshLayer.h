//
// Created by alexa on 2022-04-11.
//

#pragma once

#include "RenderLayer.h"
#include "../Objects/UniformBuffer.h"
#include "../Objects/ShaderStorageBuffer.h"
#include "../Objects/Texture.h"
#include "../../Scene/Scene.h"


class SelectedMeshLayer : public RenderLayer {
public:
    SelectedMeshLayer(VkRenderPass renderPass, std::shared_ptr<Scene> scene, const ShaderStorageBuffer& vertices,
                      const ShaderStorageBuffer& indices);

    virtual ~SelectedMeshLayer();

    virtual void update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) override;
    virtual void onEvent(Event& event) override;

    virtual void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) override;
    virtual void onImGuiRender() override;

    void setSelectedEntity(int selectedEntity);

private:
    struct SelectedMeshMVP{
        glm::mat4 original = glm::mat4(1.f);
        glm::mat4 scaledUp = glm::mat4(1.f);
    };
    // Buffers
    std::array<UniformBuffer, MAX_FRAMES_IN_FLIGHT> _mvpUniformBuffers{};
    std::shared_ptr<Scene> _scene = nullptr;

    // TODO : should be a vector!!
    MeshComponent* _selectedMesh = nullptr;
    int _selectedEntity = -1;

};
