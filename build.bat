@echo off
rem Copyright (c) 2026 Skitzelles - part of Game Vault, https://github.com/skitzellez/xbox-game-vault
rem PolyForm Noncommercial License 1.0.0: not for sale, and this notice stays in every copy.
rem Builds pkg\Flash.exe (Game Vault · Flash). WebView2's C++/WinRT headers are made once from its .winmd.
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>nul
cd /d "%~dp0"
if not exist out mkdir out
if not exist deps\cppwinrt\winrt\Microsoft.Web.WebView2.Core.h "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\cppwinrt.exe" -in deps\webview2\lib\Microsoft.Web.WebView2.Core.winmd -ref sdk -out deps\cppwinrt
cl /nologo /utf-8 /std:c++20 /EHsc /O2 /W3 /MD /DNOMINMAX /DUNICODE /D_UNICODE /DWINAPI_FAMILY=WINAPI_FAMILY_APP /D__WRL_NO_DEFAULT_LIB__ /Ideps\cppwinrt /Fo:out\ /Fe:pkg\Flash.exe src\main.cpp /link /APPCONTAINER /SUBSYSTEM:WINDOWS /LIBPATH:"%VCToolsInstallDir%lib\x64\store" /NODEFAULTLIB:kernel32.lib /NODEFAULTLIB:ole32.lib WindowsApp.lib || exit /b 1
echo BUILD OK
