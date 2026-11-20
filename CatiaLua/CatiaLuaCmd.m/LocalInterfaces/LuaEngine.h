// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2026 nwpu052009 <ljwangcumtb@163.com>

//===========================================================================
// LuaEngine.h
// Lua �����װ�ࣨ�������������� CATIA ����е��� Lua �ű�
//===========================================================================
#ifndef LuaEngine_H
#define LuaEngine_H

#include <string>

struct lua_State;

class LuaEngine
{
public:
	// ������ڣ�����ʽ��VS2005 ���ݣ���ָ̬�� + �״� new��
	static LuaEngine* GetInstance();
	// �ͷŵ�������ѡ���ã�
	static void Destroy();

	// ��ʼ�� Lua ״̬��luaL_newstate + luaL_openlibs�����ݵȣ����ظ�����
	bool Init();
	// �رղ��ͷ� Lua ״̬
	void Close();
	// ���ز�ִ�нű��ļ���luaL_dofile����ʧ�ܷ��� false ����¼����
	bool LoadScriptFile(const char* path);
	// ���ýű��е�ȫ�ֺ�����lua_getglobal + lua_pcall��
	bool RunFunction(const char* funcName);
	bool RunString(const char* code);
	// ���һ�δ�����Ϣ
	const char* GetLastError() const { return m_strError.c_str(); }

private:
	LuaEngine();
	~LuaEngine();
	LuaEngine(const LuaEngine&);            // ���ÿ���
	LuaEngine& operator=(const LuaEngine&); // ���ø�ֵ

	bool IsInited() const { return m_pState != NULL; }

	lua_State* m_pState;
	std::string m_strError;
};

#endif // LuaEngine_H
