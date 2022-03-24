#pragma once

#include <typeinfo>

class FPSCounter{
public:
    FPSCounter(float interval) : _interval(interval) {}

    bool tick(float dt, bool frameRenderered) {
        if (frameRenderered)
            ++_numFrames;

        _accumulatedTime += dt;
        if (_accumulatedTime > _interval){
            _currentFPS = _numFrames / _accumulatedTime;
            _accumulatedTime = 0.f;
            _numFrames = 0;
            return true;
        }
        return false;
    }
    float getFPS(){ return _currentFPS; }

private:
    float _interval;
    uint32_t _numFrames = 0;
    float _accumulatedTime = 0.f;
    float _currentFPS = 0.f;
};