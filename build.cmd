@echo off
setlocal
rem Build script for 7zSFX modified module (public repo, no hard-coded paths).
rem Usage: build.cmd <x86|x64> <All|Base|LZMA|LZMA2|Deflate|PPMd>
set CPU=%~1
if "%CPU%"=="" set CPU=x86
set SUB=%~2
if "%SUB%"=="" set SUB=LZMA

if /I "%CPU%"=="x86" (set ARCH=x86) else (set ARCH=x64)

if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat") else (set "VCVARS=C:\BuildTools\VC\Auxiliary\Build\vcvarsall.bat")
call "%VCVARS%" %ARCH% >nul || exit /b 1

rem shim\afxres.h provides the MFC header that is absent from VS Build Tools.
set INCLUDE=%~dp0shim;%INCLUDE%

set SUBSET=MODULE_SUBNAME=%SUB%
set METHODS=COMPRESS_COPY=1 COMPRESS_BCJ=1 COMPRESS_BCJ2=1 COMPRESS_DELTA=1 COMPRESS_LZMA=1 COMPRESS_LZMA2=1 COMPRESS_DEFLATE=1 COMPRESS_PPMD=1
if /I "%SUB%"=="Base"   (set SUBSET= & set METHODS=COMPRESS_COPY=1 COMPRESS_BCJ=1 COMPRESS_BCJ2=1 COMPRESS_DELTA=1 COMPRESS_LZMA=1)
if /I "%SUB%"=="LZMA"   set METHODS=COMPRESS_COPY=1 COMPRESS_BCJ=1 COMPRESS_BCJ2=1 COMPRESS_DELTA=1 COMPRESS_LZMA=1
if /I "%SUB%"=="LZMA2"  set METHODS=COMPRESS_COPY=1 COMPRESS_BCJ=1 COMPRESS_BCJ2=1 COMPRESS_DELTA=1 COMPRESS_LZMA=1 COMPRESS_LZMA2=1
if /I "%SUB%"=="Deflate" set METHODS=COMPRESS_COPY=1 COMPRESS_DEFLATE=1
if /I "%SUB%"=="PPMd"   set METHODS=COMPRESS_COPY=1 COMPRESS_PPMD=1

cd /d "%~dp07zSfxMod" || exit /b 1
nmake -nologo SFX_CRYPTO=1 SFX_VOLUMES=1 _SFX_MAINTAINER=1 CPU=%CPU% %SUBSET% %METHODS% build
exit /b %ERRORLEVEL%