/////////////////////////////////////////////////////////////////////////////
// Author:      Alexandre CARPENTIER
// Modified by:
// Created:     06/04/2026
// Copyright:   (c) Alexandre CARPENTIER
// Licence:     LGPL-2.1-or-later
/////////////////////////////////////////////////////////////////////////////
#include "rotation.h"

#include <print>
#include <memory>
#include <thread>
#include <format>

const double g_accuracy = 0.5; // Accuracy in degree for considering atTarget = true
static std::string calFilename = "cal.txt"; // Calibration file name


/////////////////////////////////////////////////////////////////////////////
// 
// Custom print function for debugging.
//

//#define __VERBOSE // Uncomment to print detailed states in the console for debugging (can reduce performance as std::print is slow)
namespace dbg
{
	template<typename... Args>
	void print(std::format_string<Args...> fmt, Args&&... args)
	{
#ifdef __VERBOSE
		std::print("[DEBUG] ");
		std::print(fmt, std::forward<Args>(args)...);
#endif
	}
}

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

void computeVelocity(double& velocity, double elapsedSeconds);



/////////////////////////////////////////////////////////////////////////////
// 
// define: Voltage in volt
//

struct volt_t
{
	constexpr volt_t() = default;

	template<typename T> requires std::same_as<T, double>
	explicit constexpr volt_t(T value) : m_value(value) {
		if (m_value < 0)m_value = 0.0; if (m_value >= m_maxVoltage)m_value = m_maxVoltage;
	}

	constexpr volt_t& operator= (double value) 
	{ 
		// Sanity check for voltage
		if (value < 0)value = 0.0; if (value >= m_maxVoltage)value = m_maxVoltage;
		m_value = value; return *this; 
	}

	[[nodiscard]] constexpr double get() const { return m_value; }
private:
	double m_value{};
	static constexpr double m_maxVoltage = 48.0; // Max voltage in volt
};

/////////////////////////////////////////////////////////////////////////////
// 
// define: Velocity in deg/sec
//

struct velocity_t
{
	constexpr velocity_t() = default;

	template<typename T> requires std::same_as<T, double>
	explicit constexpr velocity_t(T value) : m_value(value) {
		if (m_value < m_minVelocity)m_value = m_minVelocity; if (m_value >= m_maxVelocity)m_value = m_maxVelocity;
	}

	constexpr velocity_t& operator= (double value) 
	{ 
		// Sanity check for velocity
		if (value < m_minVelocity)value = m_minVelocity; if (value >= m_maxVelocity)value = m_maxVelocity;
		m_value = value; return *this; 
	}

	[[nodiscard]] constexpr double get() const { return m_value; }
private:
	double m_value{};
	static constexpr double m_maxVelocity = 48.0; // Max voltage in volt
	static constexpr double m_minVelocity = -48.0; // Min voltage in volt
};

///////////////////////////////////////////////////////////////////////////////////////
//
// State of the rotation table
//

enum class STATE {
	RUNNING = 0,
	HOMING,
	CALIBRATING,
	STOPPED
};

///////////////////////////////////////////////////////////////////////////////////////
//
// Rotation table data
//

struct ROTATIONSTRUCT {
	angle_t currentAngle;
	angle_t targetAngle;
	double directionSign;
	angle_t initialAngle;
	angle_t remainingAngle;
	angle_t oppositeAngle;
	angle_t destinationDistance;
	velocity_t velocityDegPerSec;
	volt_t homeVoltage;
	double duration;
	bool isComputingPositionRequested;
	bool isMoving;
	bool isTargetUpdated;
	bool isAtTarget;
	bool isHome;
	bool isCalibrationRequested;
	bool isCalibrated;
};

///////////////////////////////////////////////////////////////////////////////////////
//
// OPAQUE implementation of the rotation::driver_r208 class external dependencies
//

struct rotation::driver_r208::dependenciesimpl
{ 
	production_hardware hardware; // Real hardware for prod
	production_timer    timer1;
	production_timer    timer2;
	production_file     file;
	production_automation script;
};

///////////////////////////////////////////////////////////////////////////////////////
//
// OPAQUE implementation of the rotation::driver_r208 class
//

class rotation::driver_r208::rotationimpl
{
public:
	// Inject all dependencies in the constructor. Caller can choose to pass production or mock dependencies for testing.
	explicit rotationimpl(rotation::driver_r208& parent, hardware_interface& hw, timer_interface& t1, timer_interface& t2, file_interface& f, automation_interface& script)
		: m_parent(parent), m_hardware(hw), m_timer_move(t1), m_timer_cal(t2), m_file(f), m_script(script)
	{
		std::lock_guard<std::mutex> lockApi(m_mutexApi);
		init();
		std::print("[*] Backend initialized successfully\n");
	};

