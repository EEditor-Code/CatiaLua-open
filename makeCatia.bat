@echo off
setlocal
REM Build CatiaLua via RadeBuildBridge (VS2005 add-in, port 19528).
REM Usage:
REM   makeCatia.bat
REM   makeCatia.bat --cmd "mkmk -u CatiaLuaCmd.m"
python "%~dp0build_catialua.py" %*
exit /b %errorlevel%