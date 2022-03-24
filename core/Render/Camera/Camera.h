//
// Created by alexa on 2022-03-24.
//

#pragma once

#include "InterfaceCamera.h"

#include <glm/glm.hpp>


struct PProjectionInfo {
    float fov = 45.f;
    float nearClipZ = 0.01f;
    float farClipZ = 1000.f;
};

class Camera : public InterfaceCamera{
public:

    Camera(float aspectRatio);

    virtual void update(float deltaTime) override;
    virtual void onEvent(Event& event) override;

    virtual glm::mat4* getPVMatrix() override;
    virtual glm::vec3* getPosition() override;

    void setPosition(const glm::vec3& position);

    float* getYaw();
    float* getPitch();

    void reset();

    float* getViewMatrix();
    float* getProjectionMatrix();

    void setSpeed(float speed);

    void setRotation(float rotation);
    float getRotation();

    void translate(const glm::vec3& translation);

    // Perspective projection info
    PProjectionInfo* getPerspectiveProjectionInfo();


    // TODO : make these 2 methods private
    void recalculateProjectionMatrix();
    void recalculateViewMatrix();

private:

    static glm::quat calculateQuaternion(float pitch, float yaw);
    static glm::vec3 calculateForward(float pitch, float yaw);
    static glm::vec3 calculateRight(float pitch, float yaw);

    // constants
    static constexpr float BASE_YAW = 0.f;
    static constexpr float BASE_PITCH = 0.f;
    static constexpr glm::vec3 BASE_POS = glm::vec3(0.f, 0.f, 10.f);
    static constexpr glm::vec3 UP_VECTOR = {0.f, 1.f, 0.f}; // camera cannot roll (fixed up vector)
    static constexpr float MOUSE_SENSIBILITY = 0.005f;

private:
    float _aspectRatio = 1.f;
    float _scaleFactor = 1.f;

    // state
    glm::mat4 _projectionMatrix;
    glm::mat4 _viewMatrix;
    glm::mat4 _projectionViewMatrix;  ///< cached result (no need to recompute if not required)
    glm::vec3 _position = BASE_POS;
    float _yaw = BASE_YAW;
    float _pitch = BASE_PITCH;
    float _rotationZ = 0.f; ///< rotation around z axis

    PProjectionInfo _projInfo;   ///< Perspective projection info

    // dragging
    float _speed = 2.f;
    glm::vec2 _lastMousePos = glm::vec2(0.f);   ///< last mouse pos (in pixels with x->> and y down)
    bool _isFirstDrag = true;
};



