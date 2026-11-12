@echo off
setlocal enabledelayedexpansion

echo =======================================================
echo          NuBits Wallet Auto-Updater for Windows
echo =======================================================

set ASSET_PATH=%~1
set ROOT_DIR=%~dp0

echo [1/4] Aguardando o encerramento do NuBits...
timeout /t 3 /nobreak >nul

rem Forçar encerramento limpo se ainda estiver aberto
taskkill /F /IM nu.exe >nul 2>&1
taskkill /F /IM nud.exe >nul 2>&1

echo [2/4] Verificando pacote de atualizacao: %ASSET_PATH%
if not exist "%ASSET_PATH%" (
    echo ERRO: Arquivo de atualizacao nao encontrado!
    pause
    exit /b 1
)

echo [3/4] Aplicando nova versao...
if exist "%ROOT_DIR%release\nu.exe" (
    copy /y "%ROOT_DIR%release\nu.exe" "%ROOT_DIR%release\nu.exe.bak" >nul
)

rem Se for arquivo .zip, descompacta sobre a pasta release
echo "%ASSET_PATH%" | findstr /i "\.zip$" >nul
if %errorlevel% equ 0 (
    powershell -NoProfile -Command "Expand-Archive -Path '%ASSET_PATH%' -DestinationPath '%ROOT_DIR%release' -Force"
) else (
    copy /y "%ASSET_PATH%" "%ROOT_DIR%release\nu.exe" >nul
)

echo [4/4] Reiniciando NuBits...
start "" "%ROOT_DIR%release\nu.exe"

echo Atualizacao concluida com sucesso!
exit /b 0
