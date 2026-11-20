// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2026 nwpu052009 <ljwangcumtb@163.com>

//===========================================================================
// LuaCaaCore.cpp
// Hand-written binding core: CAA object userdata, QueryInterface, method
// dispatch chains, type marshalling helpers, "catia" session table.
//
// Encoding note: this file is pure ASCII so that it compiles on the
// VS2005 RADE toolchain without codepage issues.
//===========================================================================
#include "LuaCaaCore.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include "CATBaseUnknown.h"
#include "CATUnicodeString.h"
#include "CATISpecObject.h"
#include "CATFrmEditor.h"
#include "CATDocument.h"
#include "CATIContainerOfDocument.h"
#include "CATIContainer.h"
#include "CATIPrtContainer.h"
#include "CATIAPart.h"
#include "CATILinkableObject.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ---------------------------------------------------------------------------
// userdata layout: one full userdata holding a CATBaseUnknown* (interface
// pointer, ref-counted with AddRef/Release); the metatable is the
// interface metatable registered by lua_caa_register_iface.
// ---------------------------------------------------------------------------

static CATBaseUnknown** lua_caa_to_obj(lua_State* L, int iIdx)
{
    void* p = lua_touserdata(L, iIdx);
    return (p != NULL) ? (CATBaseUnknown**)p : NULL;
}

CATBaseUnknown* lua_caa_check(lua_State* L, int iIdx, const char* iIfaceName)
{
    CATBaseUnknown** pp = lua_caa_to_obj(L, iIdx);
    if (pp == NULL)
        luaL_argerror(L, iIdx, "CAA object expected");
    if (!lua_getmetatable(L, iIdx))
        luaL_argerror(L, iIdx, "CAA object expected");

    int bFound = (iIfaceName == NULL);
    for (int i = 0; i < 32 && !bFound; i++)
    {
        lua_getfield(L, -1, "name");
        const char* pName = lua_tostring(L, -1);
        if (pName != NULL && strcmp(pName, iIfaceName) == 0)
            bFound = 1;
        lua_pop(L, 1);
        if (bFound)
            break;
        lua_getfield(L, -1, "__parent");
        if (lua_isnil(L, -1))
            break;
        lua_remove(L, -2);   // keep walking on the parent metatable
    }
    lua_settop(L, iIdx);     // drop metatable (and chain leftovers)

    if (!bFound)
        luaL_argerror(L, iIdx, "CAA interface mismatch");
    return *pp;
}

void lua_caa_push(lua_State* L, CATBaseUnknown* iObj, const char* iIfaceName)
{
    if (iObj == NULL)
    {
        lua_pushnil(L);
        return;
    }
    CATBaseUnknown** pp = (CATBaseUnknown**)lua_newuserdata(L, sizeof(CATBaseUnknown*));
    *pp = iObj;
    iObj->AddRef();

    if (iIfaceName != NULL && luaL_getmetatable(L, iIfaceName))
    {
        lua_setmetatable(L, -2);
        return;
    }
    if (iIfaceName != NULL)
        lua_pop(L, 1);   // nil pushed by the failed metatable lookup
    if (luaL_getmetatable(L, "CATBaseUnknown"))
    {
        lua_setmetatable(L, -2);
        return;
    }
    lua_pop(L, 1);       // no base metatable registered: leave bare userdata
}

// ---------------------------------------------------------------------------
// metatable handlers
// ---------------------------------------------------------------------------

static int lua_caa_qi(lua_State* L)
{
    // obj:qi("CATI2DWFFactory")
    CATBaseUnknown* pObj = lua_caa_check(L, 1, NULL);
    const char* pName = luaL_checkstring(L, 2);
    const GUID* pIID = lua_caa_gen_iid(pName);
    if (pIID == NULL)
        return luaL_error(L, "unknown CAA interface: %s", pName);

    CATBaseUnknown* pOut = NULL;
    HRESULT hr = pObj->QueryInterface(*pIID, (void**)&pOut);
    if (FAILED(hr) || pOut == NULL)
    {
        lua_pushnil(L);
        lua_pushfstring(L, "QI %s failed, hr=0x%08lx", pName, (unsigned long)hr);
        return 2;
    }
    lua_caa_push(L, pOut, pName);
    pOut->Release();   // lua_caa_push took its own reference
    return 1;
}

