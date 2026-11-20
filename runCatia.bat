@echo off
setlocal
python "%~dp0runCatia.py" %*
exit /b %errorlevel%
