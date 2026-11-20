# CatiaLua

> CATIA V5R18 CAA 二次开发项目 · 在 CATIA 内嵌 Lua 5.3.2
> CATIA V5R18 CAA add-in with embedded Lua 5.3.2

[Gitee](https://gitee.com/eeditor/catia-lua) · [GitHub](https://github.com/EEditor-Code/CatiaLua-open) · [官网 / Website](https://eeditor.com.cn/)

[中文](#zh) | [English](#en)

---

<a id="zh"></a>

## 中文

CATIA V5R18 CAA 二次开发项目（C++/COM）。在 CATIA 内嵌 Lua 5.3.2，通过 `LuaCaaCore`
把常用 CATIA COM 接口暴露给 Lua，并提供界面按钮与本地 TCP RPC 两种执行入口。

### 当前功能

- 工具栏/菜单 `CatiaLua` → `Run Lua`：执行 `win_b64\CustomData\main.lua` 中的 `main()`。
- Lua 引擎：`LuaEngine`（初始化、加载脚本、执行函数、执行代码字符串）。
- CAA 绑定：`LuaCaaCore` + `lua_caa_gen_*.cpp`，覆盖 Part/Product/Document/Sketch 等常用接口。
- 本地 RPC：`LuaRpcServer` 监听 `127.0.0.1:19529`，由 `CatiaLuaAddin` 启动/停止。
- 脚本不存在时自动生成示例 `main.lua`。

### RPC 协议

一行一条命令，一个连接执行一条命令：

| 命令 | 说明 |
|---|---|
| `STATUS` | 返回进程与当前文档状态 |
| `OPEN <path>` | 通过 CATIA 文档服务打开文档 |
| `LUA <code>` | 执行一段 Lua 代码（换行用 `\n` 转义） |
| `LUAFILE <path>` | 加载脚本并执行 `main()` |
| `QUIT` | 停止 RPC 服务 |

成功返回 `DONE 0`；失败返回 `ERROR <msg>` 后跟 `DONE 1`。

### 项目结构

```text
CatiaLua/
├── CatiaLua/                     # CAA 框架（框架名 CatiaLua）
│   ├── CatiaLuaAddin.m           # Addin：工具栏/菜单注册、RPC 启停
│   ├── CatiaLuaCmd.m             # 命令：CmdLuaRunner/DlgLuaRunner、LuaEngine、LuaCaaCore、LuaRpcServer
│   ├── IdentityCard/             # 框架依赖声明
│   └── CNext/                    # dico 注册、msgcatalog、图标
├── buildCAAByCli.bat             # 命令行 mkmk 构建（推荐）
├── build_catialua.py             # 通过 RadeBuildBridge 构建
├── bridge_client.py              # RadeBuildBridge TCP 客户端
├── runCatia.py                   # 解析 V5R18.txt 并启动 CATIA
├── makeCatia.bat                 # 转发到 build_catialua.py
├── make_local_mingw.bat          # CMake + MinGW 配置（仅供 clangd）
├── verify_dll.py                 # 校验产物 DLL 是否为 64 位
├── V5R18.txt.example             # CATIA 环境变量模板
├── Install_config_*_*.example    # CATIA 安装路径模板
└── CMakeLists.txt                # 仅用于 clangd（compile_commands.json）
```

### 前置条件（重要）

本项目依赖以下**非开源**组件，仓库**不包含**：

- CATIA V5R18 + CAA RADE B18
- Visual Studio 2005（CAA R18 工具链）
- Lua 5.3.2（需要 `lua.lib` / `lua.dll`，建议 64 位）

另外，CAA 工作区根目录的 `CATIAV5Level.lvl` 属于 Dassault Systèmes 的 CAA SDK，
**因许可原因不随仓库分发**；请从你的 CAA 安装目录复制到仓库根目录后再构建。

### 环境准备

1. 复制环境模板并填写本机路径：
   ```bat
   copy V5R18.txt.example V5R18.txt
   copy Install_config_win_b64.example Install_config_win_b64
   ```
   把其中的 `<CATIA_INSTALL>`、`<WORKSPACE>` 替换为实际路径。
2. 从 CAA 安装复制 `CATIAV5Level.lvl` 到仓库根目录。
3. 准备 Lua 5.3.2：
   - 设置 `LUA_ROOT=<lua-5.3.2 安装目录>`（该目录下应有 `include\` 与 `lib\`）；
   - 运行时把 `lua.dll` 放到 `win_b64\code\bin\`（仓库不含该 DLL）。
4. 可选环境变量：
   - `RADE_ROOT`：默认 `D:\CATIA\RADE\B18`
   - `VS8_ROOT`：默认 `D:\Program Files (x86)\Microsoft Visual Studio 8`
   - `DEVENV_PATH`：`build_catialua.py` 启动 VS2005 用
   - `CATIA_INSTALL` / `CATENV_DIR`：`runCatia.py` 用

### 构建

命令行 `mkmk`（推荐）：

```bat
set LUA_ROOT=D:\path\to\lua-5.3.2
buildCAAByCli.bat
```

或通过 RadeBuildBridge：

```bat
python build_catialua.py
python build_catialua.py --cmd "mkmk -u CatiaLuaCmd.m"
```

产物：

- `win_b64\code\bin\CatiaLuaAddin.dll`
- `win_b64\code\bin\CatiaLuaCmd.dll`
- `win_b64\code\bin\lua.dll`（需自行提供，不入库）

### 运行

```bat
python runCatia.py
```

如需验证 RPC：

```powershell
$c = New-Object Net.Sockets.TcpClient
$c.Connect('127.0.0.1', 19529)
$s = $c.GetStream()
$w = New-Object IO.StreamWriter($s); $w.NewLine = "`n"
$w.WriteLine('STATUS'); $w.Flush()
$r = New-Object IO.StreamReader($s)
$r.ReadToEnd()
$c.Close()
```

### 校验产物

```bat
python verify_dll.py
```

默认检查脚本目录下的 `win_b64\code\bin`；可用环境变量 `CATIALUA_BIN` 覆盖。

### 说明

- `CNext/code/dictionary/CatiaLua.dico` 注册 `CatiaLuaAddin`；`win_b64/code/dictionary/CatiaLua.dico` 需保持一致。
- `win_b64/CustomData/` 用于存放 `main.lua` 及脚本运行数据。
- `CMakeLists.txt` 仅用于生成 `compile_commands.json` 供 clangd 使用，不是真实 CAA 构建入口；
  可通过环境变量 `EEditor2_DEPEND` 指向依赖目录（含 `caa/CAAIncludeFiles`）。
- `V5R18.txt`、`Install_config_win_b64`、`Install_config_intel_a`、`CATIAV5Level.lvl` 均为本机/许可相关文件，不参与版本管理。

### 许可

本仓库代码以 **GNU Lesser General Public License v2.1 or later**（LGPL-2.1-or-later）发布，
版权归 **nwpu052009** `<ljwangcumtb@163.com>` 所有。

CATIA / CAA SDK 为 Dassault Systèmes 的专有软件，不随本仓库分发；使用本项目需自行获得相应授权。

说明：`CatiaLua/*.m/` 与 `CatiaLua/IdentityCard/` 中部分骨架文件由 CAA 向导生成，
保留 Dassault 版权头，受 Dassault CAA 许可约束；本仓库的 LGPL 适用于其余原创代码
（即不带 Dassault 版权头的文件）。

### 链接

- Gitee: https://gitee.com/eeditor/catia-lua
- GitHub: https://github.com/EEditor-Code/CatiaLua-open
- 官网: https://eeditor.com.cn/

---

<a id="en"></a>

## English

A CATIA V5R18 CAA add-in (C++/COM) that embeds Lua 5.3.2 inside CATIA. Through `LuaCaaCore`,
common CATIA COM interfaces are exposed to Lua, with two execution paths: a UI command and a
local TCP RPC server.

### Features

- Toolbar/menu `CatiaLua` → `Run Lua`: executes `main()` from `win_b64\CustomData\main.lua`.
- Lua engine: `LuaEngine` (initialize, load script, execute function, execute code string).
- CAA bindings: `LuaCaaCore` + `lua_caa_gen_*.cpp`, covering Part/Product/Document/Sketch interfaces.
- Local RPC: `LuaRpcServer` listens on `127.0.0.1:19529`, started/stopped by `CatiaLuaAddin`.
- Automatically generates an example `main.lua` when the script is missing.

### RPC protocol

One command per line; one command per connection:

| Command | Description |
|---|---|
| `STATUS` | Return process and current document status |
| `OPEN <path>` | Open a document through the CATIA document service |
| `LUA <code>` | Execute a Lua snippet (escape newlines as `\n`) |
| `LUAFILE <path>` | Load a script and execute `main()` |
| `QUIT` | Stop the RPC server |

Success returns `DONE 0`; failure returns `ERROR <msg>` followed by `DONE 1`.

### Project layout

```text
CatiaLua/
├── CatiaLua/                     # CAA framework (framework name: CatiaLua)
│   ├── CatiaLuaAddin.m           # Add-in: toolbar/menu registration, RPC start/stop
│   ├── CatiaLuaCmd.m             # Commands: CmdLuaRunner/DlgLuaRunner, LuaEngine, LuaCaaCore, LuaRpcServer
│   ├── IdentityCard/             # Framework dependency declarations
│   └── CNext/                    # dico registration, msgcatalog, icons
├── buildCAAByCli.bat             # Command-line mkmk build (recommended)
├── build_catialua.py             # Build via RadeBuildBridge
├── bridge_client.py              # RadeBuildBridge TCP client
├── runCatia.py                   # Parse V5R18.txt and launch CATIA
├── makeCatia.bat                 # Forwards to build_catialua.py
├── make_local_mingw.bat          # CMake + MinGW config (clangd only)
├── verify_dll.py                 # Check that output DLLs are 64-bit
├── V5R18.txt.example             # CATIA environment template
├── Install_config_*_*.example    # CATIA install-path templates
└── CMakeLists.txt                # clangd only (compile_commands.json)
```

### Prerequisites (important)

This project depends on the following **non-open-source** components, which are **not included**:

- CATIA V5R18 + CAA RADE B18
- Visual Studio 2005 (CAA R18 toolchain)
- Lua 5.3.2 (`lua.lib` / `lua.dll`, 64-bit recommended)

In addition, `CATIAV5Level.lvl` in the CAA workspace root is part of the Dassault Systèmes
CAA SDK and is **not redistributed for licensing reasons**. Copy it from your CAA installation
into the repository root before building.

### Setup

1. Copy the templates and fill in local paths:
   ```bat
   copy V5R18.txt.example V5R18.txt
   copy Install_config_win_b64.example Install_config_win_b64
   ```
   Replace `<CATIA_INSTALL>` and `<WORKSPACE>` with actual paths.
2. Copy `CATIAV5Level.lvl` from your CAA installation to the repository root.
3. Prepare Lua 5.3.2:
   - Set `LUA_ROOT=<lua-5.3.2 install>` (it must contain `include\` and `lib\`);
   - Put `lua.dll` into `win_b64\code\bin\` at runtime (not included in this repository).
4. Optional environment variables:
   - `RADE_ROOT`: default `D:\CATIA\RADE\B18`
   - `VS8_ROOT`: default `D:\Program Files (x86)\Microsoft Visual Studio 8`
   - `DEVENV_PATH`: used by `build_catialua.py` to start VS2005
   - `CATIA_INSTALL` / `CATENV_DIR`: used by `runCatia.py`

### Build

Command-line `mkmk` (recommended):

```bat
set LUA_ROOT=D:\path\to\lua-5.3.2
buildCAAByCli.bat
```

Or via RadeBuildBridge:

```bat
python build_catialua.py
python build_catialua.py --cmd "mkmk -u CatiaLuaCmd.m"
```

Outputs:

- `win_b64\code\bin\CatiaLuaAddin.dll`
- `win_b64\code\bin\CatiaLuaCmd.dll`
- `win_b64\code\bin\lua.dll` (provide it yourself; not versioned)

### Run

```bat
python runCatia.py
```

To verify the RPC server:

```powershell
$c = New-Object Net.Sockets.TcpClient
$c.Connect('127.0.0.1', 19529)
$s = $c.GetStream()
$w = New-Object IO.StreamWriter($s); $w.NewLine = "`n"
$w.WriteLine('STATUS'); $w.Flush()
$r = New-Object IO.StreamReader($s)
$r.ReadToEnd()
$c.Close()
```

### Verify output

```bat
python verify_dll.py
```

By default it checks `win_b64\code\bin` under the script directory; override with `CATIALUA_BIN`.

### Notes

- `CNext/code/dictionary/CatiaLua.dico` registers `CatiaLuaAddin`; `win_b64/code/dictionary/CatiaLua.dico` must stay consistent.
- `win_b64/CustomData/` holds `main.lua` and script runtime data.
- `CMakeLists.txt` only generates `compile_commands.json` for clangd; it is not the real CAA build entry.
  Set `EEditor2_DEPEND` to the dependency directory (containing `caa/CAAIncludeFiles`).
- `V5R18.txt`, `Install_config_win_b64`, `Install_config_intel_a` and `CATIAV5Level.lvl` are local/licensing-related and are not versioned.

### License

This repository is released under the **GNU Lesser General Public License v2.1 or later**
(LGPL-2.1-or-later). Copyright (C) 2026 **nwpu052009** `<ljwangcumtb@163.com>`.

CATIA / CAA SDK are proprietary Dassault Systèmes software and are not distributed with this
repository; you must obtain the appropriate license to use them.

Note: some skeleton files under `CatiaLua/*.m/` and `CatiaLua/IdentityCard/` are generated by
the CAA wizard and retain Dassault copyright headers; they remain subject to the Dassault CAA
license. The LGPL applies to the other original code (files without a Dassault copyright header).

### Links

- Gitee: https://gitee.com/eeditor/catia-lua
- GitHub: https://github.com/EEditor-Code/CatiaLua-open
- Website: https://eeditor.com.cn/