	~rotationimpl() {
		std::lock_guard<std::mutex> lockApi(m_mutexApi);
		std::print("[*]  ~rotationimpl() called");
		if (m_state != STATE::STOPPED)
		{
			std::print("[*] Stopping polling loop\n");
			stop();
		}

		//std::print("[*] Stopping lua automation\n");

		std::print("[*] Stopping backend polling thread\n");
		if (m_pollingThread.joinable()) {
			m_pollingThread.request_stop();
		}

		std::print("[*] Stopping backend RPC server\n");
		if (m_serverThread.joinable()) {
			m_serverThread.request_stop();
		}
		std::print("[*] Delete done\n");
	};

	bool start(frequency_t freqHz)
	{
		std::lock_guard<std::mutex> lockApi(m_mutexApi);
		std::print("[*] Starting polling operation\n");
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

	bool stop()
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

	bool doCalibrating()
	{
		std::lock_guard<std::mutex> lockApi(m_mutexApi);
		std::print("[*] Calibrating\n");
		m_rotationData.isCalibrationRequested = true;
		m_state = STATE::CALIBRATING;
		return true;
	}

	bool doHoming()
	{
		std::lock_guard<std::mutex> lockApi(m_mutexApi);
		std::print("[*] Homing\n");
		m_state = STATE::HOMING;
		return true;
	}

	// Read current position
	bool readAngle(angle_t& angle)
	{
		std::lock_guard<std::mutex> lockApi(m_mutexApi);
		if(m_state == STATE::RUNNING || m_state == STATE::HOMING)
		{
			angle = m_rotationData.currentAngle;
			return true;
		}
		angle = 0.0;
		return false;	
	}

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
		std::print("[*] Running rotationimpl\n");

		resetData();

		std::print("[*] Starting server thread\n");
		m_serverThread = std::jthread([this](std::stop_token st) {
			backend_rpc_init(m_parent, st);
			});
		assert(m_serverThread.joinable());// Ensure the server thread is running correctly

		std::print("[*] Start the polling thread\n");
		m_pollingThread = std::jthread([this](std::stop_token st) {
			poll(st);
		});
		assert(m_pollingThread.joinable());// Ensure the polling thread is running correctly
	}

	void updateStates()
	{
		std::lock_guard<std::mutex> lockApi(m_mutexApi);
		// Update currentAngle state
		static bool s_timerStarted = false;
		// Update isHome state
		static size_t s_firstTrig = 0;
		// Update isMoving state
		static angle_t s_previousAngle { 0.0};

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
				// Clockwize
				if (m_rotationData.directionSign>= 0)
				{
					m_rotationData.currentAngle = 0.0; // Reset angle to 0 when at home
					m_rotationData.initialAngle = 0.0;
				}
				else
				{
					m_rotationData.currentAngle = 359.9; // Reset angle to 360 when at home
					m_rotationData.initialAngle = 359.9;
				}

				m_timer_move.stop();
				m_timer_move.reset();
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
				s_timerStarted = false; // reset computing
				m_rotationData.isTargetUpdated = false;
			}

			if (!s_timerStarted)
			{
				m_timer_move.start();
				s_timerStarted = true;
				m_rotationData.initialAngle = m_rotationData.currentAngle;
			}

			m_timer_move.tick();
			m_rotationData.duration = m_timer_move.elapsed_seconds();

