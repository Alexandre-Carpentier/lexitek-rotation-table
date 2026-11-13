/////////////////////////////////////////////////////////////////////////////
// Author:      Alexandre CARPENTIER
// Modified by:
// Created:     03/11/2026
// Copyright:   (c) Alexandre CARPENTIER
// Licence:     LGPL-2.1-or-later
/////////////////////////////////////////////////////////////////////////////
#pragma	once

#include <print>
#include <memory>
#include <thread>
#include <format>
#include <atomic>

/////////////////////////////////////////////////////////////////////////////
// 
// Dependencies (components) of the project
//

#include "colors.h"
#include "backend_rpc.h"
#include "hardwareControl.h"
#include "timerControl.h"
#include "fileControl.h"
#include "automationControl.h"

///////////////////////////////////////////////////////////////////////////////////////
//
// OPAQUE implementation for time based positionning
//

class lexitek::rotationImpl::rotationClockBasedImpl final : public lexitek::rotationImpl
{
public:
	// Inject all dependencies in the constructor. Caller can choose to pass production or mock dependencies for testing.
	explicit rotationClockBasedImpl(lexitek& parent, hardware_interface& hw, counter_interface& t1, counter_interface& t2, file_interface& f, automation_interface& script)
		: m_parent(parent), m_hardware(hw), m_counter_move(t1), m_counter_cal(t2), m_file(f), m_script(script)
	{
		std::lock_guard<std::mutex> lockApi(m_mutexApi);
		init();
		std::print("[*] Backend initialized successfully\n");
	};

	~rotationClockBasedImpl() override {
		std::lock_guard<std::mutex> lockApi(m_mutexApi);
		std::print("[*]  ~rotationClockBasedImpl() called");
		if (m_state != STATE::STOPPED)
		{
			std::print("[*] Stopping polling loop\n");
			stop();
		}

		std::print("[*] Stopping backend polling thread\n");
		if (m_pollingThread.joinable()) {
			m_pollingThread.request_stop();
		}

		std::print("[*] Delete done\n");
	};

	bool start(frequency_t freqHz) override
	{
		std::print("[*] Starting polling operation\n");
		std::lock_guard<std::mutex> lockApi(m_mutexApi);
		if (!m_hardware.checkDaq())
		{
			std::print("[!] Running failed\n");
			return false;
		}

		m_freqHz = freqHz;
		m_hardware.start(m_freqHz.get());

		std::print("[*] Running\n");
		m_state = STATE::RUNNING;
		return true;
	}

	bool stop() override
	{
		std::lock_guard<std::mutex> lockApi(m_mutexApi);
		resetData();

		std::print(CYELLOW"[*] Stopping hardware\n"); std::print(CRESET);
		m_hardware.stop();
		std::print(CYELLOW"[*] Reseting hardware\n"); std::print(CRESET);
		m_hardware.resetDaq();
		m_state = STATE::STOPPED;

		return true;
	}

	bool doCalibrating() override
	{
		std::print("[*] Calibrating\n");
		std::lock_guard<std::mutex> lockApi(m_mutexApi);
		m_rotationData.isCalibrationRequested = true;
		m_state = STATE::CALIBRATING;
		return true;
	}

	bool doHoming() override
	{
		std::print("[*] Homing\n");
		std::lock_guard<std::mutex> lockApi(m_mutexApi);
		m_state = STATE::HOMING;
		return true;
	}

	// [[Deprecated]] Blocking calls occurs than can timeout
	// Read current position
	bool readAngle(angle_t& angle) override
	{
		std::lock_guard<std::mutex> lockApi(m_mutexApi);
		if (m_state == STATE::RUNNING || m_state == STATE::HOMING)
		{
			angle = m_rotationData.currentAngle;
			return true;
		}
		angle = 0.0;
		return false;
	}

	//bool readAngle(angle_t& angle)
	//{
	//	angle = angle_t{
	//		m_cachedAngle.load(std::memory_order_relaxed)
	//	};
	//	return true;
	//}

