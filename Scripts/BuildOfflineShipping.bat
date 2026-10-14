@echo off
setlocal EnableExtensions DisableDelayedExpansion

rem Package the offline Game target, not the online Client or Server targets.
if not defined UE_ROOT set "UE_ROOT=F:\Engine2"
for %%I in ("%~dp0..") do set "MT2_PROJECT_DIR=%%~fI"
set "MT2_RUNUAT=%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat"
set "MT2_UPROJECT=%MT2_PROJECT_DIR%\UnrealLongju.uproject"
set "MT2_OFFLINE_STAGE=%MT2_PROJECT_DIR%\Saved\StagedOfflineShipping"
if not exist "%MT2_RUNUAT%" (
    echo [OfflineShippingBuild] Missing RunUAT: "%MT2_RUNUAT%"
    exit /b 1
)
if not exist "%MT2_UPROJECT%" (
    echo [OfflineShippingBuild] Missing project: "%MT2_UPROJECT%"
    exit /b 1
)
call "%MT2_RUNUAT%" BuildCookRun -project="%MT2_UPROJECT%" -target=UnrealLongju -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -iterativecooking -iterativedeploy -nodebuginfo -stagingdirectory="%MT2_OFFLINE_STAGE%" -nop4 -utf8output
if errorlevel 1 exit /b 1
echo [OfflineShippingBuild] Package: "%MT2_OFFLINE_STAGE%\Windows"
exit /b 0
