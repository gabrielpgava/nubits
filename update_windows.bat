@echo off
setlocal

rem The worker runs from %TEMP% so Windows can replace this script during update.
if /i not "%~1"=="--worker" (
    set "WORKER=%TEMP%\NuBits-update-%RANDOM%-%RANDOM%.bat"
    copy /y "%~f0" "%WORKER%" >nul
    if errorlevel 1 exit /b 1
    start "" /min "%COMSPEC%" /d /c ""%WORKER%" --worker "%~1" "%~2" "%~3" "%~dp0""
    exit /b 0
)

set "ASSET_PATH=%~2"
set "PROCESS_ID=%~3"
set "TARGET=%~4"
set "ROOT_DIR=%~5"

if not defined ASSET_PATH exit /b 1
if not exist "%ASSET_PATH%" exit /b 1
if not defined PROCESS_ID exit /b 1
if /i not "%TARGET%"=="gui" if /i not "%TARGET%"=="daemon" exit /b 1
if not defined ROOT_DIR exit /b 1

echo Waiting for NuBits process %PROCESS_ID% to shut down...
set "NUBITS_PROCESS_ID=%PROCESS_ID%"
powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -Command "$id=[int]$env:NUBITS_PROCESS_ID; for($i=0; $i -lt 120; $i++){ if(-not (Get-Process -Id $id -ErrorAction SilentlyContinue)){ exit 0 }; Start-Sleep -Seconds 1 }; exit 1"
if errorlevel 1 (
    echo NuBits did not stop; update cancelled.
    exit /b 1
)

set "TEMP_DIR=%TEMP%\NuBits-update-%RANDOM%-%RANDOM%"
set "NUBITS_UPDATE_ARCHIVE=%ASSET_PATH%"
set "NUBITS_UPDATE_TEMP=%TEMP_DIR%"
set "NUBITS_UPDATE_TARGET=%TARGET%"
set "NUBITS_UPDATE_ROOT=%ROOT_DIR%"
powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -Command "$ErrorActionPreference='Stop'; $archive=$env:NUBITS_UPDATE_ARCHIVE; $temp=$env:NUBITS_UPDATE_TEMP; $target=$env:NUBITS_UPDATE_TARGET; $root=$env:NUBITS_UPDATE_ROOT; New-Item -ItemType Directory -Path $temp | Out-Null; Expand-Archive -LiteralPath $archive -DestinationPath $temp -Force; if(!(Test-Path (Join-Path $temp 'nud.exe'))){throw 'Package does not contain nud.exe'}; if($target -eq 'gui' -and !(Test-Path (Join-Path $temp 'nu.exe'))){throw 'Package does not contain nu.exe'}; Copy-Item -Path (Join-Path $temp '*') -Destination $root -Recurse -Force; Remove-Item -LiteralPath $temp -Recurse -Force"
if errorlevel 1 (
    echo Failed to extract or install the verified NuBits update.
    exit /b 1
)

if /i "%TARGET%"=="gui" (
    start "" "%ROOT_DIR%nu.exe"
) else (
    start "" /min "%ROOT_DIR%nud.exe"
)

rem Remove the temporary worker after this batch process exits.
set "NUBITS_UPDATE_WORKER=%~f0"
start "" /b powershell.exe -NoProfile -NonInteractive -WindowStyle Hidden -Command "Start-Sleep -Seconds 2; Remove-Item -LiteralPath $env:NUBITS_UPDATE_WORKER -Force -ErrorAction SilentlyContinue"
