//
// Created by alexa on 2022-03-23.
//

#include "FirstPersonCamera.h"

#include "../../Application.h"
#include "../../core/events/KeyEvent.h"

FirstPersonCamera::FirstPersonCamera(const glm::vec3& pos, const glm::vec3& target, const glm::vec3& up):
        _cameraPosition(pos), _cameraOrientation(glm::lookAt(pos, target, up)), _up(up) {}

glm::mat4 FirstPersonCamera::getViewMatrix() const {
    const glm::mat4 t = glm::translate(glm::mat4(1.0f), -_cameraPosition);
    const glm::mat4 r = glm::mat4_cast(_cameraOrientation);
    return r * t;
}

glm::vec3 FirstPersonCamera::getPosition() const {
    return _cameraPosition;
}

void FirstPersonCamera::update(float dt) {
    glm::vec2 mousePos = Application::getApp()->getMousePos();
    if (Application::getApp()->isMouseButtonPressed(MouseCode::ButtonLeft))
    {
        const glm::vec2 delta = mousePos - _mousePos;
        const glm::quat deltaQuat = glm::quat(glm::vec3(-mouseSpeed * delta.y, mouseSpeed * delta.x, 0.0f));
        _cameraOrientation = deltaQuat * _cameraOrientation;
        _cameraOrientation = glm::normalize(_cameraOrientation);
        setUpVector(_up);
    }
    _mousePos = mousePos;

    const glm::mat4 v = glm::mat4_cast(_cameraOrientation);

    const glm::vec3 forward = -glm::vec3(v[0][2], v[1][2], v[2][2]);
    const glm::vec3 right = glm::vec3(v[0][0], v[1][0], v[2][0]);
    const glm::vec3 up = glm::cross(right, forward);

    glm::vec3 accel(0.0f);

    if (_movement.forward) accel += forward;
    if (_movement.backward) accel -= forward;

    if (_movement.left) accel -= right;
    if (_movement.right) accel += right;

    if (_movement.up) accel += up;
    if (_movement.down) accel -= up;

    if (_movement.fastSpeed) accel *= fastCoef;

    if (accel == glm::vec3(0))
    {
        // decelerate naturally according to the damping value
        _moveSpeed -= _moveSpeed * std::min((1.0f / damping) * static_cast<float>(dt), 1.0f);
    }
    else
    {
        // acceleration
        _moveSpeed += accel * acceleration * static_cast<float>(dt);
        const float maxSpeed = _movement.fastSpeed ? _maxSpeed * fastCoef : _maxSpeed;
        if (glm::length(_moveSpeed) > maxSpeed) _moveSpeed = glm::normalize(_moveSpeed) * maxSpeed;
    }

    _cameraPosition += _moveSpeed * static_cast<float>(dt);
}

void FirstPersonCamera::onEvent(Event& e) {
    //if (e.getType())
    switch (e.getType()) {
        case Event::Type::KEY_PRESSED:
        case Event::Type::KEY_RELEASED:{
            SPDLOG_INFO("Event {}", e.toString());
            bool pressed =  e.getType() != Event::Type::KEY_RELEASED;
            KeyEvent* keyEvent = (KeyEvent*)&e;
            switch (keyEvent->getKeyCode()) {
                case KeyCode::W:
                    _movement.forward = pressed;
                    break;
                case KeyCode::S:
                    _movement.backward = pressed;
                    break;
                case KeyCode::A:
                    _movement.left = pressed;
                    break;
                case KeyCode::D:
                    _movement.right = pressed;
                    break;
                case KeyCode::LeftShift:
                    _movement.fastSpeed = pressed;
                    break;
                case KeyCode::Space:
                    setUpVector({0.f, 1.f, 0.f});
                    break;
                default:
                    break;
            }
        }
        default:
            break;
    }
}


void FirstPersonCamera::resetMousePosition(const glm::vec2& p) {
    _mousePos = p;
}

void FirstPersonCamera::setPosition(const glm::vec3& pos) {
    _cameraPosition = pos;
}

void FirstPersonCamera::setUpVector(const glm::vec3& up) {
    const glm::mat4 view = getViewMatrix();
    const glm::vec3 dir = -glm::vec3(view[0][2], view[1][2], view[2][2]);
    _cameraOrientation = glm::lookAt(_cameraPosition, _cameraPosition + dir, up);
}

void FirstPersonCamera::lookAt(const glm::vec3& pos, const glm::vec3& target, const glm::vec3& up) {
    _cameraPosition = pos;
    _cameraOrientation = glm::lookAt(pos, target, up);
}