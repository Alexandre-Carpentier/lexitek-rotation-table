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
#include "../../rotation-backend/src/rotation.h"
#include "../../rotation-backend/src/hardwareControl.h"
#include "../../rotation-backend/src/timerControl.h"
#include "../../rotation-backend/src/fileControl.h"
#include "../../rotation-backend/src/automationControl.h"

#ifdef _WIN32
#include "../../../rotation-frontend/src/shell.h"
#endif
#ifdef __APPLE__
#include "../../rotation-frontend/src/shell.h"
#endif

int main()
{
    bool isServerRunning = Execute("rotation-frontend");
    if (!isServerRunning)
    {
        std::print("[*] Warning: Could not start frontend process\n");
    }

    std::print("[*] Injecting test mock\n");
    mock_hardware hw;
    production_timer t1, t2;
    production_file f;
    production_automation script;
    rotation::driver_r208 rotation{ rotation::driver_r208::dependencies{ hw, t1, t2, f, script } };

    while (getchar() != EOF)
    {
        // Keep the main thread alive to allow the server and polling threads to run
    }
    return 0;
}