	// Set target position
	bool setAngle(double angle)
	{
		std::lock_guard<std::mutex> lockApi(m_mutexApi);
		m_rotationData.targetAngle = angle;
		m_rotationData.isTargetUpdated = true;

		// Compute direction once, from stable current position, and lock it
		double angularDiff = std::fmod(m_rotationData.targetAngle.get() - m_rotationData.currentAngle.get() + 540.0, 360.0) - 180.0;
		m_rotationData.directionSign = (angularDiff >= 0.0) ? +1.0 : -1.0;
		m_rotationData.remainingAngle = std::abs(angularDiff);
		return true;
	}

private:
	void init()
	{
		assert(&m_parent != nullptr);
		std::print("[*] Running rotationClockBasedImpl\n");

		resetData();

		std::print("[*] Start the polling thread\n");
		m_pollingThread = std::jthread([this](std::stop_token st) {
			poll(st);
			});
		assert(m_pollingThread.joinable());// Ensure the polling thread is running correctly
	}

	void updateStates()
	{
		// Update currentAngle state
		static bool s_counterStarted = false;
		// Update isHome state
		static size_t s_firstTrig = 0;
		// Update isMoving state
		static angle_t s_previousAngle{ 0.0 };

		////////////////////////////////////////////////
		// Update homeVoltage
		double val;
		m_hardware.acquires(val);
		m_rotationData.homeVoltage = val;

		////////////////////////////////////////////////
		// Update isHome state
		if (m_rotationData.homeVoltage.get() < 1.5) { // Assuming 1.5V as threshold for being at home
			if (s_firstTrig == 0) {

				std::print("\n[*] Home position detected\n");
				m_rotationData.isHome = true;
				// Clockwise
				if (m_rotationData.directionSign >= 0)
				{
					std::lock_guard<std::mutex> lockApi(m_mutexApi);
					m_rotationData.currentAngle = 0.0; // Reset angle to 0 when at home
					m_rotationData.initialAngle = 0.0;
				}
				else
				{
					std::lock_guard<std::mutex> lockApi(m_mutexApi);
					m_rotationData.currentAngle = 359.9; // Reset angle to 359.9 when at home
					m_rotationData.initialAngle = 359.9;
				}

				m_counter_move.stop();
				m_counter_move.reset();
				s_firstTrig = 1;
			}
		}
		else if (m_rotationData.homeVoltage.get() > 3.5) {
			std::lock_guard<std::mutex> lockHome(m_mutexHome);
			m_rotationData.isHome = false;
			s_firstTrig = 0;
		}

		////////////////////////////////////////////////
		// Update directionSign state

		// Compute signed shortest angular difference, normalized to [-180, 180]
		//double angularDiff = std::fmod(m_rotationData.targetAngle.get() - m_rotationData.currentAngle.get() + 540.0, 360.0) - 180.0;
		//m_rotationData.remainingAngle = std::abs(angularDiff);
		//m_rotationData.directionSign = (angularDiff >= 0.0) ? +1.0 : -1.0;

		double angularDiff = std::fmod(m_rotationData.targetAngle.get() - m_rotationData.currentAngle.get() + 540.0, 360.0) - 180.0;
		m_rotationData.remainingAngle = std::abs(angularDiff);

		////////////////////////////////////////////////
		// Update currentAngle state
		if (m_rotationData.isComputingPositionRequested)
		{
			// If target changed when already moving, reset timer and initial angle to avoid position error
			if (m_rotationData.isTargetUpdated == true)
			{
				s_counterStarted = false; // reset computing
				m_rotationData.isTargetUpdated = false;
			}

			if (!s_counterStarted)
			{
				m_counter_move.start();
				s_counterStarted = true;
				std::lock_guard<std::mutex> lockApi(m_mutexApi);
				m_rotationData.initialAngle = m_rotationData.currentAngle;
			}

			// Get clock counter value
			{
				std::lock_guard<std::mutex> lockApi(m_mutexApi);
				m_rotationData.counterValue = m_counter_move.get();
			}
			
			// Clockwise rotation
			if (m_rotationData.directionSign >= 0)
			{
				std::lock_guard<std::mutex> lockApi(m_mutexApi);
				m_rotationData.currentAngle = ((m_rotationData.counterValue * 360.0) / m_rotationData.velocityClkPerTurn.get()) + m_rotationData.initialAngle.get();

			}
			// Anti-clockwise rotation
			if (m_rotationData.directionSign < 0)
			{
				std::lock_guard<std::mutex> lockApi(m_mutexApi);
				m_rotationData.currentAngle = m_rotationData.initialAngle.get() - ((m_rotationData.counterValue * 360.0) / m_rotationData.velocityClkPerTurn.get());
			}

			// Sanitize
			if(std::isnan(m_rotationData.currentAngle.get()))
			{
				std::print("[*] Warning: currentAngle is NaN, resetting to 0.0\n");
				m_rotationData.currentAngle = 0.0;
			}
		}
		else
		{
			m_counter_move.stop();
			s_counterStarted = false;
		}

		////////////////////////////////////////////////
		// Update isMoving state
		if (s_previousAngle.get() != m_rotationData.currentAngle.get()) {
			std::lock_guard<std::mutex> lockApi(m_mutexApi);
			m_rotationData.isMoving = true;
			s_previousAngle = m_rotationData.currentAngle;
		}
		else {
			m_rotationData.isMoving = false;
		}

		////////////////////////////////////////////////
		// Update isAtTarget state
		{
			double diff = std::fmod(std::abs(m_rotationData.targetAngle.get() - m_rotationData.currentAngle.get()), 360.0);
			if (diff > 180.0) diff = 360.0 - diff;
			std::lock_guard<std::mutex> lockApi(m_mutexApi);
			m_rotationData.isAtTarget = static_cast<bool>(diff <= g_accuracy);
		}
	}

