/////////////////////////////////////////////////////////////////////////////
// Author:      Alexandre CARPENTIER
// Modified by:
// Created:     06/04/2026
// Copyright:   (c) Alexandre CARPENTIER
// Licence:     LGPL-2.1-or-later
/////////////////////////////////////////////////////////////////////////////
#include <QApplication>
#include <QPalette>
#include <QColor>
#include <QMessageBox>
#include <memory>
#include <iostream>
#include <print>

#include "mainwindow.h"
#include "shell.h"

    ///////////////////////////////////////////////////////////////////////////////////////
    //
    // Custom colors and style
    //

void setDarkTheme(QApplication& app)
{
    app.setStyle("Fusion");
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(45, 45, 45));
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::ToolTipBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(90, 170, 255));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::white);
    darkPalette.setColor(QPalette::PlaceholderText, QColor(160, 160, 160));
    darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(127, 127, 127));
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(127, 127, 127));
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(127, 127, 127));
    app.setPalette(darkPalette);
}

int main(int argc, char *argv[])
{
    ///////////////////////////////////////////////////////////////////////////////////////
    //
	// First start the remote backend process (server)
    //

//#define _BACKEND_EXEC
#ifdef _BACKEND_EXEC
    std::print("[*] Backend Starting...\n");
    bool isServerRunning = Execute("cppServer");
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
    setDarkTheme(app);
    MainWindow ui;
    ui.show();
    ui.setWindowTitle("[*] Rotation table controler: carpentier@iram.fr");

    return app.exec();
}
