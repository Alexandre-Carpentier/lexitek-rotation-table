/////////////////////////////////////////////////////////////////////////////
// Author:      Alexandre CARPENTIER
// Modified by:
// Created:     06/04/2026
// Copyright:   (c) Alexandre CARPENTIER
// Licence:     LGPL-2.1-or-later
/////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <thread>
#include <chrono>

// Testing framework
#include "gtest/gtest.h"

// Components testing
#include "../../rotation-backend/src/timerControl.h"
TEST(test_components, timer_tests)
{
	std::print("[*] Test the timer class\n\n");

	// Test the timer interface
	production_timer timer;

	std::print("[*] Sleep 500ms\n");
	timer.start();
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	timer.stop();

	// Measurement must be between 0.4 and 0.6 seconds as sleep is 500ms
	std::cout << "Elapsed time: " << timer.elapsed_seconds() << " seconds" << std::endl;
	EXPECT_LT(timer.elapsed_seconds(), 0.55);
	EXPECT_GT(timer.elapsed_seconds(), 0.45);

	std::print("[*] Sleep 1000ms\n");
	timer.reset();

	timer.start();
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	timer.stop();

	std::cout << "Elapsed time: " << timer.elapsed_seconds() << " seconds" << std::endl;
	EXPECT_LT(timer.elapsed_seconds(), 1.05);
	EXPECT_GT(timer.elapsed_seconds(), 0.95);
}

#include "../../rotation-backend/src/fileControl.h"
TEST(test_components, file_tests)
{
	std::print("[*] Test the file class\n\n");

	production_file file;
	std::string filename = "toDelete.txt";
	std::string dataToWrite = "Tourner manège";
	std::string dataRead;
	file.selectFile(filename);
	file.clearContentFile();
	file.addToFile(dataToWrite);
	file.readFromFile(dataRead);

	std::cout << "Data read from file: " << dataRead << std::endl;
	EXPECT_STREQ(dataRead.c_str(), dataToWrite.c_str());
	system("del toDelete.txt");
}

#include "../../rotation-backend/src/automationControl.h"
TEST(test_components, automation_tests)
{
	std::print("[*] Test the automation class\n\n");
	// TODO: unit test
	production_automation script;
	script.loadScript("test_script.lua");
	script.unloadScript();
}

#include "../../rotation-backend/src/hardwareControl.h"
TEST(test_components, hardware_tests)
{
	std::print("[*] Test the hardware class\n\n");

	// Uncomment this line to enable hardware tests (requires connected DAQ hardware)
	 //#define REAL_TEST_HARDWARE
#ifdef REAL_TEST_HARDWARE
	production_hardware hardware;
	hardware.checkDaq();

	double voltage;
	hardware.acquires(voltage);
	std::print("[*] Read voltage: {}\n\n", voltage);
	EXPECT_LT(voltage, 6.0);
	EXPECT_GT(voltage, -1.0);

	hardware.setSpeed(500.0 );
	std::print("[*] Move ON");
	EXPECT_TRUE(hardware.doMove(true));
	std::this_thread::sleep_for(std::chrono::milliseconds(2000));

	std::print("[*] Move clockwise");
	hardware.doClockwizeDirection(true);
	std::this_thread::sleep_for(std::chrono::milliseconds(2000));

	std::print("[*] Move anticlockwise");
	hardware.doClockwizeDirection(false);
	std::this_thread::sleep_for(std::chrono::milliseconds(2000));

	std::print("[*] Move OFF");
	EXPECT_TRUE(hardware.doMove(false));
#else
	std::print("[*] Hardware tests skipped (REAL_TEST_HARDWARE not defined)\n");
	GTEST_SKIP() << "Hardware tests disabled - no DAQ hardware available";
#endif
}

#include "../../rotation-backend/src/rotation.h"
// Typesafety
TEST(TypeSafetyTest, frequency_clamps_to_valid_range) {
	std::print("[Test] frequency_t clamps to valid range\n");

	frequency_t freq_negative{ -100.0 };
	EXPECT_DOUBLE_EQ(freq_negative.get(), 0.0);

	frequency_t freq_too_high{ 5000.0 };
	EXPECT_DOUBLE_EQ(freq_too_high.get(), 2000.0); // Max 2000 Hz

	frequency_t freq_valid{ 1000.0 };
	EXPECT_DOUBLE_EQ(freq_valid.get(), 1000.0);
}