	void onStop()
	{
		std::lock_guard<std::mutex> lockApi(m_mutexApi);
		if (m_state == STATE::STOPPED)
		{
			// TODO: Reset every states
			static STATE s_previousState = STATE::RUNNING;
			if (s_previousState != m_state)
			{
				s_previousState = m_state;
				m_hardware.doMove(false); // Stop hardware
			}
		}
	}

	void onRun()
	{
		std::lock_guard<std::mutex> lockApi(m_mutexApi);
		if (m_rotationData.isCalibrated == true)
		{
			static bool s_clockDirectionLock = false;
			static bool s_anticlockDirectionLock = false;

			////////////////////////////////////////////////
			// Calculate the shorter direction
			if (m_rotationData.directionSign >= 0)
			{
				// Rotate clockwise
				if (s_clockDirectionLock == false)
				{
					s_clockDirectionLock = true;
					s_anticlockDirectionLock = false;
					m_hardware.doClockwizeDirection(true);
					m_counter_move.reset(); // Reset counter 
					dbg::print(CYELLOW"[*] Rotating clockwise\n"); std::print(CRESET);
				}
			}
			else
			{
				// Rotate counter-clockwise
				if (s_anticlockDirectionLock == false)
				{
					s_clockDirectionLock = false;
					s_anticlockDirectionLock = true;
					m_hardware.doClockwizeDirection(false);
					m_counter_move.reset(); // Reset counter
					dbg::print(CYELLOW"[*] Rotating counter-clockwise\n"); std::print(CRESET);
				}
			}

			////////////////////////////////////////////////
			// Move when actual angle != target angle
			if (!m_rotationData.isAtTarget)
			{
				std::print("[*] Counter is: \"{}\".\n", m_counter_move.get());
				// Go to target
				m_rotationData.isComputingPositionRequested = true;
				m_hardware.doMove(true);
				m_counter_move.tick(); // Start counting clock generated each time			
			}
			else
			{
				s_clockDirectionLock = false;
				s_anticlockDirectionLock = false;
				m_hardware.doMove(false);
				m_counter_move.reset(); // Reset counter when stopped
				m_rotationData.isComputingPositionRequested = false;
			}
		}
	}

	void onHome()
	{
		std::lock_guard<std::mutex> lockApi(m_mutexApi);
		if (m_state == STATE::HOMING)
		{
			// TODO: handle timeout
			static bool homeStep = false;

			// Go to home position
			m_hardware.doMove(true);

			// Step1: Find home
			if (m_rotationData.isHome && homeStep == false)
			{
				homeStep = true;
				// Stop a bit
				m_hardware.doMove(false);
				m_counter_move.reset(); // Reset counter when stopped
				std::this_thread::sleep_for(std::chrono::milliseconds(500));

				// Flag current position = target = 0
				m_rotationData.currentAngle = 0.0;
				m_rotationData.targetAngle = 0.0;
				homeStep = false;
				m_state = STATE::RUNNING;
			}
		}
	}

