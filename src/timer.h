#pragma once

#include <SDL.h>

struct TimeData {
    Uint64 then;
    Uint64 now;
    Uint64 frequency;
    double delta;
};

class Timer {
public:
    Timer() {
        m_time.now = SDL_GetPerformanceCounter();
        m_time.then = m_time.now;
        m_time.frequency = SDL_GetPerformanceFrequency();
    }

    // Note: delta is since last touch
    const TimeData &touch() {
        m_time.then = m_time.now;
        m_time.now = SDL_GetPerformanceCounter();
        m_time.delta = double(m_time.now - m_time.then) / double(m_time.frequency);

        return m_time;
    }
    // Note: delta is since last touch
    const TimeData &peek() {
        Uint64 now = SDL_GetPerformanceCounter();
        m_time.delta = double(now - m_time.then) / double(m_time.frequency);
        return m_time;
    }

protected:
    TimeData m_time;
};
