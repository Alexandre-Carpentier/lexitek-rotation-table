/////////////////////////////////////////////////////////////////////////////
// Author:      Alexandre CARPENTIER
// Modified by:
// Created:     06/04/2026
// Copyright:   (c) Alexandre CARPENTIER
// Licence:     LGPL-2.1-or-later
/////////////////////////////////////////////////////////////////////////////
#include "rotation.h"

#include <print>

#ifdef _WIN32
#include "../../../../rotation-frontend/src/shell.h"
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

    std::print("[*] Launching rotation table.\n");
    rotation::driver_r208 operateRotationSoftware;

    while(getchar() != EOF)
    {
        // Keep the main thread alive to allow the server and polling threads to run
	}
    return 0;
}
