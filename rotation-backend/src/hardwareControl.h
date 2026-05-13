/////////////////////////////////////////////////////////////////////////////
// Author:      Alexandre CARPENTIER
// Modified by:
// Created:     18/04/2026
// Copyright:   (c) Alexandre CARPENTIER
// Licence:     LGPL-2.1-or-later
/////////////////////////////////////////////////////////////////////////////
#pragma once
#include <cassert>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include <condition_variable>
#include "cppDaq.h"
#include "colors.h"

///////////////////////////////////////////////////////////////////////////////////////
//
// Hardware configuration for NI USB 6001 (must be edit)
//
// Wiring: NI USB-6001 <-> Lexitek R208 (DB9 connector)
// See hardware/doc.md for full wiring diagram.
//
//  NI USB-6001 Pin | Signal        | R208 DB9 Pin | Description
// -----------------|---------------|--------------|----------------------------
//  AI0             | HOME_PIN      | Pin 5        | Home position sensor (analog voltage)
//  AO0             | CLOCK_PIN     | Pin 2        | PWM step signal (motor speed)
//  P0.0            | MOVE_PIN      | Pin 4        | Motor enable/disable
//  P0.1            | DIR_PIN       | Pin 3        | Rotation direction
//  AGND / GND      | GND           | Pin 1        | Common ground
//

// inline to prevent multiple definitions
inline analog_pin HOME_PIN{ 0 };				// AI0  → R208 DB9 Pin 5 : Home position sensor
inline analog_pin_continuous CLOCK_PIN{ 0 };	// AO0  → R208 DB9 Pin 2 : PWM step signal (speed control)
inline digital_pin MOVE_PIN{ 0 };				// P0.0 → R208 DB9 Pin 4 : Motor enable (HIGH = active)
inline digital_pin DIR_PIN{ 1 };				// P0.1 → R208 DB9 Pin 3 : Direction (HIGH = CW, LOW = CCW)

///////////////////////////////////////////////////////////////////////////////////////
//
// Interface dependencies injection
//

class hardware_interface
{
public:
	virtual bool start(double freqHz)=0;
	virtual bool stop()=0;
	virtual bool resetDaq() = 0;
	virtual void acquires(double& voltage) = 0;
	virtual bool setSpeed(double freqHz) = 0;
	virtual bool doMove(bool isMoving) = 0;
	virtual void doClockwizeDirection(bool isClockWise) = 0;
	virtual bool checkDaq() = 0;
	virtual ~hardware_interface() = default; // Mandatory for childrens
};

///////////////////////////////////////////////////////////////////////////////////////
//
// Production code
//

class production_hardware : public hardware_interface
{
public:
	production_hardware()
	{
		std::lock_guard<std::mutex> lockInit(m_mutexAPI);
		std::print("[*] Initialize production_hardware\n");
		std::print("[*] Connecting to DAQ\n");

		DaqConfig config;

		config.setDevice("Dev1") // Auto connect to first device if empty. Default name is Dev1 if needed.
			.setAnalogPins(analog_pins{ HOME_PIN }) // To read home position
			.setAnalogContinuous(analog_pins_continuous{ CLOCK_PIN }) // To make a PWM signal
			.setDigitalPins(digital_pins{ MOVE_PIN, DIR_PIN }); // To control the direction and the movement

		m_daq = std::make_unique<cppDaq>(config);
		assert(m_daq); // Ensure DAQ is initialized correctly
	}

	bool start(double freqHz) override
	{
		std::lock_guard<std::mutex> lockInit(m_mutexAPI);
		std::print("[*] Starting hardware\n");
		m_freqHz = freqHz;	

		m_daq->WriteDigitalPin(DIR_PIN, digital_state::HIGH);  // TURN CLOCKWIZE
		m_daq->WriteDigitalPin(MOVE_PIN, digital_state::LOW); // MOVE ON
		m_running = true;
		return true;
		//return m_daq->StartAnalogPWM(CLOCK_PIN, Frequency_hz<double>(freqHz));
	}

