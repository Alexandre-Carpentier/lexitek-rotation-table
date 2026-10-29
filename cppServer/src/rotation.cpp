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
#include "backend_rpc.h"
#include "hardwareControl.h"
#include "timerControl.h"
#include "fileControl.h"
#include "automationControl.h"

///////////////////////////////////////////////////////////////////////////////////////
//
// OPAQUE implementation of the lexitek class external dependencies
// instanciate real dependencies here

struct lexitek::dependenciesimpl
{
	timeBased_hardware hardware; // Real hardware for prod
	production_timer    timer1;
	production_timer    timer2;
	production_file     file;
	production_automation script;
};

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
	//std::unique_ptr<rotationTimeBasedImpl> timeImpl;

	// Opaque ptr logic for clock based positionning
	class rotationClockBasedImpl;
	//std::unique_ptr<rotationClockBasedImpl> clockImpl;
};

lexitek::rotationImpl::~rotationImpl() = default;

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
	: implDeps(std::make_unique<dependenciesimpl>())
{
	// Factory
	switch (mode)
	{

	case positionnning_mode::TIME_BASED:
	{
		impl = std::make_unique<rotationImpl::rotationTimeBasedImpl>(*this,
			implDeps->hardware, implDeps->timer1, implDeps->timer2, implDeps->file, implDeps->script);
		break;
	}
	case positionnning_mode::CLOCK_BASED:
	{
/*		impl = std::make_unique<rotationImpl::rotationClockBasedImpl>(*this,
			implDeps->hardware, implDeps->timer1, implDeps->timer2, implDeps->file, implDeps->script);*/
		break;
	}

	}


	std::print("[*] driver_r208 loading (production)\n");
}

lexitek::lexitek(positionnning_mode mode, dependencies deps)
	: impl(std::make_unique<rotationImpl::rotationTimeBasedImpl>(*this,
		deps.hardware, deps.timer1, deps.timer2, deps.file, deps.script))
{
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
