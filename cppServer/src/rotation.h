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
#include <algorithm>
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
	explicit angle_t(T value) : m_value(std::clamp(std::fmod(std::fmod(value, 360.0) + 360.0, 360.0), 0.0, 360.0)) {}

	angle_t& operator= (double value)
	{
		m_value = std::clamp(std::fmod(std::fmod(value, 360.0) + 360.0, 360.0), 0.0, 360.0);
		return *this;
	}

	[[nodiscard]] constexpr double get() const { return std::clamp(m_value, 0.0, 360.0); }
private:
	double m_value{};
};

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

/////////////////////////////////////////////////////////////////////////////
// 
// define: Velocity in deg/clock
//

struct clkPerDeg_t
{
	constexpr clkPerDeg_t() = default;

	template<typename T> requires std::same_as<T, double>
	explicit constexpr clkPerDeg_t(T value) : m_value(value) {
		if (m_value < m_minVelocity)m_value = m_minVelocity; if (m_value >= m_maxVelocity)m_value = m_maxVelocity;
	}

	constexpr clkPerDeg_t& operator= (double value)
	{
		// Sanity check for velocity
		if (value < m_minVelocity)value = m_minVelocity; if (value >= m_maxVelocity)value = m_maxVelocity;
		m_value = value; return *this;
	}

	[[nodiscard]] constexpr double get() const { return m_value; }
private:
	double m_value{};
	static constexpr double m_maxVelocity = 100000.0; // Max voltage in volt
	static constexpr double m_minVelocity = -100000.0; // Min voltage in volt
};

/////////////////////////////////////////////////////////////////////////////
// 
// Rotation main class:
// 

// Forward dependencies declarations
struct hardware_interface;
struct counter_interface;
struct file_interface;
struct automation_interface;

enum class positionnning_mode {
	TIME_BASED,
	CLOCK_BASED
};

class lexitek
{
public:
	// This struct is used to inject dependencies in the constructor.
	// Caller can choose to pass production or mock dependencies for testing if needed.
	struct dependencies {
		std::unique_ptr<hardware_interface> hardware;
		std::unique_ptr<counter_interface> counter1;
		std::unique_ptr<counter_interface> counter2;
		std::unique_ptr<file_interface> file;
		std::unique_ptr<automation_interface> script;

		void reset()
		{
			hardware.reset();
			counter1.reset();
			counter2.reset();
			file.reset();
			script.reset();
		}
	};

	explicit lexitek(positionnning_mode mode);
	~lexitek();

	bool setPositioningMode(positionnning_mode mode);// Change the positioning mode at runtime

	bool start(frequency_t frequency); // (2) table.start(frequency{100.0});
	bool stop(); // (6) Stop the table
	bool doCalibrating(); // (3) Calibrate will set the rotation table in a known state
	bool doHoming();
	bool readAngle(angle_t& angle); // (5) Read current position
	bool setAngle(angle_t angle); // (4) Go to a position

private:
	bool setStrategy(positionnning_mode mode);

	// Opaque ptr logic 
	class rotationImpl;
	std::unique_ptr<rotationImpl> m_impl;
	dependencies m_deps;
	positionnning_mode m_mode;
};