	bool stop() override
	{
		std::lock_guard<std::mutex> lockInit(m_mutexAPI);
		std::print("[*] Stopping hardware\n");

		m_daq->WriteDigitalPin(DIR_PIN, digital_state::HIGH);  // TURN CLOCKWIZE
		m_daq->WriteDigitalPin(MOVE_PIN, digital_state::HIGH); // MOVE OFF
		m_running = false;
		m_daq->StopAnalogPWM(CLOCK_PIN);

		std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Give some time for the DAQ to reset
		return true;
	}

	virtual bool resetDaq() override
	{
		std::lock_guard<std::mutex> lockInit(m_mutexAPI);
		std::print("[*] Reseting hardware\n");
		return m_daq->resetDaq();
	}

	virtual void acquires(double& voltage)
	{
		std::lock_guard<std::mutex> lockInit(m_mutexAPI);
		voltage = m_daq->ReadAnalogPin(HOME_PIN);
	}

	virtual bool setSpeed(double freqHz) override
	{
		std::lock_guard<std::mutex> lockInit(m_mutexAPI);
		m_freqHz = freqHz;

		if (m_running)
		{
			m_daq->StopAnalogPWM(CLOCK_PIN);
		}
		std::print("[*] Setting speed to {} Hz\n", freqHz);
		return m_daq->StartAnalogPWM(CLOCK_PIN, Frequency_hz<double>(m_freqHz));
	}

	virtual bool doMove(bool isMoving) override
	{
		std::lock_guard<std::mutex> lockInit(m_mutexAPI);
		static bool s_state = false;
		if (isMoving != s_state) // Only update if state changed	
		{
			s_state = isMoving; // Save state
			
			//Act
			if (isMoving)
			{
				std::print(CMAGENTA"[*]doMove(true) called\n"); std::print(CRESET);	
				//m_daq->WriteDigitalPin(MOVE_PIN, digital_state::LOW); // Enable			
				return m_daq->StartAnalogPWM(CLOCK_PIN, Frequency_hz<double>(m_freqHz));
			}
			std::print(CMAGENTA"[*]doMove(false) called\n"); std::print(CRESET);
			//m_daq->WriteDigitalPin(MOVE_PIN, digital_state::HIGH); // Disable
			return m_daq->StopAnalogPWM(CLOCK_PIN);
		}
		return true;
	}

	virtual void doClockwizeDirection(bool isClockWise) override
	{
		std::lock_guard<std::mutex> lockInit(m_mutexAPI);
		static bool s_lastDirection = true;
		if (isClockWise != s_lastDirection)
		{
			s_lastDirection = isClockWise;

			std::print(CMAGENTA"[*] clockWize = {}\n", isClockWise); std::print(CRESET);

			if (isClockWise == true)
			{
				std::print(CMAGENTA"[*] WRITE HIGH\n"); std::print(CRESET);
				m_daq->WriteDigitalPin(DIR_PIN, digital_state::HIGH);// turn anticlockwise
			}
			else if(isClockWise == false)
			{
				std::print(CMAGENTA"[*] WRITE LOW\n"); std::print(CRESET);
				m_daq->WriteDigitalPin(DIR_PIN, digital_state::LOW);// turn clockwise
			}
		}
	}

	virtual bool checkDaq() override
	{
		std::lock_guard<std::mutex> lockInit(m_mutexAPI);
		bool isOk = true;
		if (m_daq->GetStatus() != DAQ_STATUS::NO_ERR)
		{
			isOk = false;
		}
		return isOk;
	}
private:
	std::unique_ptr<cppDaq> m_daq = nullptr;
	double m_freqHz {0.0};
	bool m_running = false;

	std::mutex m_mutexAPI;
};

///////////////////////////////////////////////////////////////////////////////////////
//
// Mock code
//
#include "timerControl.h"
class mock_hardware : public hardware_interface
{
	std::mutex m_mutexMovement;


	std::condition_variable m_conditionStartMove;

