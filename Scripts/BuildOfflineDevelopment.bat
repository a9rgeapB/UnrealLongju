@echo off
setlocal EnableExtensions DisableDelayedExpansion

rem Package the Game target, not the online Client or Server targets.
if not defined UE_ROOT set "UE_ROOT=F:\Engine2"
for %%I in ("%~dp0..") do set "MT2_PROJECT_DIR=%%~fI"
set "MT2_RUNUAT=%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat"
set "MT2_UPROJECT=%MT2_PROJECT_DIR%\UnrealLongju.uproject"
set "MT2_OFFLINE_STAGE=%MT2_PROJECT_DIR%\Saved\StagedOffline"
if not exist "%MT2_RUNUAT%" (
    echo [OfflineBuild] Missing RunUAT: "%MT2_RUNUAT%"
    exit /b 1
)
if not exist "%MT2_UPROJECT%" (
    echo [OfflineBuild] Missing project: "%MT2_UPROJECT%"
    exit /b 1
)
call "%MT2_RUNUAT%" BuildCookRun -project="%MT2_UPROJECT%" -target=UnrealLongju -platform=Win64 -clientconfig=Development -build -cook -stage -pak -iterativecooking -iterativedeploy -stagingdirectory="%MT2_OFFLINE_STAGE%" -nop4 -utf8output
if errorlevel 1 exit /b 1
echo [OfflineBuild] Package: "%MT2_OFFLINE_STAGE%\Windows"
exit /b 0
