// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2026 nwpu052009 <ljwangcumtb@163.com>

#include "ToolsManager.h"
#include "CATUnicodeString.h"

#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <CATDlgNotify.h>


struct ToolsManagerData
{
    // char logStr[300];
};

ToolsManager* ToolsManager::m_instance = NULL;

ToolsManager::ToolsManager()
{
    m_data = new ToolsManagerData;
}

ToolsManager::~ToolsManager()
{
    delete m_data;
}
    
ToolsManager* ToolsManager::instance()
{
    if(m_instance == NULL)
    {
        m_instance = new ToolsManager();
    }
    return m_instance;
}

void ToolsManager::showLog(CATDialog *iParent)
{
    CATDlgNotify* pNotify = new CATDlgNotify(iParent, "MyWarningDlg", CATDlgNfyWarning | CATDlgNfyOKCancel);
    pNotify->SetText("notification");

    // ELOG("Hello World");

    int nResult = pNotify->DisplayBlocked(
                        ToolsManager::instance()->m_catLogStr, 
                        CATUnicodeString("title"));

    this->m_catLogStr = "";

}

void log_log(const char* file, int line, const char* fmt, ...) 
{
    // 获取当前时间
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

    // 提取文件名（去除路径）
    const char* base_name = strrchr(file, '\\');
    base_name = base_name ? base_name + 1 : file;

    // 组合日志头信息
    char strInfo[30];
    sprintf(strInfo, "[%s][%s:%d] ", time_str, base_name, line);
    // fprintf(stderr, "[%s][%s:%d] ", time_str, base_name, line);

    // 处理可变参数
    va_list args;
    va_start(args, fmt);
    // vfprintf(stderr, fmt, args);
    char strData[300];
    sprintf(strData, fmt, args);
    char strFinal[400];
    sprintf(strFinal, "%s %s\n", strInfo, strData);
    va_end(args);

    // add to CATUnicodeString
    ToolsManager::instance()->m_catLogStr.Append(CATUnicodeString(strFinal));
    // fprintf(stderr, "\n");
    // fflush(stderr);  // 确保实时输出
}
