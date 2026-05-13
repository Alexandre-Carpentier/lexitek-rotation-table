/////////////////////////////////////////////////////////////////////////////
// Author:      Alexandre CARPENTIER
// Modified by:
// Created:     06/04/2026
// Copyright:   (c) Alexandre CARPENTIER
// Licence:     LGPL-2.1-or-later
/////////////////////////////////////////////////////////////////////////////
#include <QApplication>
#include <QMessageBox>
#include <memory>
#include <iostream>
#include <print>

#include "mainwindow.h"
#include "shell.h"

int main(int argc, char *argv[])
{
    ///////////////////////////////////////////////////////////////////////////////////////
    //
	// First start the remote backend process (server)
    //

//#define _BACKEND_EXEC
#ifdef _BACKEND_EXEC
    std::print("[*] Backend Starting...\n");
    bool isServerRunning = Execute("rotation-backend");
    if (!isServerRunning) {
        std::print("Warning: Could not start backend process\n");
    }
#endif
    ///////////////////////////////////////////////////////////////////////////////////////
    // 
	// Start the frontend (client)
    //
    std::print("[*] Frontend Starting...\n");

    QApplication app (argc, argv);

    MainWindow ui;
    ui.show();
    ui.setWindowTitle("[*] Rotation table controler: carpentier@iram.fr");

    return app.exec();
}