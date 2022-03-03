#pragma once

#include "../../dep/magic_enum.hpp"

class Event {
public:
    virtual ~Event() = default;

    enum class Type{
        UNKNOWN = 0,
        KEY_PRESSED,
        KEY_RELEASED,
        KEY_TYPED,
        MOUSE_MOVEDD,
        MOUSE_PRESSED,
        MOUSE_RELEASED,
        MOUSE_SCROLLED,
        WINDOW_RESIZE,
//        WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
//        AppTick, AppUpdate, AppRender,
//        KeyPressed, KeyReleased, KeyTyped,
//        MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
    };

    bool isHandled() {return _handled; }
    void handle() {_handled = true; }
    Event::Type getType() { return _type; };

    std::string toString() {
        auto test = std::string(magic_enum::enum_name(_type));

        return std::string(magic_enum::enum_name(_type)) + " : " + customString();
    }

    Event(): _type(Type::UNKNOWN) {}

protected:
    Event(Type type) : _type(type) {}

    virtual std::string customString() = 0;

private:
    bool _handled = false;

    Type _type = Type::UNKNOWN;

};

class WindowResizeEvent : public Event {
public:
    WindowResizeEvent(int width, int height) :
    Event(Event::Type::WINDOW_RESIZE), _width(width), _height(height) {}
    std::string customString() override {
        return "size : (" + std::to_string(_width) + ", " + std::to_string(_height) + ')';
    }

    int getWidth() { return _width; }
    int getHeight() { return  _height; }

private:
    int _width = 1;
    int _height = 1;
};