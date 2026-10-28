@echo off
rem Copyright (c) 2026 Skitzelles - part of Game Vault, https://github.com/skitzellez/xbox-game-vault
rem PolyForm Noncommercial License 1.0.0: not for sale, and this notice stays in every copy.
rem Builds dist\FlashForGameVault_x64.appx (signed with Game Vault's certificate, same publisher).
setlocal
set SDK=C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64
set GV=%~dp0..\GameVault
cd /d "%~dp0"
if exist stage rmdir /s /q stage
mkdir stage\Assets
copy /y pkg\AppxManifest.xml stage\ >nul
copy /y pkg\Flash.exe stage\ >nul || exit /b 1
copy /y pkg\Microsoft.Web.WebView2.Core.dll stage\ >nul || exit /b 1
copy /y pkg\Assets\*.png stage\Assets\ >nul
mkdir stage\Assets\Licenses
copy /y deps\webview2\LICENSE.txt stage\Assets\Licenses\WebView2-LICENSE.txt >nul
rem the page and Ruffle (MIT / Apache-2.0, both licences in web\ruffle)
xcopy /e /i /q /y web stage\web >nul || exit /b 1
if not exist dist mkdir dist
if exist dist\FlashForGameVault_x64.appx del dist\FlashForGameVault_x64.appx
"%SDK%\makeappx.exe" pack /o /d stage /p dist\FlashForGameVault_x64.appx || exit /b 1
if not defined PFX_PASSWORD for /f "usebackq delims=" %%p in (`powershell -NoProfile -ExecutionPolicy Bypass -File "%GV%\pfx_password.ps1"`) do set "PFX_PASSWORD=%%p"
if not defined PFX_PASSWORD (echo the certificate password could not be read & exit /b 1)
"%SDK%\signtool.exe" sign /fd SHA256 /a /f "%GV%\cert\GameVault.pfx" /p %PFX_PASSWORD% dist\FlashForGameVault_x64.appx || exit /b 1
rmdir /s /q stage
echo PACKAGE OK
