@echo off
setlocal enabledelayedexpansion
rem ============================================================
rem  Loader example builder
rem    payload\*                 -> payload.7z (LZMA2, AES, -mhe=on)
rem    ..\7zsd_LZMA2.sfx + loader.txt -> loader.exe
rem  The password is read from loader.txt (line: Password="...")
rem  Usage: build_loader.bat [ /q ]
rem ============================================================
cd /d "%~dp0"

rem --- locate 7-Zip ---------------------------------------------
set "SZ="
if exist "C:\Program Files\7-Zip\7z.exe" set "SZ=C:\Program Files\7-Zip\7z.exe"
if not defined SZ if exist "C:\Program Files (x86)\7-Zip\7z.exe" set "SZ=C:\Program Files (x86)\7-Zip\7z.exe"
if not defined SZ set "SZ=%SZ_7ZIP%"
if not defined SZ (
    echo [ERROR] 7-Zip not found. Install 7-Zip or set SZ_7ZIP env var.
    exit /b 1
)

rem --- names ----------------------------------------------------
set "MODULE=..\7zsd_LZMA2.sfx"
set "LCFG=loader.txt"
set "SRC=payload"
set "OUTEXE=loader.exe"
set "OUTARC=payload.7z"

if not exist "%MODULE%" ( echo [ERROR] %MODULE% not found - keep the module in the set root & exit /b 1 )
if not exist "%LCFG%"   ( echo [ERROR] %LCFG% not found & exit /b 1 )
if not exist "%SRC%\"   ( echo [ERROR] folder %SRC% not found & exit /b 1 )

rem --- password from loader.txt: Password="..." -----------------
set "PWD="
for /f "usebackq tokens=1,* delims==" %%a in ("%LCFG%") do (
    if "%%a"=="Password" set "PWD=%%b"
)
if defined PWD set "PWD=%PWD:"=%"
if not defined PWD (
    echo [ERROR] Password="..." not found in %LCFG%
    exit /b 1
)

rem --- payload archive: LZMA2 + AES + encrypted names -----------
pushd "%SRC%"
"%SZ%" a -t7z -m0=lzma2 -mx=9 -mhe=on -p"%PWD%" -bd -y "%~dp0%OUTARC%" *
set "RC=%ERRORLEVEL%"
popd
if not "%RC%"=="0" ( echo [ERROR] 7z archiving failed & exit /b 1 )

rem --- loader = sfx module + loader config ----------------------
copy /b "%MODULE%" + "%LCFG%" "%OUTEXE%" >nul
if errorlevel 1 ( echo [ERROR] concat failed & exit /b 1 )

echo.
echo OK: %OUTEXE%  +  %OUTARC%
echo     password: "%PWD%"
echo     run: %OUTEXE% %OUTARC%
if /i not "%~1"=="/q" pause
exit /b 0
