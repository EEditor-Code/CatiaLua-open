// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2026 nwpu052009 <ljwangcumtb@163.com>

#ifndef TOOLS_MANAGER_H
#define TOOLS_MANAGER_H


#include <string.h>
#include "CATUnicodeString.h"
#include "CATDialog.h"

struct ToolsManagerData;
/**This class manager some tools for CAA development.
 * 1.xxx
 * 2.xxx
 * */
class ToolsManager
{
public:

    static ToolsManager* instance();
    void showLog(CATDialog *iParent);

    ToolsManagerData* m_data;
    CATUnicodeString m_catLogStr;
protected:
    ToolsManager();
    virtual ~ToolsManager();

    

    static ToolsManager* m_instance;

};

#define ELOG(fmt, ...) log_log(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define ELOG_CAT(CATSTR) ToolsManager::instance()->m_catLogStr.Append(CATSTR);ToolsManager::instance()->m_catLogStr.Append("\n")

void log_log(const char* file, int line, const char* fmt, ...);

#endif //TOOLS_MANAGER_H 
