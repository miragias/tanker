@echo off
setlocal EnableDelayedExpansion

:: Paths
set BuildDir=build
set PCHHeader=common.h
set PCHSource=src\common.cpp
set PCHOut=%BuildDir%\common.pch
set PCHObj=%BuildDir%\common.obj

:: Source files list
set SourceFiles=src\main.cpp

:: Include directories
set IncludeDirs=/I"C:\Lib\stb" ^
 /I"C:\Users\ioann\Programming\tanker\src\imgui" ^
 /I"C:\Lib\glm-0.9.9.8\glm" ^
 /I"C:\Users\ioann\Bin\glfw-3.4\Include" ^
 /I"C:\VulkanSDK\1.4.313.2\Include" ^
 /I"C:\Users\ioann\Bin\glfw-3.4\lib-vc2022"

:: Library directories
set LibDirs=/LIBPATH:"C:\Lib\lib-vc2019" ^
 /LIBPATH:"C:\VulkanSDK\1.4.313.2\Lib" ^
 /LIBPATH:"C:\Users\ioann\Bin\glfw-3.4\lib-vc2022"

:: Libraries to link
set Libs=vulkan-1.lib glfw3.lib gdi32.lib user32.lib kernel32.lib opengl32.lib

:: Compiler flags
set CFlags=/nologo /EHsc /MP /Zi /MDd /W4 /std:c++17 /Zc:__cplusplus /wd4100 %IncludeDirs%

:: Linker flags
set LFlags=/SUBSYSTEM:console /DEBUG %LibDirs% %Libs%

if not exist %BuildDir% mkdir %BuildDir%

:: Compiler and linker
set Compiler=cl
set Linker=link

set TimestampFile=%BuildDir%\pch_timestamp.txt

:: Read last recorded timestamp
set LastTS=
if exist "%TimestampFile%" (
    for /F "usebackq delims=" %%L in ("%TimestampFile%") do set "LastTS=%%L"
    echo [PCH] Last recorded timestamp: !LastTS!
) else (
    echo [PCH] No previous timestamp found.
)

:: Grab current timestamps
for %%F in ("%PCHHeader%") do set "CurrH=%%~tF"
for %%F in ("%PCHSource%") do set "CurrC=%%~tF"
echo [PCH] Current timestamps: header=!CurrH!, source=!CurrC!
set "CurrTS=!CurrH!|!CurrC!"

:: Decide whether to rebuild
if not defined LastTS (
    set RebuildPCH=1
) else if NOT "!LastTS!"=="!CurrTS!" (
    echo [PCH] Rebuilding pch
    set RebuildPCH=1
) else (
    echo [PCH] Skipping PCH rebuild
)
set RebuildPCH=1

if defined RebuildPCH (
    echo [PCH] Compiling precompiled header: %PCHHeader%…
    %Compiler% /c /Yc"%PCHHeader%" %CFlags% /Fp"%PCHOut%" /Fo"%PCHObj%" "%PCHSource%"
    if errorlevel 1 goto BuildFailed

    >"%TimestampFile%" echo !CurrTS!
    echo [PCH] Updated timestamp file with: !CurrTS!
)

:: Compile game.dll
echo Building Game.dll...
call "C:\Users\ioann\Programming\tanker\buildDll.bat"
if errorlevel 1 goto BuildFailed

:: Simple approach: always compile (fast for single file projects)
echo Compiling...
for %%f in (%SourceFiles%) do (
    set "Src=%%f"
    set "Obj=%BuildDir%\%%~nf.obj"
    echo   !Src!
    %Compiler% /c /Yu"common.h" %CFlags% /Fp"%PCHOut%" /Fo"!Obj!" "!Src!"
    if errorlevel 1 goto BuildFailed
)

:: Link
echo Linking...
%Linker% %LFlags% %BuildDir%\*.obj /OUT:%BuildDir%\tanker.exe
if errorlevel 1 goto BuildFailed

echo Build succeeded! Running %BuildDir%\tanker.exe...
%BuildDir%\tanker.exe
goto BuildEnd

:BuildFailed
echo.
echo Build failed!
pause
goto BuildEnd

:BuildEnd
endlocal