static int lua_caa_gc(lua_State* L)
{
    CATBaseUnknown** pp = lua_caa_to_obj(L, 1);
    if (pp != NULL && *pp != NULL)
    {
        (*pp)->Release();
        *pp = NULL;
    }
    return 0;
}

static int lua_caa_tostring(lua_State* L)
{
    CATBaseUnknown** pp = lua_caa_to_obj(L, 1);
    lua_getmetatable(L, 1);
    lua_getfield(L, -1, "name");
    const char* pName = lua_tostring(L, -1);
    lua_pushfstring(L, "%s@%p",
                    (pName != NULL) ? pName : "CATBaseUnknown",
                    (void*)((pp != NULL) ? *pp : NULL));
    return 1;
}

static int lua_caa_index(lua_State* L)
{
    // stack: userdata, key
    lua_getmetatable(L, 1);
    for (int i = 0; i < 32; i++)
    {
        lua_getfield(L, -1, "methods");
        lua_pushvalue(L, 2);
        lua_rawget(L, -2);               // methods[key]
        if (!lua_isnil(L, -1))
            return 1;                    // found
        lua_pop(L, 2);                   // pop nil and methods
        lua_getfield(L, -1, "__parent");
        if (lua_isnil(L, -1))
            break;
        lua_remove(L, -2);               // walk up to the parent metatable
    }
    lua_settop(L, 2);                    // drop metatable and chain leftovers

    const char* pKey = lua_tostring(L, 2);
    if (pKey != NULL && strcmp(pKey, "qi") == 0)
    {
        lua_pushcfunction(L, lua_caa_qi);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

// ---------------------------------------------------------------------------
// registration
// ---------------------------------------------------------------------------

void lua_caa_register_iface(lua_State* L, const char* iName,
                            const luaL_Reg* iMethods, const char* iParentName)
{
    luaL_newmetatable(L, iName);
    lua_pushstring(L, iName);
    lua_setfield(L, -2, "name");
    lua_pushcfunction(L, lua_caa_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, lua_caa_gc);
    lua_setfield(L, -2, "__gc");
    lua_pushcfunction(L, lua_caa_tostring);
    lua_setfield(L, -2, "__tostring");

    lua_newtable(L);
    if (iMethods != NULL)
    {
        for (const luaL_Reg* pReg = iMethods; pReg->name != NULL; pReg++)
        {
            lua_pushcfunction(L, pReg->func);
            lua_setfield(L, -2, pReg->name);
        }
    }
    lua_setfield(L, -2, "methods");

    if (iParentName != NULL && luaL_getmetatable(L, iParentName))
    {
        lua_setfield(L, -2, "__parent");
    }
    else if (iParentName != NULL)
    {
        lua_pop(L, 1);   // nil pushed by the failed metatable lookup
    }
    lua_pop(L, 1);       // metatable
}

// ---------------------------------------------------------------------------
// marshalling helpers
// ---------------------------------------------------------------------------

int lua_caa_hr(lua_State* L, long iHr, const char* iMethod)
{
    if (SUCCEEDED((HRESULT)iHr))
    {
        lua_pushinteger(L, 0);
        return 1;
    }
    lua_pushnil(L);
    lua_pushfstring(L, "%s failed, hr=0x%08lx",
                    (iMethod != NULL) ? iMethod : "CAA call",
                    (unsigned long)iHr);
    return 2;
}

int lua_caa_check_ustring(lua_State* L, int iIdx, CATUnicodeString& oStr)
{
    const char* p = luaL_checkstring(L, iIdx);
    oStr = CATUnicodeString(p);
    return 0;
}

void lua_caa_push_ustring(lua_State* L, const CATUnicodeString& iStr)
{
    const char* p = iStr.ConvertToChar();
    lua_pushstring(L, (p != NULL) ? p : "");
}

int lua_caa_check_double_array(lua_State* L, int iIdx, double* oBuf, int iMaxN)
{
    if (lua_isnumber(L, iIdx))
    {
        oBuf[0] = lua_tonumber(L, iIdx);
        return 1;
    }
    luaL_checktype(L, iIdx, LUA_TTABLE);
    int nCount = 0;
    for (int i = 1; i <= iMaxN; i++)
    {
        lua_rawgeti(L, iIdx, i);
        if (lua_isnil(L, -1))
        {
            lua_pop(L, 1);
            break;
        }
        oBuf[i - 1] = luaL_checknumber(L, -1);
        lua_pop(L, 1);
        nCount = i;
    }
    return nCount;
}

// ---------------------------------------------------------------------------
// "catia" session table (hand-written, not generated)
// ---------------------------------------------------------------------------

// catia.activePart() -> Part feature as a CATISpecObject
static int l_catia_activePart(lua_State* L)
{
    CATFrmEditor* pEditor = CATFrmEditor::GetCurrentEditor();
    if (pEditor == NULL)
    {
        lua_pushnil(L);
        lua_pushliteral(L, "no active editor");
        return 2;
    }
    CATDocument* pDoc = pEditor->GetDocument();
    if (pDoc == NULL)
    {
        lua_pushnil(L);
        lua_pushliteral(L, "no active document");
        return 2;
    }

    CATIContainerOfDocument* pCod = NULL;
    HRESULT hr = pDoc->QueryInterface(IID_CATIContainerOfDocument, (void**)&pCod);
    if (FAILED(hr) || pCod == NULL)
    {
        lua_pushnil(L);
        lua_pushfstring(L, "QI CATIContainerOfDocument failed, hr=0x%08lx", (unsigned long)hr);
        return 2;
    }
    CATIContainer* pContRaw = NULL;
    hr = pCod->GetSpecContainer(pContRaw);
    pCod->Release();
    if (FAILED(hr) || pContRaw == NULL)
    {
        lua_pushnil(L);
        lua_pushliteral(L, "no spec container");
        return 2;
    }
    CATIContainer_var spCont = pContRaw;
    pContRaw->Release();

    CATIPrtContainer* pPrtCont = NULL;
    hr = spCont->QueryInterface(IID_CATIPrtContainer, (void**)&pPrtCont);
    if (FAILED(hr) || pPrtCont == NULL)
    {
        lua_pushnil(L);
        lua_pushfstring(L, "QI CATIPrtContainer failed, hr=0x%08lx", (unsigned long)hr);
        return 2;
    }
    CATISpecObject_var spPart = pPrtCont->GetPart();
    pPrtCont->Release();
    if (spPart == NULL_var)
    {
        lua_pushnil(L);
        lua_pushliteral(L, "GetPart returned NULL");
        return 2;
    }
    lua_caa_push(L, spPart, "CATISpecObject");
    return 1;
}

// catia.update(specObj) -> CATIAPart::Update()
static int l_catia_update(lua_State* L)
{
    CATBaseUnknown* pObj = lua_caa_check(L, 1, NULL);
    CATIAPart* pPart = NULL;
    HRESULT hr = pObj->QueryInterface(IID_CATIAPart, (void**)&pPart);
    if (FAILED(hr) || pPart == NULL)
    {
        lua_pushnil(L);
        lua_pushfstring(L, "QI CATIAPart failed, hr=0x%08lx", (unsigned long)hr);
        return 2;
    }
    hr = pPart->Update();
    pPart->Release();
    return lua_caa_hr(L, hr, "CATIAPart::Update");
}

// ---------------------------------------------------------------------------
// Tree traversal entry helpers (Lua side implements the actual walk).
// Mirrors EDataVisitor's entry points: current document, generic
// interface conversion (QueryInterface), reference -> linked document.
// ---------------------------------------------------------------------------

// catia.currentDocument() -> "CATDocument" userdata (borrowed pointer)
static int l_catia_currentDocument(lua_State* L)
{
    CATFrmEditor* pEditor = CATFrmEditor::GetCurrentEditor();
    if (pEditor == NULL)
    {
        lua_pushnil(L);
        return 1;
    }
    CATDocument* pDoc = pEditor->GetDocument();
    if (pDoc == NULL)
    {
        lua_pushnil(L);
        return 1;
    }
    lua_caa_push(L, pDoc, "CATDocument");
    return 1;
}

// catia.qi(obj, "IfaceName") -> interface userdata or nil
// Generic QueryInterface: converts any registered CAA object.
static int l_catia_qi(lua_State* L)
{
    CATBaseUnknown* pObj = lua_caa_check(L, 1, NULL);
    const char* iName = luaL_checkstring(L, 2);
    const GUID* pIID = lua_caa_gen_iid(iName);
    if (pIID == NULL)
    {
        lua_pushnil(L);
        lua_pushfstring(L, "interface not registered: %s", iName);
        return 2;
    }
    CATBaseUnknown* pOut = NULL;
    HRESULT hr = pObj->QueryInterface(*pIID, (void**)&pOut);
    if (FAILED(hr) || pOut == NULL)
    {
        lua_pushnil(L);
        return 1;
    }
    lua_caa_push(L, pOut, iName);   // AddRef for the userdata
    pOut->Release();                // drop the QI reference
    return 1;
}

// catia.getDocument(obj) -> "CATDocument" userdata or nil
// obj: any CAA object supporting CATILinkableObject (e.g. a reference
// product). Equivalent to EDataVisitor's reference -> part document step.
static int l_catia_getDocument(lua_State* L)
{
    CATBaseUnknown* pObj = lua_caa_check(L, 1, NULL);
    CATILinkableObject* pLink = NULL;
    HRESULT hr = pObj->QueryInterface(IID_CATILinkableObject, (void**)&pLink);
    if (FAILED(hr) || pLink == NULL)
    {
        lua_pushnil(L);
        return 1;
    }
    CATDocument* pDoc = pLink->GetDocument();   // borrowed pointer
    pLink->Release();
    if (pDoc == NULL)
    {
        lua_pushnil(L);
        return 1;
    }
    lua_caa_push(L, pDoc, "CATDocument");
    return 1;
}

// ---------------------------------------------------------------------------
// Lua print redirection. Lua stdlib print writes to stdout, which is
// invisible in the GUI cnext process. Replace it with a dual writer:
//   1) printf + fflush -> CATIA console (requires CNEXTOUTPUT=console)
//   2) append to log file -> always reliable
// File path: env CATIALUA_PRINT_FILE, or the fixed default below.
// ---------------------------------------------------------------------------

static void lua_caa_print_file(const char* iText)
{
    const char* pEnv = getenv("CATIALUA_PRINT_FILE");
    const char* pPath = (pEnv != NULL && pEnv[0] != '\0')
                        ? pEnv
                        : "D:/eric/coding/EEditor2/project/CatiaLua/win_b64/CustomData/lua_print.log";
    FILE* f = fopen(pPath, "a");
    if (f != NULL)
    {
        fputs(iText, f);
        fflush(f);
        fclose(f);
    }
}

static int lua_caa_print(lua_State* L)
{
    int n = lua_gettop(L);
    char buf[4096];
    size_t pos = 0;
    for (int i = 1; i <= n && pos + 1 < sizeof(buf); i++)
    {
        size_t len = 0;
        const char* s = luaL_tolstring(L, i, &len);
        if (s == NULL)
            s = "";
        if (i > 1 && pos + 1 < sizeof(buf))
            buf[pos++] = '\t';
        size_t rem = sizeof(buf) - pos - 1;
        size_t cp = (len < rem) ? len : rem;
        if (cp > 0)
            memcpy(buf + pos, s, cp);
        pos += cp;
        lua_pop(L, 1);   // pop the luaL_tolstring result
    }
    buf[pos] = '\0';

    // 1) CATIA console (CNEXTOUTPUT=console must be set before startup)
    printf("%s\n", buf);
    fflush(stdout);
    // 2) log file (always works)
    lua_caa_print_file(buf);
    lua_caa_print_file("\n");
    return 0;
}

// ---------------------------------------------------------------------------
// entry point
// ---------------------------------------------------------------------------

int lua_caa_open(lua_State* L)
{
    // Root metatable of every interface chain.
    lua_caa_register_iface(L, "CATBaseUnknown", NULL, NULL);
    // Framework class carrier: no methods, used with catia.qi only.
    lua_caa_register_iface(L, "CATDocument", NULL, NULL);

    // Session helpers.
    lua_newtable(L);
    lua_pushcfunction(L, l_catia_activePart);
    lua_setfield(L, -2, "activePart");
    lua_pushcfunction(L, l_catia_update);
    lua_setfield(L, -2, "update");
    lua_pushcfunction(L, l_catia_currentDocument);
    lua_setfield(L, -2, "currentDocument");
    lua_pushcfunction(L, l_catia_qi);
    lua_setfield(L, -2, "qi");
    lua_pushcfunction(L, l_catia_getDocument);
    lua_setfield(L, -2, "getDocument");
    lua_setglobal(L, "catia");

    // Replace Lua stdlib print: CATIA console + log file (see above).
    lua_pushcfunction(L, lua_caa_print);
    lua_setglobal(L, "print");

    // Generated interface registration.
    lua_caa_gen_open(L);
    return 0;
}
