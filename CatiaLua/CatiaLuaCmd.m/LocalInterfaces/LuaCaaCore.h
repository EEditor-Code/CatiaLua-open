// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2026 nwpu052009 <ljwangcumtb@163.com>

//===========================================================================
// LuaCaaCore.h
// CAA interface <-> Lua binding core (hand-written part).
// Generated wrappers (lua_caa_gen.cpp) build on top of this core.
//
// Encoding note: this file is pure ASCII so that it compiles on the
// VS2005 RADE toolchain without codepage issues.
//===========================================================================
#ifndef LuaCaaCore_H
#define LuaCaaCore_H

struct lua_State;
struct luaL_Reg;
class CATBaseUnknown;
class CATUnicodeString;

// Windows GUID forward declaration (IID is a GUID typedef).
struct _GUID;
typedef struct _GUID GUID;

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
// Called from LuaEngine::Init() right after luaL_openlibs.
// Registers the "catia" session table (activePart / update) and delegates
// interface registration to lua_caa_gen_open.
int lua_caa_open(lua_State* L);

// ---------------------------------------------------------------------------
// Provided by lua_caa_gen.cpp (GENERATED -- do not hand-edit)
// ---------------------------------------------------------------------------
// Registers the metatable of every generated interface, in parent-first
// order (an interface's parent must be registered before the interface).
int lua_caa_gen_open(lua_State* L);

// Returns the IID of an interface by name ("CATISketchFactory"),
// or NULL when the name is not generated.
const GUID* lua_caa_gen_iid(const char* iName);

// ---------------------------------------------------------------------------
// Helpers used by generated wrappers
// ---------------------------------------------------------------------------
// Pushes a CAA object on the Lua stack (AddRef) under the given interface
// name. NULL pointer becomes nil. Falls back to the CATBaseUnknown
// metatable when the interface is not registered.
void lua_caa_push(lua_State* L, CATBaseUnknown* iObj, const char* iIfaceName);

// Checks that the stack value at iIdx is a CAA object of the given
// interface (or of one of its parents). iIfaceName may be NULL to accept
// any CAA object. Returns the stored interface pointer.
CATBaseUnknown* lua_caa_check(lua_State* L, int iIdx, const char* iIfaceName);

// Standard HRESULT result handling for generated wrappers:
//   success -> 0            (out parameters are pushed separately)
//   failure -> nil, errmsg
int lua_caa_hr(lua_State* L, long iHr, const char* iMethod);

// Reads a Lua string into a CATUnicodeString.
int lua_caa_check_ustring(lua_State* L, int iIdx, CATUnicodeString& oStr);

// Pushes a CATUnicodeString as a Lua string (copies the buffer).
void lua_caa_push_ustring(lua_State* L, const CATUnicodeString& iStr);

// Reads a Lua table {x, y} (or a single number) into oBuf; returns the
// number of values read (at most iMaxN).
int lua_caa_check_double_array(lua_State* L, int iIdx, double* oBuf, int iMaxN);

// Registers an interface metatable: stores iMethods under "methods",
// chains to the parent metatable iParentName (which must already exist),
// installs __index / __gc / __tostring. iMethods may be NULL (metatable
// with no generated methods yet).
void lua_caa_register_iface(lua_State* L, const char* iName,
                            const luaL_Reg* iMethods, const char* iParentName);

#endif // LuaCaaCore_H
