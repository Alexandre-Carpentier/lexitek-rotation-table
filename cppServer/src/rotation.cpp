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

/////////////////////////////////////////////////////////////////////////////
// 
// Buissness logic of the project
//

#include "rotationTimeBased.h"
#include "rotationClockBased.h"

///////////////////////////////////////////////////////////////////////////////////////
//
// This class is only to provide a clean header (proxy).
// refer to pimpl idiom

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
