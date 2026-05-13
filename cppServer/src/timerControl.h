#pragma once 
#include <chrono>
#include "colors.h"
#include <print>

class timer_interface
{
public:
    virtual ~timer_interface() = default;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void tick() = 0;
    virtual double elapsed_seconds() = 0;
    virtual void reset() = 0;
};


//const auto start{ std::chrono::steady_clock::now() };
//const auto fb{ Fibonacci(42) };
//const auto finish{ std::chrono::steady_clock::now() };
//const std::chrono::duration<double> elapsed_seconds{ finish - start };

class production_timer : public timer_interface
{
public:
    void start() override {
        m_start = std::chrono::steady_clock::now();
        std::print(CCYAN"[*] Timer started\n"); std::print(CRESET);
        m_running = true;
    }

    void stop() override {
        if (m_running) {
            std::print(CCYAN"[*] Timer stopped\n"); std::print(CRESET);
            m_end = std::chrono::steady_clock::now();
            m_running = false;
        }
    }

    void tick() override {
        //std::print(CCYAN"[*] Timer ticked\n"); std::print(CRESET);
            m_end = std::chrono::steady_clock::now();
    }

    double elapsed_seconds() override {
        //std::print(CCYAN"[*] Timer elapsed?\n"); std::print(CRESET);
        const std::chrono::duration<double> elapsed_seconds{ m_end - m_start };
        return elapsed_seconds.count();
    }

    void reset() override {
        std::print(CCYAN"[*] Timer reset\n"); std::print(CRESET);
        m_start = std::chrono::steady_clock::now();
        m_end = m_start;
        m_running = false;
    }

private:
    std::chrono::time_point<std::chrono::steady_clock> m_start;
    std::chrono::time_point<std::chrono::steady_clock> m_end;
    bool m_running = false;
};