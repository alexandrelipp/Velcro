#pragma once

#include "Event.h"
#include "../Codes.h"


class MouseMovedEvent : public Event
{
public:
    MouseMovedEvent(const float x, const float y)
            : Event(Event::Type::MOUSE_MOVEDD), _mouseX(x), _mouseY(y) {}

    float getX() const { return _mouseX; }
    float getY() const { return _mouseY; }

    std::string customString() override {
        return "Pos : (" + std::to_string(_mouseY) + " , " + std::to_string(_mouseX) + ")";
    }


private:
    float _mouseX, _mouseY;
};

class MouseScrolledEvent : public Event
{
public:
    MouseScrolledEvent(const float xOffset, const float yOffset)
            : Event(Event::Type::MOUSE_SCROLLED), _xOffset(xOffset), _yOffset(yOffset) {}

    float getXOffset() const { return _xOffset; }
    float getYOffset() const { return _yOffset; }

    std::string customString() override {
        return "Offset : (" + std::to_string(_xOffset) + " , " + std::to_string(_yOffset) + ")";
    }

private:
    float _xOffset, _yOffset;
};

class MouseButtonEvent : public Event
{
public:
    MouseCode getMouseButton() const { return _mouseCode; }


protected:
    std::string customString() override {
        return "Mouse button: " + std::string(magic_enum::enum_name(_mouseCode));
    }

    MouseButtonEvent(const MouseCode button, Event::Type type)
            : Event(type), _mouseCode(button) {}

    MouseCode _mouseCode;
};

class MouseButtonPressedEvent : public MouseButtonEvent
{
public:
    MouseButtonPressedEvent(const MouseCode button)
            : MouseButtonEvent(button, Event::Type::MOUSE_PRESSED) {}


};

class MouseButtonReleasedEvent : public MouseButtonEvent
{
public:
    MouseButtonReleasedEvent(const MouseCode button)
            : MouseButtonEvent(button, Event::Type::MOUSE_RELEASED) {}


};