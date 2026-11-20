// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2026 nwpu052009 <ljwangcumtb@163.com>

//===========================================================================
// LuaRpcServer.h
// TCP RPC server running inside the CATIA process (CatiaLuaCmd.dll).
//
// Protocol (one connection per command, line-terminated by '\n'):
//   STATUS               -> "OK pid=<pid> doc=<displayName>"
//   OPEN <path>          -> open document via CATDocumentServices
//   LUA <code>           -> execute Lua code (newlines escaped as \\n)
//   LUAFILE <path>       -> load script file and run main()
//   QUIT                 -> stop the server
// Response: "DONE 0" on success; on failure "ERROR <msg>" then "DONE 1".
//
// Threading: a worker thread does socket I/O only. Each command is posted
// to a hidden message window so it executes on the CATIA main thread
// (document APIs and the Lua engine are main-thread-bound). The worker
// waits on an event for the main thread result.
//===========================================================================
#ifndef LuaRpcServer_H
#define LuaRpcServer_H

#include <string>
#include <winsock2.h>

class LuaRpcServer
{
public:
    static LuaRpcServer* GetInstance();

    bool Start(unsigned short iPort);
    void Stop();

    // Called from the hidden-window proc (free function) on the CATIA
    // main thread. Public because RpcWndProc is not a member.
    void ExecuteOnMainThread(std::string* pCmd);

private:
    LuaRpcServer();
    ~LuaRpcServer();
    LuaRpcServer(const LuaRpcServer&);
    LuaRpcServer& operator=(const LuaRpcServer&);

    static unsigned __stdcall WorkerThread(void* pv);
    void WorkerLoop();
    void HandleConnection(SOCKET sock);
    bool PostToMainThread(std::string* pCmd);

    bool InitHiddenWindow();
    void CleanupHiddenWindow();

    HWND     m_hwnd;
    SOCKET   m_listenSock;
    HANDLE   m_hThread;
    HANDLE   m_hDone;
    volatile bool m_running;
    std::string m_response;   // filled on the main thread, read after m_hDone
    int      m_exitCode;
};

#endif // LuaRpcServer_H
