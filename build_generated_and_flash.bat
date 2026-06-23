@echo off
setlocal

set PROJECT_DIR=%~dp0

cd /d "%PROJECT_DIR%"
call build_and_flash.bat
