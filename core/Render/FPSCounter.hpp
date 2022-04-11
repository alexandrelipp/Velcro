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
            // calculate current fps
            _currentFPS = (float)_numFrames / _accumulatedTime;

            // calculate average fps
            _totalTime += _accumulatedTime;
            _totalFrames += _numFrames;
            _averageFPS = (float)_totalFrames / _totalTime;

            // reset time and frames
            _accumulatedTime = 0.f;
            _numFrames = 0;
            return true;
        }
        return false;
    }
    float getFPS(){ return _currentFPS; }
    float getAverageFPS() { return _averageFPS; }

private:
    float _interval = 0.f;

    // used for current fps
    uint32_t _numFrames = 0;
    float _accumulatedTime = 0.f;
    float _currentFPS = 0.f;

    // used for average fps
    uint32_t _totalFrames = 0;
    float _totalTime = 0.f;
    float _averageFPS = 0.f;
};