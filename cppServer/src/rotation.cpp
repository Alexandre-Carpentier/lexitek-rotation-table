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
#include <string>

/////////////////////////////////////////////////////////////////////////////
// 
// Dependencies (components) of the project
//

#include "colors.h"
#include "hardwareControl.h"
#include "timerControl.h"
#include "fileControl.h"
#include "automationControl.h"

///////////////////////////////////////////////////////////////////////////////////////
//
// Interface class
//

class lexitek::rotationImpl
{
public:
	virtual ~rotationImpl() = 0;

	virtual bool start(frequency_t freqHz) = 0;
	virtual bool stop() = 0;
	virtual bool doCalibrating() = 0;
	virtual bool doHoming() = 0;
	virtual bool readAngle(angle_t& angle) = 0;
	virtual bool setAngle(double angle) = 0;

	// Opaque ptr logic for time based positionning
	class rotationTimeBasedImpl;

	// Opaque ptr logic for clock based positionning
	class rotationClockBasedImpl;
};

lexitek::rotationImpl::~rotationImpl() = default;

/////////////////////////////////////////////////////////////////////////////
// 
// Custom print function for debugging.
//

// Example: dbg::print("Current angle: {}\n", currentAngle.get()); // Relace std::print with dbg::print
//#define __VERBOSE // Uncomment to print detailed states in the console for debugging (can reduce performance as std::print is slow)

namespace dbg
{
	template<typename... Args>
	void print(std::format_string<Args...> fmt, Args&&... args)
	{
#ifdef __VERBOSE
		std::print(fmt, std::forward<Args>(args)...);
#endif
	}
}

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
	clkPerDeg_t velocityClkPerTurn;
	volt_t homeVoltage;
	double duration;
	uint64_t counterValue;
	bool isComputingPositionRequested;
	bool isMoving;
	bool isTargetUpdated;
	bool isAtTarget;
	bool isHome;
	bool isCalibrationRequested;
	bool isCalibrated;
};

/////////////////////////////////////////////////////////////////////////////
// 
// Buissness logic of the project
//

#include "rotationTimeBased.h"
#include "rotationClockBased.h"

///////////////////////////////////////////////////////////////////////////////////////
//
// This class is only to provide a clean header (opaque proxy).
// refer to pimpl idiom

lexitek::lexitek(positionnning_mode mode)
{
	// Factory for dependencies
	// Configure the production code here
	switch (mode)
	{

	case positionnning_mode::TIME_BASED:
	{
		static timeBased_hardware hw;
		static timeCounter t1, t2;
		static production_file f;
		static production_automation script;

		impl = std::make_unique<rotationImpl::rotationTimeBasedImpl>(*this,
			hw, t1, t2, f, script);
		break;
	}
	case positionnning_mode::CLOCK_BASED:
	{
		static clockBased_hardware hw;
		static clockCounter t1, t2;
		static production_file f;
		static production_automation script;

		impl = std::make_unique<rotationImpl::rotationClockBasedImpl>(*this,
			hw, t1, t2, f, script
		);

		break;
	}
	}

	std::print("[*] driver_r208 loading (production)\n");
}

lexitek::lexitek(positionnning_mode mode, dependencies deps)
{
	// Factory
	switch (mode)
	{

	case positionnning_mode::TIME_BASED:
	{
		impl = std::make_unique<rotationImpl::rotationTimeBasedImpl>(*this,
			deps.hardware, deps.counter1, deps.counter2, deps.file, deps.script);
		break;
	}
	case positionnning_mode::CLOCK_BASED:
	{
		impl = std::make_unique<rotationImpl::rotationClockBasedImpl>(*this,
			deps.hardware, deps.counter1, deps.counter2, deps.file, deps.script);
		break;
	}

	}
	std::print("[*] lexitek loading (injected)\n");
}

bool lexitek::start(frequency_t freqHz)
{
	assert(freqHz.get() > 0.0);
	assert(freqHz.get() <= 2000.0);
	return impl->start(freqHz);
}

bool lexitek::stop()
{
	std::print("[*] driver_r208 stopping\n");
	return impl->stop();
}

bool lexitek::doCalibrating()
{
	return impl->doCalibrating();
}

bool lexitek::doHoming()
{
	return impl->doHoming();
}

bool lexitek::readAngle(angle_t& angle)
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

bool lexitek::setAngle(angle_t angle)
{
	if(!impl->setAngle(angle.get()))
	{
		std::print("[*] Error setting angle\n");
		return false;
	}
	return true;
}

lexitek::~lexitek()
{
	std::print("[*] lexitek finnish\n");
}
