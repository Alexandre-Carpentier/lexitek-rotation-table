/////////////////////////////////////////////////////////////////////////////
// Author:      Alexandre CARPENTIER
// Modified by:
// Created:     06/04/2026
// Copyright:   (c) Alexandre CARPENTIER
// Licence:     LGPL-2.1-or-later
/////////////////////////////////////////////////////////////////////////////
#pragma once
#include <memory>
#include <iostream>
#include <print>
#include <filesystem>

bool Execute(std::string executable_name) 
{
    namespace fs = std::filesystem;

    std::print("[*] Starting backend service...\n");

    // Prepare
    fs::path serverPath = fs::current_path();
	serverPath /= executable_name;
#ifdef _WIN32
    serverPath += ".exe";
    std::string command = "start /B \"\" \"" + serverPath.string() + "\"";
#else
    std::string command = "\"" + serverPath.string() + "\" &";
#endif

    // Test 
    if (!fs::exists(serverPath)) {
        std::print("[!] Executable not found at: {}\n", serverPath.string());
        return false;
    }

    // Exec
    int result = std::system(command.c_str());

    if (result == 0) {
        std::print("[*] Exec success\n");
        return true;
    }
    else {
        std::print("[!] Failed to exec (error code: {})\n", result);
        return false;
    }
}