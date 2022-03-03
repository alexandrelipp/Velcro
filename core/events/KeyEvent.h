#pragma once

#include "Event.h"

#include "../../core/Codes.h"


class KeyEvent : public Event
{
public:
    KeyCode getKeyCode() const { return _keyCode; }

    std::string customString() override {
        return std::string(magic_enum::enum_name(_keyCode));
    }

protected:
    KeyEvent(const KeyCode keycode, Event::Type type)
            : Event(type), _keyCode(keycode) {}

    KeyCode _keyCode;
};

class KeyPressedEvent : public KeyEvent
{
public:
    KeyPressedEvent(const KeyCode keycode, const uint16_t repeatCount)
            : KeyEvent(keycode, Event::Type::KEY_PRESSED), _repeatCount(repeatCount) {}

    uint16_t getRepeatCount() const { return _repeatCount; }

    std::string customString() override {
        return KeyEvent::customString() + "Repeat" + std::to_string(_repeatCount);
    }

private:
    uint16_t _repeatCount;
};

class KeyReleasedEvent : public KeyEvent
{
public:
    KeyReleasedEvent(const KeyCode keycode)
            : KeyEvent(keycode, Event::Type::KEY_RELEASED) {}


};

class KeyTypedEvent : public KeyEvent
{
public:
    KeyTypedEvent(const KeyCode keycode)
            : KeyEvent(keycode, Event::Type::KEY_TYPED) {}
};