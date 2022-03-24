//
// Created by alexa on 2022-03-24.
//

#include "Camera.h"

//
// Created by alexa on 2021-11-27.
//

#include "Camera.h"
#include "../../Application.h"
#include "../../events/MouseEvent.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/quaternion.hpp>

Camera::Camera(float aspectRatio) :
        _aspectRatio(aspectRatio), _viewMatrix(1.f) {
    recalculateViewMatrix();
    recalculateProjectionMatrix();
}

void Camera::setPosition(const glm::vec3& position) {
    _position = position;
    recalculateViewMatrix();
}

void Camera::reset() {
    _position = BASE_POS;
    _pitch = BASE_PITCH;
    _yaw = BASE_YAW;
    recalculateViewMatrix();
}


void Camera::update(float deltaTime){
    //SPDLOG_INFO("fps {}", 1.f/deltaTime);
    Application* application = Application::getApp();

    if (application->isKeyPressed(KeyCode::D)){
        glm::vec3 right = calculateRight(_pitch, _yaw);
        translate(right * _speed * deltaTime);
    }

    if (application->isKeyPressed(KeyCode::A)){
        glm::vec3 left = -calculateRight(_pitch, _yaw);
        translate(left * _speed * deltaTime);
    }

    if (application->isKeyPressed(KeyCode::W)){
        translate(UP_VECTOR * _speed * deltaTime);
    }

    if (application->isKeyPressed(KeyCode::S)){
        translate(-UP_VECTOR * _speed * deltaTime);
    }

    if (application->isMouseButtonPressed(MouseCode::ButtonRight)){
        glm::vec2 currentPos = Application::getApp()->getMousePos();
        glm::vec2 offset = _lastMousePos - currentPos;
        if (!_isFirstDrag && offset != glm::vec2{0.f, 0.f}){

            //offset.y *= -1.f;
            offset *= MOUSE_SENSIBILITY;

//            _yaw += offset.x * 0.01f;
//            _pitch += offset.y * 0.01f;
//            _forward.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
//            _forward.y = sin(glm::radians(_pitch));
//            _forward.z = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));
//            _forward = glm::normalize(_forward);
            recalculateViewMatrix();
        }
        _lastMousePos = currentPos;
        _isFirstDrag = false;
    }
    else if (application->isMouseButtonPressed(MouseCode::ButtonLeft)){
        glm::vec2 currentPos = Application::getApp()->getMousePos();
        glm::vec2 offset = _lastMousePos - currentPos;
        if (!_isFirstDrag && offset != glm::vec2(0.f)){
            offset *= MOUSE_SENSIBILITY;
            _pitch -= offset.y;
            _yaw -= offset.x;
            recalculateViewMatrix();
        }
        _lastMousePos = currentPos;
        _isFirstDrag = false;
    }
}

void Camera::onEvent(Event& event) {
    switch (event.getType()) {
        case Event::Type::MOUSE_SCROLLED: {
            MouseScrolledEvent* mouseScrolledEvent = (MouseScrolledEvent*) &event;

            constexpr float speed = 0.5f;
            if (mouseScrolledEvent->getYOffset() > 0)
                _position += _speed * calculateForward(_pitch, _yaw);
            else
                _position -= speed * calculateForward(_pitch, _yaw);


            recalculateViewMatrix();
            event.handle();
            return;
        }
        case Event::Type::WINDOW_RESIZE:{
            WindowResizeEvent* windowResizeEvent = (WindowResizeEvent*)&event;
            _aspectRatio = windowResizeEvent->getWidth()/ (float)windowResizeEvent->getHeight();
            recalculateProjectionMatrix();
            event.handle();
            return;
        }
        case Event::Type::MOUSE_RELEASED:{
            MouseButtonReleasedEvent* e = (MouseButtonReleasedEvent*)&event;
            switch (e->getMouseButton()){
                case MouseCode::ButtonRight:
                case MouseCode::ButtonLeft:
                    _isFirstDrag = true;
                default: ;
            }
            return;
        }
    }
}

void Camera::setSpeed(float speed) {
    _speed = speed;
}


glm::vec3* Camera::getPosition() {
    return &_position;
}


float* Camera::getYaw() {
    return &_yaw;
}

float* Camera::getPitch() {
    return &_pitch;
}


glm::mat4* Camera::getPVMatrix(){
    return &_projectionViewMatrix;
}

float* Camera::getViewMatrix() {
    return glm::value_ptr(_viewMatrix);
}

float* Camera::getProjectionMatrix() {
    return glm::value_ptr(_projectionMatrix);
}


float Camera::getRotation()  {
    return _rotationZ;
}

void Camera::setRotation(float rotation) {
    _rotationZ = rotation;
    recalculateViewMatrix();
}

void Camera::translate(const glm::vec3& translation){
    _position += translation;
    recalculateViewMatrix();
}


PProjectionInfo* Camera::getPerspectiveProjectionInfo() {
    return &_projInfo;
}

void Camera::recalculateViewMatrix() {
    // could be optimised


    //_viewMatrix = glm::lookAt(_viewMatInfo.position, _viewMatInfo.position + _viewMatInfo.forward, UP_VECTOR);
    //_viewMatrix = glm::lookAt(_viewMatInfo.position, _viewMatInfo.center, UP_VECTOR);
    glm::mat4 translate = glm::translate(glm::mat4(1.f), _position);
    glm::mat4 rotation = glm::toMat4(calculateQuaternion(_pitch, _yaw));
    _viewMatrix = glm::inverse(translate * rotation);
    _projectionViewMatrix = _projectionMatrix * _viewMatrix;
}

void Camera::recalculateProjectionMatrix() {
    _projectionMatrix = glm::perspective(glm::radians(_projInfo.fov), _aspectRatio, _projInfo.nearClipZ, _projInfo.farClipZ);
    //GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted.
    // The easiest way to compensate for that is to flip the sign on the scaling factor of the Y axis in
    // the projection matrix. If you don't do this, then the image will be rendered upside down.
    _projectionMatrix[1][1] *= -1;
    _projectionViewMatrix = _projectionMatrix * _viewMatrix;
}

glm::quat Camera::calculateQuaternion(float pitch, float yaw) {
    return glm::quat(glm::vec3(-pitch, -yaw, 0.0f));
}

glm::vec3 Camera::calculateForward(float pitch, float yaw) {
    return glm::rotate(calculateQuaternion(pitch, yaw), {0.f, 0.f, -1.f});
}

glm::vec3 Camera::calculateRight(float pitch, float yaw) {
    return glm::rotate(calculateQuaternion(pitch, yaw), {1.f, 0.f, 0.f});
}