	void onCalibrate()
	{
		if (m_state == STATE::CALIBRATING)
		{
			// TODO: handle timeout

			static bool s_init;
			static bool s_home;
			static bool s_outHome;
			static bool s_inc;
			static bool s_calc;

			// Define step order if already at home
			static bool s_runonce = false;
			if (s_runonce == false)
			{
				if (m_rotationData.isHome == true)
				{
					s_init = false;
					s_home = true; //skip
					s_outHome = false; //begin
					s_inc = true;
					s_calc = true;
				}
				else
				{
					s_init = false;
					s_home = false;//begin
					s_outHome = true;
					s_inc = true;
					s_calc = true;
				
				}
				s_runonce = true;
			}

			// init runonce
			if (s_init == false)
			{
				m_counter_cal.start(); // Reset counter when stopped
				s_init = true;
			}

			// Go to home position
			if (s_home == false)
			{
				std::print(CCYAN"[*] Goto home position\n"); std::print(CRESET);
				{
					std::lock_guard<std::mutex> lockApi(m_mutexApi);
					m_hardware.doMove(true);
				}
				
				m_counter_cal.tick(); //inc

				if (m_rotationData.isHome == true) // Step0 == OK
				{
					std::lock_guard<std::mutex> lockApi(m_mutexApi);
					m_hardware.doMove(false);
					s_home = true;
					s_outHome = false; // next
				}
			}

			// get out of home position
			if (s_outHome == false)
			{
				if (m_rotationData.isHome == true)
				{
					std::lock_guard<std::mutex> lockApi(m_mutexApi);
					m_hardware.doMove(true);
				}
				else
				{
					std::lock_guard<std::mutex> lockApi(m_mutexApi);
					for (int i = 0; i < 10; ++i) // Move a bit to get out of home position
					{
						m_hardware.doMove(true);
						std::this_thread::sleep_for(std::chrono::milliseconds(100));
					}
					m_hardware.doMove(false);
					s_outHome = true;
					s_inc = false; // next
				}
			}

			// inc counter until one turn is reached
			if (s_inc == false)
			{
				std::print(CCYAN"[*] INC counter :\"{}\"\n", m_counter_cal.get()); std::print(CRESET);
				{
					std::lock_guard<std::mutex> lockApi(m_mutexApi);
					m_hardware.doMove(true);
				}

				m_counter_cal.tick(); //inc

				if (m_rotationData.isHome == true) // Step0 == OK
				{
					std::lock_guard<std::mutex> lockApi(m_mutexApi);
					m_hardware.doMove(false);
					s_inc = true;
					s_calc = false; // next
				}
			}

			// Calculate

			if (s_calc == false)
			{
 			m_rotationData.velocityClkPerTurn = m_counter_cal.get();

				std::print(CGREEN"[*] Calibration done. Velocity computed: \"{}\" clock per turns.\n", 
					m_rotationData.velocityClkPerTurn.get());
				std::print(CRESET);

				// Save to disk
				try
				{
					writeCal(std::abs(m_rotationData.velocityClkPerTurn.get()));
				}
				catch (std::exception& e)
				{
					std::print(CRED"[*] Error saving calibration data: {}\n", e.what());
					std::print(CRESET);
				}

				// Reset state for next calibration
				m_counter_cal.stop(); 
				s_runonce = false;
				s_init = false;
				s_home = false;
				s_outHome = false;
				s_inc = false;
				s_calc = false;

				// Flag current position = target = 0
				m_rotationData.currentAngle = 0.0;
				m_rotationData.targetAngle = 0.0;

				// Flag calibrated
				m_rotationData.isCalibrationRequested = false;
				m_rotationData.isCalibrated = true;
				m_state = STATE::RUNNING;
			}
		}
	}