	std::atomic_bool m_isAtHome;
	std::atomic_bool m_isMoving;
	std::atomic_bool m_isClockWise = true;

	std::atomic<double> m_currentAngle = 0.0;

	std::jthread m_HomeThread;

	void simulateHoming(const double& speed)
	{
		m_HomeThread = std::jthread([this, speed](std::stop_token stopToken)
			{
				m_isAtHome = false;

				while (!stopToken.stop_requested())
				{
					// Lock until start is triggered
					std::unique_lock<std::mutex> movementLock(m_mutexMovement);
					m_conditionStartMove.wait(movementLock, [&] { return m_isMoving.load() || stopToken.stop_requested(); });

					if (stopToken.stop_requested()) break;

					// Moving started - calculate position using delta time
					auto lastTime = std::chrono::steady_clock::now();
					while (m_isMoving)
					{
						movementLock.unlock();
						std::this_thread::sleep_for(std::chrono::milliseconds(50));
						movementLock.lock();

						auto now = std::chrono::steady_clock::now();
						double deltaSeconds = std::chrono::duration<double>(now - lastTime).count();
						lastTime = now;

						double dirSign = m_isClockWise ? +1.0 : -1.0;
						m_currentAngle = std::fmod(m_currentAngle + dirSign * speed * deltaSeconds + 360.0, 360.0);

						std::print(CGREEN"[*] Simulated angle: {:.2f} deg\n", m_currentAngle.load()); std::print(CRESET);

						if (m_currentAngle < 1.0 || m_currentAngle > 359.0)
						{
							m_isAtHome = true;
							movementLock.unlock();
							std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Simulate home sensor pulse
							movementLock.lock();
							m_isAtHome = false;
						}
					}
				}
			});
	}
public:
	mock_hardware()
	{
		m_currentAngle = 0.0;		
		m_isAtHome = false;
		m_isMoving = false;
		m_isClockWise = true;
	}

	virtual bool start(double freqHz) override
	{
		std::print(CMAGENTA"[*] SIMULATE start()\n"); std::print(CRESET);
		const double simSpeed{28.0};
		simulateHoming(simSpeed);
		return true;
	}

	virtual bool stop() override
	{
		// Close the thread properly
		m_HomeThread.request_stop();  
		m_conditionStartMove.notify_all(); 

		std::print(CMAGENTA"[*] SIMULATE stop()\n"); std::print(CRESET);
		return true;
	}

	virtual bool resetDaq() override
	{
		return true;
	}


	virtual void acquires(double& voltage) override
	{
		if (m_isAtHome)
		{
			std::print(CMAGENTA"[*] SIMULATE acquires() 0.12V - home detected\n"); std::print(CRESET);
			voltage = 0.12;
		}
		else
		{
			voltage = 5.03;
		}
		return;
	}

	virtual bool setSpeed(double freqHz) override
	{
		std::print(CMAGENTA"[*] SIMULATE setSpeed()\n"); std::print(CRESET);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		return true;
	}

	virtual bool doMove(bool isMoving) override
	{
		if (isMoving && !m_isMoving)
		{
			std::print(CMAGENTA"[*] SIMULATE doMove() start moving\n"); std::print(CRESET);
			m_isMoving = true;
			m_conditionStartMove.notify_all();
		}
		else if (!isMoving && m_isMoving)
		{
			std::print(CMAGENTA"[*] SIMULATE doMove() stop moving\n"); std::print(CRESET);
			m_isMoving = false;
		}
		return true;
	}

	virtual void doClockwizeDirection(bool isClockWise) override
	{
		m_isClockWise = isClockWise;
		if (isClockWise)
		{
			std::print(CMAGENTA"[*] SIMULATE doClockwizeDirection() - Clockwise\n"); std::print(CRESET);
		}
		else
		{
			std::print(CMAGENTA"[*] SIMULATE doClockwizeDirection() - Counter-Clockwise\n"); std::print(CRESET);
		}
	}

	virtual bool checkDaq() override
	{
		return true;
	}
};