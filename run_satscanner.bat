@echo off
setlocal

rem Adjust this path to your MSYS2 UCRT64 installation.
set "MSYS2_UCRT64=C:\msys64\ucrt64"
set "PATH=%MSYS2_UCRT64%\bin;%PATH%"

cd /d "%~dp0"

"%~dp0src\satscanner.exe" -p "%~dp0inst.prm"

set "EXIT_CODE=%ERRORLEVEL%"
echo.
echo SaTScan exited with code %EXIT_CODE%.
pause

endlocal
exit /b %EXIT_CODE%