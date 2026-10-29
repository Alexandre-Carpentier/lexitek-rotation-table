/////////////////////////////////////////////////////////////////////////////
// Author:      Alexandre CARPENTIER
// Modified by:
// Created:     01/05/2026
// Copyright:   (c) Alexandre CARPENTIER
// Licence:     LGPL-2.1-or-later
/////////////////////////////////////////////////////////////////////////////
#include <print>
#include <cassert>
#include <memory>
#include "../../cppServer/src/rotation.h"
#include "../../cppServer/src/hardwareControl.h"
#include "../../cppServer/src/timerControl.h"
#include "../../cppServer/src/fileControl.h"
#include "../../cppServer/src/automationControl.h"

#ifdef _WIN32
#include "../../../qtClient/src/shell.h"
#endif
#ifdef __APPLE__
#include "../../qtClient/src/shell.h"
#endif

int main()
{
    bool isServerRunning = Execute("qtClient");
    if (!isServerRunning)
    {
        std::print("[*] Warning: Could not start frontend process\n");
    }

    std::print("[*] Injecting test mock\n");
    mock_hardware hw;
    production_timer t1, t2;
    production_file f;
    production_automation script;
    lexitek rotation{ positionnning_mode::TIME_BASED,lexitek::dependencies{ hw, t1, t2, f, script } };

    while (getchar() != EOF)
    {
        // Keep the main thread alive to allow the server and polling threads to run
    }
    return 0;
}