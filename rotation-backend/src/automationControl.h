/////////////////////////////////////////////////////////////////////////////
// Author:      Alexandre CARPENTIER
// Modified by:
// Created:     02/05/2026
// Copyright:   (c) Alexandre CARPENTIER
// Licence:     LGPL-2.1-or-later
/////////////////////////////////////////////////////////////////////////////
#pragma once
#include <string>
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
// 
// Automation tools with lua
//
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

class automation_interface
{
public:
	virtual bool loadScript(std::string filename) = 0;
	virtual bool unloadScript() = 0;
	virtual ~automation_interface() = default; // Mandatory for childrens
};

class production_automation : public automation_interface
{
public:
	virtual bool loadScript(std::string filename) override
	{
		if (filename.empty())
		{
			std::print("[*] No script filename provided\n");
			return false;
		}
		m_filename = filename;

		std::print("[*] Loading {} script\n", filename);
		m_script = luaL_newstate();
		if (!m_script)
		{
			std::print("[*] Loading {} failed\n", filename);
			return false;
		}

		assert(m_script); // Ensure automation loaded correctly
		luaL_openlibs(m_script);
		luaL_dofile(m_script, filename.c_str());
		return true;
	}
	virtual bool unloadScript() override
	{
		if (m_script)
		{
			lua_close(m_script);
			m_script = nullptr;
			return true;
		}
		return false;
	}
	virtual ~production_automation() override
	{
		if (m_script)
		{
			lua_close(m_script);
			m_script = nullptr;
		}
	}
private:
	lua_State* m_script = nullptr;
	std::string m_filename;
};

