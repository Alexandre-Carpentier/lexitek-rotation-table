/////////////////////////////////////////////////////////////////////////////
// Author:      Alexandre CARPENTIER
// Modified by:
// Created:     06/04/2026
// Copyright:   (c) Alexandre CARPENTIER
// Licence:     LGPL-2.1-or-later
/////////////////////////////////////////////////////////////////////////////
#include "backend_rpc.h"
#include "rotation.h"
#include <print>

#ifdef _WIN32
#include "../../../../qtClient/src/shell.h"
#endif
#ifdef __APPLE__
#include "../../qtClient/src/shell.h"
#endif

int main(int argc, char **argv)
{
    //argc = 2;

    if (argc > 1)
    {
        bool isServerRunning = Execute("qtClient");
        if (!isServerRunning)
        {
            std::print("[*] Warning: Could not start frontend process\n");
        }
    }

    std::print("[*] Launching rotation table.\n");
    lexitek operateRotationSoftware(positionnning_mode::CLOCK_BASED);

    std::print("[*] Starting server thread\n");
    std::jthread m_serverThread = std::jthread([&](std::stop_token st) {
        backend_rpc_init(operateRotationSoftware, st);
        });
    assert(m_serverThread.joinable());// Ensure the server thread is running correctly

    std::print("[*] Launching gRpc server.\n");

    while(getchar() != EOF)
    {
        // Keep the main thread alive to allow the server and polling threads to run
	}

    return 0;
}
