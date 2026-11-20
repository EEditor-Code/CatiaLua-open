// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2026 nwpu052009 <ljwangcumtb@163.com>

//===========================================================================
// LuaEngine.cpp
// Lua �����װ�ࣨ������ʵ��?//===========================================================================
#include "LuaEngine.h"
#include "LuaCaaCore.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

static LuaEngine* s_pInstance = NULL;

LuaEngine* LuaEngine::GetInstance()
{
	if (s_pInstance == NULL)
	{
		s_pInstance = new LuaEngine();
	}
	return s_pInstance;
}

void LuaEngine::Destroy()
{
	if (s_pInstance != NULL)
	{
		s_pInstance->Close();
		delete s_pInstance;
		s_pInstance = NULL;
	}
}

LuaEngine::LuaEngine()
	: m_pState(NULL)
{
}

LuaEngine::~LuaEngine()
{
	Close();
}

bool LuaEngine::Init()
{
	if (IsInited())
		return true;
	m_pState = luaL_newstate();
	if (m_pState == NULL)
	{
		m_strError = "luaL_newstate ʧ��";
		return false;
	}
	luaL_openlibs(m_pState);
	lua_caa_open(m_pState);
	m_strError.clear();
	return true;
}

void LuaEngine::Close()
{
	if (m_pState != NULL)
	{
		lua_close(m_pState);
		m_pState = NULL;
	}
}

bool LuaEngine::LoadScriptFile(const char* path)
{
	if (!IsInited())
	{
		if (!Init())
			return false;
	}
	int nRet = luaL_dofile(m_pState, path);
	if (nRet != 0)
	{
		// ջ��ȡ������Ϣ
		const char* szErr = luaL_tolstring(m_pState, -1, NULL);
		m_strError = szErr ? szErr : "δ֪����";
		lua_pop(m_pState, 1);
		return false;
	}
	m_strError.clear();
	return true;
}

bool LuaEngine::RunFunction(const char* funcName)
{
	if (!IsInited())
	{
		if (!Init())
			return false;
	}
	lua_getglobal(m_pState, funcName);
	if (!lua_isfunction(m_pState, -1))
	{
		m_strError = std::string("����������: ") + funcName;
		lua_pop(m_pState, 1);
		return false;
	}
	int nRet = lua_pcall(m_pState, 0, 0, 0);
	if (nRet != 0)
	{
		const char* szErr = luaL_tolstring(m_pState, -1, NULL);
		m_strError = szErr ? szErr : "δ֪����";
		lua_pop(m_pState, 1);
		return false;
	}
	m_strError.clear();
	return true;
}

bool LuaEngine::RunString(const char* code)
{
	if (!IsInited())
	{
		if (!Init())
			return false;
	}
	int nRet = luaL_dostring(m_pState, code);
	if (nRet != 0)
	{
		const char* szErr = luaL_tolstring(m_pState, -1, NULL);
		m_strError = szErr ? szErr : "unknown error";
		lua_pop(m_pState, 1);
		return false;
	}
	m_strError.clear();
	return true;
}
