@echo off
REM Double-click this to run Bundle-Sources.ps1 in the same folder.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Bundle-Sources.ps1" %*
pause