	void poll(std::stop_token stopToken)
	{
		if (std::isnan(m_rotationData.velocityClkPerTurn.get()))
		{
			std::print(CRED"[*] Warning: velocityClkPerTurn is NaN, resetting to 20000\n"); std::print(CRESET);
			m_rotationData.velocityClkPerTurn = 20000;
		}

		while (!stopToken.stop_requested())
		{
			// Check daq status
			if (!m_hardware.checkDaq())
			{
				m_state = STATE::STOPPED;
				break;
			}
			updateStates();

			switch (m_state)
			{
			case STATE::STOPPED:
				dbg::print(CRED"[*] State: STOPPED\r"); dbg::print(CRESET);
				onStop();
				break;

			case STATE::RUNNING:
				dbg::print(CGREEN"[*] State: RUNNING\n"); dbg::print(CRESET);
				onRun();
				break;

			case STATE::HOMING:
				dbg::print(CCYAN"[*] State: HOMING\n"); dbg::print(CRESET);
				onHome();
				break;

			case STATE::CALIBRATING:

				dbg::print(CCYAN"[*] State: CALIBRATING\n"); dbg::print(CRESET);
				onCalibrate();
				break;
			}
		}
		std::print("[*] Polling loop thread exiting\n");
	}

	void checkHomeRise(size_t& turns)
	{
		//Lock
		std::lock_guard<std::mutex> lockHome(m_mutexHome);

		if ((home == 0) and (m_rotationData.isHome == true))
		{
			home = 1;
			++turns;
			std::print("[*] Home position detected, turns: {}\n", turns);
		}

		// Reset
		if (!m_rotationData.isHome)
		{
			home = 0;
		}
	}

	void writeCal(double velocity)
	{
		m_file.clearContentFile(); // Overwrite: erase previous calibration before writing
		std::string strDouble = std::format("VELOCITY::{:.3f}\n", velocity);
		std::print("[*] Saving calibration data: {}\n", strDouble);
		m_file.addToFile(strDouble);
	}

	void readCal(std::string& calData, double& velocity)
	{
		m_file.readFromFile(calData);
		if (calData.size() > 0)
		{
			calData = calData.substr(calData.find("::") + 2);
			std::print("[*] Calibration data loaded: {}\n", calData);
			try
			{
				velocity = std::stod(calData);
				std::print("[*] Calibration value: "); std::print(CGREEN"{}\n", velocity); std::print(CRESET);
				m_rotationData.isCalibrated = true;
			}
			catch (const std::exception&)
			{
				std::print(CRED"[*] Error parsing calibration data\n"); std::print(CRESET);
			}
		}
		else
		{
			std::print("[*] No calibration data found\n");
		}
	}

	void resetData()
	{
		// Read calibration data
		std::lock_guard<std::mutex> lockHome(m_mutexHome);
		m_rotationData.isCalibrated = false;
		std::string clockCal = "clockCal.txt";
		m_file.selectFile(clockCal);
		std::string calData;
		double calVelocity = 20000;
		readCal(calData, calVelocity);

		try
		{
			m_rotationData.velocityClkPerTurn = calVelocity;
			m_rotationData.isCalibrated = true;

			std::print(
				CGREEN"[*] Calibration found at \"{}\". Velocity read: \"{}\" clk per turn.\n",
				clockCal,
				m_rotationData.velocityClkPerTurn.get()
			);

			std::print(CRESET);
		}
		catch(const std::exception & e)
		{
			m_rotationData.isCalibrated = false;

			std::print(CRED"[*] Error reading calibration data: {}\n", e.what());
			std::print(CRESET);
		}

		m_rotationData.directionSign = 1.0;

		m_rotationData.currentAngle = 0.0;
		m_rotationData.targetAngle = g_accuracy * 2; // To not oscillate
		m_rotationData.initialAngle = 0.0;
		m_rotationData.destinationDistance = 0.0;
		m_rotationData.remainingAngle = 0.0;

		m_rotationData.isComputingPositionRequested = false;
		m_rotationData.isMoving = false;
		m_rotationData.isAtTarget = false;
		m_rotationData.isHome = false;
		m_rotationData.isCalibrationRequested = false;
	}

	// This is external dependencies
	lexitek& m_parent;
	hardware_interface& m_hardware;
	counter_interface& m_counter_cal;
	counter_interface& m_counter_move;
	file_interface& m_file;
	automation_interface& m_script;

	// datas
	size_t home;
	std::jthread m_pollingThread;

	STATE m_state = STATE::STOPPED;
	ROTATIONSTRUCT m_rotationData{};
	//std::atomic<double> m_cachedAngle{ 0.0 }; // Used for instantaneous reading from grpc
	frequency_t m_freqHz;

	std::mutex m_mutexApi;
	std::mutex m_mutexHome;;
};
