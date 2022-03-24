//
// Created by alexa on 2022-03-23.
//

#pragma once

#include "InterfaceCamera.h"
#include "../../events/Event.h"


class FirstPersonCamera : public InterfaceCamera {
public:
    FirstPersonCamera(const glm::vec3& pos, const glm::vec3& target, const glm::vec3& up);

    virtual glm::mat4 getViewMatrix() const override;
    virtual glm::vec3 getPosition() const override;

    void update(float dt);
    void onEvent(Event& e);

    void setPosition(const glm::vec3& pos);
    void resetMousePosition(const glm::vec2& p);
    void setUpVector(const glm::vec3& up);
    void lookAt(const glm::vec3& pos, const glm::vec3& target, const glm::vec3& up);



public:
    float mouseSpeed = 4.0f;
    float acceleration = 150.0f;
    float damping = 0.2f;
    float _maxSpeed = 10.0f;
    float fastCoef = 10.0f;

private:

    struct Movement{
        bool forward = false;
        bool backward = false;
        bool left = false;
        bool right = false;
        bool up = false;
        bool down = false;
        bool fastSpeed = false;
    } _movement;

    glm::vec2 _mousePos = glm::vec2(0);
    glm::vec3 _cameraPosition = glm::vec3(0.0f, 10.0f, 10.0f);
    glm::quat _cameraOrientation = glm::quat(glm::vec3(0));
    glm::vec3 _moveSpeed = glm::vec3(0.0f);
    glm::vec3 _up = glm::vec3(0.0f, 0.0f, 1.0f); // TODO : what is that up vector??
};



