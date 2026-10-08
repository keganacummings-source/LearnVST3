@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0create-v1-branch.ps1"
if errorlevel 1 pause