TEST(TypeSafetyTest, angle_clamps_to_valid_range) {
	std::print("[Test] angle_t clamps to valid range\n");

	angle_t angle_negative{ -45.0 };
	EXPECT_DOUBLE_EQ(angle_negative.get(), 0.0);

	angle_t angle_too_high{ 400.0 };
	EXPECT_DOUBLE_EQ(angle_too_high.get(), 359.9);

	angle_t angle_valid{ 180.0 };
	EXPECT_DOUBLE_EQ(angle_valid.get(), 180.0);
}

TEST(TypeSafetyTest, frequency_assignment_operator) {
	std::print("[Test] frequency_t assignment operator\n");

	frequency_t freq{ 100.0 };
	freq = 500.0;
	EXPECT_DOUBLE_EQ(freq.get(), 500.0);

	freq = -50.0; // must clamp 0
	EXPECT_DOUBLE_EQ(freq.get(), 0.0);

	freq = 3000.0; // must clamp 2000
	EXPECT_DOUBLE_EQ(freq.get(), 2000.0);
}

TEST(TypeSafetyTest, angle_assignment_operator) {
	std::print("[Test] angle_t assignment operator\n");

	angle_t angle{ 45.0 };
	angle = 90.0;
	EXPECT_DOUBLE_EQ(angle.get(), 90.0);

	angle = -30.0; // must clamp 0
	EXPECT_DOUBLE_EQ(angle.get(), 0.0);

	angle = 500.0; // must clamp 359.9
	EXPECT_DOUBLE_EQ(angle.get(), 359.9);
}
TEST(test_system, rotation_test) {
	mock_hardware hw;
	production_timer t1, t2;
	production_file f;
	production_automation script;
	rotation::driver_r208 rotation{ rotation::driver_r208::dependencies{ hw, t1, t2, f, script } };

	std::print("[*] Test the rotation class\n");
	std::print("[*] Injecting dependencies in constructor\n");

	std::print("[*] Test the \"start()\"\n");
	EXPECT_TRUE(rotation.start(frequency_t{1000.0}));

	std::print("[*] Test the \"calibrate()\"\n");
	EXPECT_TRUE(rotation.doCalibrating());

	std::print("[*] Test the \"readAngle()\"\n");
	angle_t angle{};
	EXPECT_FALSE(rotation.readAngle(angle));
	std::print("[*] Current angle: {}\n\n", angle.get());
	EXPECT_FLOAT_EQ(angle.get(), 0.0f);

	std::print("[*] Test the \"setAngle()\"\n");
	EXPECT_TRUE(rotation.setAngle(angle_t{90.0}));
	std::print("[*] Angle set to 90°");

	std::print("[*] Test the \"stop()\"\n");
	EXPECT_TRUE(rotation.stop());
	//rotation.~driver_r208(); // Use after free
}

TEST(test_system, cleanup) {

	std::print(CGREEN"[*] Test Finished\n"); std::print(CRESET);

}

// ✅ Comparaisons de base
//  Macro	Description
//  EXPECT_EQ(a, b)	a == b
//  EXPECT_NE(a, b)	a != b
//  EXPECT_LT(a, b)	a < b
//	EXPECT_LE(a, b)	a <= b
//	EXPECT_GT(a, b)	a > b
//	EXPECT_GE(a, b)	a >= b
//	-- -
//	✅ Booléens
//	Macro	Description
//	EXPECT_TRUE(condition)	Condition est vraie
//	EXPECT_FALSE(condition)	Condition est fausse
//	-- -
//	✅ Chaînes de caractères(const char*)
//	Macro	Description
//	EXPECT_STREQ(a, b)	Contenu identique
//	EXPECT_STRNE(a, b)	Contenu différent
//	EXPECT_STRCASEEQ(a, b)	Identique(insensible à la casse)
//	EXPECT_STRCASENE(a, b)	Différent(insensible à la casse)
//	-- -
//	✅ Flottants
//	Macro	Description
//	EXPECT_FLOAT_EQ(a, b)	Égalité float(~4 ULP)
//	EXPECT_DOUBLE_EQ(a, b)	Égalité double(~4 ULP)
//	EXPECT_NEAR(a, b, delta)	\ | a - b\| <= delta


int main()
{
	std::print("[*] Test components and \"end-to-end\" here.\n[*] Must be run after new integration to grant program is working correctly.\n");
	std::print("[*] Before programming was fun..., but now we need to test everything to ensure it works correctly.\n");

	testing::InitGoogleTest();
	return RUN_ALL_TESTS();
}