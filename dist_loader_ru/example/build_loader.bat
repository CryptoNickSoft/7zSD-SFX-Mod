@echo off
setlocal
pushd "%~dp0"
if not exist "..\7zsd_LZMA.sfx" (
    echo Module not found: ..\7zsd_LZMA.sfx
    popd
    exit /b 1
)
if not exist "payload.7z" (
    "C:\Program Files\7-Zip\7z.exe" a -t7z -m0=lzma -mx=9 payload.7z ".\payload\*" >nul
    if errorlevel 1 (
        echo 7z failed. Install 7-Zip or create payload.7z manually.
        popd
        exit /b 1
    )
)
copy /b "..\7zsd_LZMA.sfx" + "loader.txt" + "payload.7z" loader.exe >nul
if errorlevel 1 (
    echo Concatenation failed.
    popd
    exit /b 1
)
echo Built: loader.exe
echo Run:   loader.exe payload.7z
popd
endlocal
