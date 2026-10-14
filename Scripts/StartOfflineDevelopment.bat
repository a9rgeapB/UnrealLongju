@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0..") do set "MT2_PROJECT_DIR=%%~fI"
set "MT2_OFFLINE_DIR=%MT2_PROJECT_DIR%\Saved\StagedOffline\Windows"
set "MT2_OFFLINE_EXE=%MT2_OFFLINE_DIR%\UnrealLongju.exe"
if not exist "%MT2_OFFLINE_EXE%" (
    echo [OfflineStart] Missing executable: "%MT2_OFFLINE_EXE%"
    echo Run Scripts\BuildOfflineDevelopment.bat first.
    exit /b 1
)
start "UnrealLongju Offline" /D "%MT2_OFFLINE_DIR%" "%MT2_OFFLINE_EXE%" -log %*
if errorlevel 1 exit /b 1
exit /b 0
