@echo off
setlocal EnableExtensions

set "ROOT=%~dp0"
for %%I in ("%ROOT%") do set "ROOT=%%~fI"
set "BUILDER=%ROOT%tools\install\windows\build-installer.bat"
set "OUTDIR=%ROOT%dist\windows"

if not exist "%BUILDER%" (
  echo [ERROR] Builder script not found:
  echo   %BUILDER%
  pause
  exit /b 1
)

echo [INFO] Building Rex installer...
call "%BUILDER%" %*
if errorlevel 1 (
  echo.
  echo [ERROR] Build failed.
  pause
  exit /b 1
)

set "SETUP_EXE="
for /f "delims=" %%F in ('dir /b /a-d /o-d "%OUTDIR%\rex-*-windows-setup.exe" 2^>nul') do (
  set "SETUP_EXE=%OUTDIR%\%%F"
  goto :found
)

:found
if not defined SETUP_EXE (
  echo [WARN] Installer built but setup file was not found in:
  echo   %OUTDIR%
  pause
  exit /b 1
)

echo [INFO] Launching installer:
echo   %SETUP_EXE%
start "" "%SETUP_EXE%"
exit /b 0