			// Clockwise rotation
			if (m_rotationData.directionSign >= 0)
			{
				// d = vt + d0
				m_rotationData.currentAngle = (m_rotationData.duration * m_rotationData.velocityDegPerSec.get()) + m_rotationData.initialAngle.get();
			}
			// Anti-clockwise rotation
			if (m_rotationData.directionSign < 0)
			{
				// d = d0 - vt 
				m_rotationData.currentAngle = m_rotationData.initialAngle.get() - (m_rotationData.duration * m_rotationData.velocityDegPerSec.get());
			}
		}
		else
		{
			m_timer_move.stop();
			s_timerStarted = false;
		}


		////////////////////////////////////////////////
		// Update isMoving state
		if (s_previousAngle.get() != m_rotationData.currentAngle.get()) {
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
			m_rotationData.isAtTarget = (diff <= g_accuracy);
		}

		////////////////////////////////////////////////
		// Print states
		dbg::print("[*] directionSign: {:.1f}\t", m_rotationData.directionSign); dbg::print(CRESET);
		dbg::print(CGREEN"[*] Current angle : {:.1f}\t", m_rotationData.currentAngle.get()); dbg::print(CRESET);
		dbg::print(CRED"[*] target angle: {:.1f}\t", m_rotationData.targetAngle.get()); dbg::print(CRESET);
		dbg::print(CYELLOW"[*] initial angle: {:.1f}\n", m_rotationData.initialAngle.get()); dbg::print(CRESET);
		dbg::print(CYELLOW"[*] remaining angle: {:.1f}\n", m_rotationData.remainingAngle.get()); dbg::print(CRESET);
		dbg::print(CYELLOW"[*] opposite angle: {:.1f}\n", m_rotationData.oppositeAngle.get()); dbg::print(CRESET);
		dbg::print(CYELLOW"[*] destination distance: {:.1f}\n", m_rotationData.destinationDistance.get()); dbg::print(CRESET);
		dbg::print(CYELLOW"[*] velocity: {:.1f}\n", m_rotationData.velocityDegPerSec.get()); dbg::print(CRESET);
		dbg::print(CYELLOW"[*] homeVoltage {:.1f}V ", m_rotationData.homeVoltage.get()); dbg::print(CRESET);
		dbg::print(CYELLOW"[*] isComputingPositionRequested {} ", m_rotationData.isComputingPositionRequested); dbg::print(CRESET);
		dbg::print(CYELLOW"[*] isMoving {} ", m_rotationData.isMoving); dbg::print(CRESET);
		dbg::print(CYELLOW"[*] isTargetUpdated {} ", m_rotationData.isTargetUpdated); dbg::print(CRESET);
		dbg::print(CRED"[*] isAtTarget {} ", m_rotationData.isAtTarget); dbg::print(CRESET);
		dbg::print(CYELLOW"[*] isHome {} ", m_rotationData.isHome); dbg::print(CRESET);
		dbg::print(CYELLOW"[*] isCalibrationRequested {} ", m_rotationData.isCalibrationRequested); dbg::print(CRESET);
		dbg::print(CYELLOW"[*] isCalibrated {}\n", m_rotationData.isCalibrated); dbg::print(CRESET);
		dbg::print(CGREEN"[*] Current duration (sec): {:.1f}\n", m_rotationData.duration); dbg::print(CRESET);
	}

	void onStop()
	{
		std::lock_guard<std::mutex> lockApi(m_mutexApi);
		if (m_state == STATE::STOPPED)
		{
			// TODO: Reset every states
			m_hardware.doMove(false); // Stop hardware
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
					 dbg::print(CYELLOW"[*] Rotating counter-clockwise\n"); std::print(CRESET);
					}
				}

				////////////////////////////////////////////////
				// Move when actual angle != target angle
				if (!m_rotationData.isAtTarget)
				{
					// Go to target
					m_rotationData.isComputingPositionRequested = true;
					m_hardware.doMove(true);
				}
				else
				{
					s_clockDirectionLock = false;
					s_anticlockDirectionLock = false;
					m_hardware.doMove(false);
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
		std::lock_guard<std::mutex> lockApi(m_mutexApi);
		if (m_state == STATE::CALIBRATING)
		{
			// TODO: handle timeout
			static bool s_step0 = false;
			static bool s_step1 = false;
			static bool s_step2 = false;

			// Go to home position
			if (s_step0 == false)
			{
				std::print(CCYAN"[*] STEP0\n"); std::print(CRESET);
				s_step0 = true;
				m_hardware.doMove(true);
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
			}

			// Step1: Find home
			if ((m_rotationData.isHome == true) and (s_step1 == false))
			{
				std::print(CCYAN"[*] STEP1\n"); std::print(CRESET);

				s_step1 = true;
				// Stop a bit

				m_hardware.doMove(false);
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}
			else if (s_step1 == true)// Step 2: Make 2 turns with chronometer to calculate velocity
			{
				static size_t s_runonece = 0;
				static size_t s_turns = 0;
				if (!m_rotationData.isHome)
				{
					home = 0;
				}

				// Prepare timer once and start moving
				if (s_runonece == 0)
				{
					std::print(CCYAN"[*] STEP2\n"); std::print(CRESET);
					m_hardware.doMove(true);
					m_timer_cal.start();
					++s_runonece;
				}
				else if (s_step2 == false)
				{
					// Turn counter. More is accurate.
					const size_t turnsToDo = 2;

					// Home reached counter

					checkHomeRise(s_turns);
					std::print(CYELLOW"[*] Turns: {}\r", s_turns); std::print(CRESET);
					if (s_turns == turnsToDo)
					{
						m_hardware.doMove(false);
						m_timer_cal.stop();

						double elapsedTimeSec = m_timer_cal.elapsed_seconds();

						assert(elapsedTimeSec > 0.0);

						// Compute velocity
						double val;
						computeVelocity(val, elapsedTimeSec);
						m_rotationData.velocityDegPerSec = val;

						s_step2 = true;
					}
				}
				else if (s_step2 == true)
				{
					std::print(CGREEN"[*] Calibration done. Velocity computed: {}\n[*]Reseting state\n", m_rotationData.velocityDegPerSec.get());
					std::print(CRESET);

					// Save to disk
					try
					{
						writeCal(std::abs(m_rotationData.velocityDegPerSec.get()));
					}
					catch (std::exception& e)
					{
						std::print(CRED"[*] Error saving calibration data: {}\n", e.what());
						std::print(CRESET);
					}

					s_runonece = 0;
					s_turns = 0;
					s_step0 = false;
					s_step1 = false;
					s_step2 = false;

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
	}

	void poll(std::stop_token stopToken)
	{
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
				dbg::print(CRED"[*] State: STOPPED\n"); dbg::print(CRESET);
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
		
		if((home==0) and (m_rotationData.isHome==true))
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
			calData = calData.substr(calData.find("::")+2);
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
		m_file.selectFile(calFilename);
		std::string calData;
		double calVelocity;
		readCal(calData, calVelocity);

		m_rotationData.velocityDegPerSec = NAN;
		if (calVelocity > 0 and calVelocity < 60)
		{
			m_rotationData.velocityDegPerSec = calVelocity;
		}

		m_rotationData.directionSign = 1.0;
		
		m_rotationData.currentAngle = 0.0;
		m_rotationData.targetAngle = g_accuracy*2;
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
	rotation::driver_r208& m_parent;
	hardware_interface& m_hardware;
	timer_interface& m_timer_cal;
	timer_interface& m_timer_move;
	file_interface& m_file;
	automation_interface& m_script;

	// datas
	size_t home;
	std::jthread m_pollingThread;
	std::jthread m_serverThread;
	STATE m_state = STATE::STOPPED;
	ROTATIONSTRUCT m_rotationData{};
	frequency_t m_freqHz;

	std::mutex m_mutexApi;
	std::mutex m_mutexHome;;
};

///////////////////////////////////////////////////////////////////////////////////////
//
// PIMPL class
// This class is only to make a clean header (proxy).

rotation::driver_r208::driver_r208()
	: implDeps(std::make_unique<dependenciesimpl>())
{
	impl = std::make_unique<rotationimpl>(*this,
		implDeps->hardware, implDeps->timer1, implDeps->timer2, implDeps->file, implDeps->script);
	std::print("[*] driver_r208 loading (production)\n");
}

rotation::driver_r208::driver_r208(dependencies deps)
	: impl(std::make_unique<rotationimpl>(*this,
		deps.hardware, deps.timer1, deps.timer2, deps.file, deps.script))
{
	std::print("[*] driver_r208 loading (injected)\n");
}

bool rotation::driver_r208::start(frequency_t freqHz)
{
	assert(freqHz.get() > 0.0);
	assert(freqHz.get() <= 2000.0);
	return impl->start(freqHz);
}

bool rotation::driver_r208::stop()
{
	std::print("[*] driver_r208 stopping\n");
	return impl->stop();
}

bool rotation::driver_r208::doCalibrating()
{
	return impl->doCalibrating();
}

bool rotation::driver_r208::doHoming()
{
	return impl->doHoming();
}

bool rotation::driver_r208::readAngle(angle_t& angle)
{
	angle_t val{};
	if(!impl->readAngle(val))
	{
		std::print("[*] Error reading angle\n");
		return false;
	}
	angle = angle_t{val};
	return true;
}

bool rotation::driver_r208::setAngle(angle_t angle)
{
	if(!impl->setAngle(angle.get()))
	{
		std::print("[*] Error setting angle\n");
		return false;
	}
	return true;
}

rotation::driver_r208::~driver_r208()
{
	std::print("[*] driver_r208 finnish\n");
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Free functions
//

void computeVelocity(double& velocity, double elapsedSeconds)
{
	const size_t degreesToTurn = 360;
	velocity = degreesToTurn/ elapsedSeconds;
	std::print("[*] Elapsed: {:1} seconds, Velocity calculated: {:1} degrees/second\n", elapsedSeconds, velocity);
}
