#pragma once 
#include <chrono>
#include "colors.h"
#include <print>
#include <mutex>

class counter_interface
{
public:
    virtual ~counter_interface() = default;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void tick() = 0;
    virtual double get() = 0;
    virtual void reset() = 0;
};

class timeCounter : public counter_interface
{
public:
    void start() override {
        m_start = std::chrono::steady_clock::now();
        std::print(CCYAN"[*] Counter started\n"); std::print(CRESET);
        m_running = true;
    }

    void stop() override {
        if (m_running) {
            std::print(CCYAN"[*] Counter stopped\n"); std::print(CRESET);
            m_end = std::chrono::steady_clock::now();
            m_running = false;
        }
    }

    void tick() override {
            m_end = std::chrono::steady_clock::now();
    }

    double get() override {
        const std::chrono::duration<double> elapsed_seconds{ m_end - m_start };
        return elapsed_seconds.count();
    }

    void reset() override {
        m_start = std::chrono::steady_clock::now();
        m_end = m_start;
        m_running = false;
    }

private:
    std::chrono::time_point<std::chrono::steady_clock> m_start;
    std::chrono::time_point<std::chrono::steady_clock> m_end;
    bool m_running = false;
};

class clockCounter : public counter_interface
{
public:
    void start() override {
		std::lock_guard<std::mutex> lock(m_mutex);
        m_clock = 0;
        std::print(CCYAN"[*] Counter started\n"); std::print(CRESET);
        m_running = true;
    }

    void stop() override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_running) {
            m_running = false;
        }
    }

    void tick() override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_clock++;
    }

    double get() override {
        std::lock_guard<std::mutex> lock(m_mutex);
        return static_cast<double>(m_clock);
    }

    void reset() override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_clock = 0;
        m_running = false;
    }

private:
    uint64_t m_clock;
    bool m_running = false;
	std::mutex m_mutex;
};