/////////////////////////////////////////////////////////////////////////////
// Author:      Alexandre CARPENTIER
// Modified by:
// Created:     06/04/2026
// Copyright:   (c) Alexandre CARPENTIER
// Licence:     LGPL-2.1-or-later
/////////////////////////////////////////////////////////////////////////////
#pragma	once
#include <cmath>
#include <memory>
#include <string>
#include <expected>

/////////////////////////////////////////////////////////////////////////////
// define: frequency in Hz
//

struct frequency_t
{
	constexpr frequency_t() = default;

	template<typename T> requires std::same_as<T, double>
	explicit constexpr frequency_t(T value) : m_value(value) {
		if (m_value < 0)m_value = 0.0; if (m_value > m_maxHz)m_value = m_maxHz; 
	}

	constexpr frequency_t& operator= (double value) 
	{
		// Sanity check for frequency
		if (value < 0)value = 0.0; if (value > m_maxHz)value = m_maxHz;
		m_value = value; return *this;
	}

	[[nodiscard]] constexpr double get() const { return m_value; }
private:
	double m_value{};
	static constexpr double m_maxHz = 2000.0; // NI USB6001 max frequency is 2000Hz
};

/////////////////////////////////////////////////////////////////////////////
// define: angle in degrees
//

struct angle_t
{
	constexpr angle_t() = default;

	template<typename T> requires std::same_as<T, double>
	explicit angle_t(T value) : m_value(std::fmod(std::fmod(value, 360.0) + 360.0, 360.0)) {}

	angle_t& operator= (double value)
	{
		m_value = std::fmod(std::fmod(value, 360.0) + 360.0, 360.0);
		return *this;
	}

	[[nodiscard]] constexpr double get() const { return m_value; }
private:
	double m_value{};
};

/////////////////////////////////////////////////////////////////////////////
// 
// Rotation main class:
// 

// Forward dependencies declarations
struct hardware_interface;
struct timer_interface;
struct file_interface;
struct automation_interface;

namespace rotation
{
	class driver_r208 
	{
	public:
		// This struct is used to inject dependencies in the constructor.
		// Caller can choose to pass production or mock dependencies for testing.
		struct dependencies {
			hardware_interface& hardware;
			timer_interface&    timer1;
			timer_interface&    timer2;
			file_interface&     file;
			automation_interface& script;
		};

		explicit driver_r208();// (1) Default constructor for prod
		[[deprecated]] explicit driver_r208(dependencies deps); // Inject for tests only. This is why it is marked as deprecated to avoid misuse in production code.
		~driver_r208();

		bool start(frequency_t frequency); // (2) table.start(frequency{100.0});
		bool stop(); // (6) Stop the table
		bool doCalibrating(); // (3) Calibrate will set the rotation table in a known state
		bool doHoming();
		bool readAngle(angle_t& angle); // (5) Read current position
		bool setAngle(angle_t angle); // (4) Go to a position

	private:
		// Opaque ptr logic
		class rotationimpl;
		std::unique_ptr<rotationimpl> impl;
		// Opaque ptr dependencies
		struct dependenciesimpl;
		std::unique_ptr<dependenciesimpl> implDeps;
	};
}
