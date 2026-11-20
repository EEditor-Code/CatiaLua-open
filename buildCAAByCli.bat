@echo off
setlocal EnableExtensions

REM ============================================================
REM  buildCAAByCli.bat - build CatiaLua CAA workspace from CLI
REM  (no VS2005 add-in / no RadeBuildBridge required)
REM
REM  Usage:
REM    buildCAAByCli.bat                              -> mkmk -a -u (whole workspace)
REM    buildCAAByCli.bat CatiaLua\CatiaLuaCmd.m -u    -> build one module
REM    buildCAAByCli.bat mkmk -a -u -g                -> full command override
REM
REM  Notes:
REM    - Module targets must be framework-qualified (e.g. CatiaLua\CatiaLuaCmd.m).
REM      A bare "CatiaLuaCmd.m" makes mkmk report "no framework in workspace".
REM    - mkmk 2.18.3 can return exit code 0 even when some module failed, so
REM      this script also scans the log for "-ERROR", "fatal error", etc.
REM
REM  Optional env overrides:
REM    set RADE_ROOT=D:\CATIA\RADE\B18
REM    set VS8_ROOT=D:\Program Files (x86)\Microsoft Visual Studio 8
REM
REM  Note: this file intentionally avoids multi-line ( ... ) blocks because
REM  VS/VS80COMNTOOLS paths contain "(x86)", which breaks cmd parsing when
REM  expanded inside a parenthesized block.
REM ============================================================

REM ---- workspace = directory of this script ----
set "WS=%~dp0"
if "%WS:~-1%"=="\" set "WS=%WS:~0,-1%"

REM ---- RADE root (override via RADE_ROOT) ----
if not defined RADE_ROOT set "RADE_ROOT=D:\CATIA\RADE\B18"
set "RADE_INTEL=%RADE_ROOT%\intel_a"
set "NTSETENV=%RADE_INTEL%\code\command\NTsetenv.bat"

if not exist "%NTSETENV%" goto err_no_ntsetenv
if not exist "%WS%\CatiaLua\CatiaLuaCmd.m" goto err_no_workspace
if not exist "%WS%\CATIAV5Level.lvl" goto err_no_level

REM ---- VS2005 root: VS8_ROOT > VS80COMNTOOLS > default ----
if not defined VS8_ROOT if defined VS80COMNTOOLS for %%I in ("%VS80COMNTOOLS%..\..") do set "VS8_ROOT=%%~fI"
if not defined VS8_ROOT set "VS8_ROOT=D:\Program Files (x86)\Microsoft Visual Studio 8"
if not exist "%VS8_ROOT%\VC\bin\amd64" goto err_no_vs8

REM ---- RADE environment ----
set "MkmkINSTALL_PATH=%RADE_ROOT%"
set "MkmkROOT_PATH=%RADE_INTEL%"
if not defined APPDATA set "APPDATA=%USERPROFILE%\AppData\Roaming"
set "RADECATSettingPath=%APPDATA%\DassaultSystemes\RADE"
if not exist "%RADECATSettingPath%" mkdir "%RADECATSettingPath%" >nul 2>&1

call "%NTSETENV%"
if errorlevel 1 goto err_ntsetenv

REM ---- force 64-bit / win_b64 target ----
set "MkmkOS_BitMode=64"
set "MkmkOS_Buildtime=win_b64"
set "MkmkOS_VAR=win_b64"
set "BLD_HOST_OS=win_b64"

REM ---- VS2005 x64 compiler paths (same values the RADE VS2005 plugin uses) ----
set "MkmkMSVCADDON_PATH_80=%VS8_ROOT%\VC\bin\amd64"
set "MkmkMSVCADDON_INCLUDE_80=%VS8_ROOT%\VC\include;%VS8_ROOT%\VC\atlmfc\include;%VS8_ROOT%\VC\PlatformSDK\include"
set "MkmkMSVCADDON_LIB_80=%VS8_ROOT%\VC\lib\amd64;%VS8_ROOT%\VC\atlmfc\lib\amd64;%VS8_ROOT%\VC\PlatformSDK\lib\amd64"

cd /d "%WS%"

REM ---- build command: default all, or pass through to mkmk ----
set "MKCMD=mkmk %*"
if "%~1"=="" set "MKCMD=mkmk -a -u"
if /i "%~1"=="mkmk" set "MKCMD=%*"

set "LOG_DIR=%WS%\temp"
if not exist "%LOG_DIR%" mkdir "%LOG_DIR%" >nul 2>&1
set "LOG=%LOG_DIR%\buildCAAByCli_%RANDOM%%RANDOM%.log"

echo ============================================================
echo  Workspace : %WS%
echo  RADE      : %RADE_ROOT%
echo  VS2005    : %VS8_ROOT%
echo  Command   : %MKCMD%
echo  Log       : %LOG%
echo ============================================================
echo.

REM Stream output to console + log (ANSI), and flag errors.
REM mkmk 2.18.3 can return 0 even when a module failed, so match log lines.
set "FAILED=0"
call %MKCMD% 2>&1 | powershell -NoProfile -ExecutionPolicy Bypass -Command "$w=New-Object IO.StreamWriter('%LOG%',$false,[Text.Encoding]::Default);$f=$false;$input|ForEach-Object{$w.WriteLine($_);Write-Host $_;if($_ -match '-ERROR|fatal error|error C\d|error LNK\d'){$f=$true}};$w.Close();if($f){exit 1}else{exit 0}"
if errorlevel 1 set "FAILED=1"

echo.
if "%FAILED%"=="0" echo [OK] build finished
if "%FAILED%"=="1" echo [FAIL] build failed, see log: %LOG%
exit /b %FAILED%

:err_no_ntsetenv
echo [ERROR] NTsetenv.bat not found: %NTSETENV%
echo         Set RADE_ROOT to your RADE install, for example:
echo         set RADE_ROOT=D:\CATIA\RADE\B18
exit /b 1

:err_no_workspace
echo [ERROR] Not a CatiaLua workspace - CatiaLua\CatiaLuaCmd.m missing in:
echo         %WS%
exit /b 1

:err_no_level
echo [ERROR] CATIAV5Level.lvl missing in:
echo         %WS%
echo         This file belongs to the CATIA CAA SDK and is not redistributed.
echo         Copy it from your CAA installation (e.g. CAAIncludeFiles) to the workspace root.
exit /b 1

:err_no_vs8
echo [ERROR] VS2005 x64 tools not found under:
echo         %VS8_ROOT%\VC\bin\amd64
echo         Set VS8_ROOT to your Visual Studio 2005 install.
exit /b 1

:err_ntsetenv
echo [ERROR] NTsetenv.bat failed: %NTSETENV%
exit /b 1
