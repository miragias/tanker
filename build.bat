@echo off
setlocal EnableDelayedExpansion

:: Paths
set BuildDir=build
set PCHHeader=pch.h
set PCHSource=src\pch.cpp
set PCHOut=%BuildDir%\pch.pch
set PCHObj=%BuildDir%\pch.obj

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

:: --- Determine file sizes ---
set "LastSizeFile=%BuildDir%\pch_size.txt"
set "RebuildPCH="

:: Read last recorded sizes
set "LastSizes="
if exist "%LastSizeFile%" (
    for /F "usebackq delims=" %%L in ("%LastSizeFile%") do set "LastSizes=%%L"
    echo [PCH] Last recorded size: !LastSizes!
) else (
    echo [PCH] No previous size found.
)

:: Get current sizes of the header and source
for %%F in ("%PCHHeader%") do set "SizeH=%%~zF"
for %%F in ("%PCHSource%") do set "SizeC=%%~zF"
set "CurrSizes=!SizeH!|!SizeC!"
echo [PCH] Current sizes: header=!SizeH!, source=!SizeC!

:: Decide whether to rebuild
if not defined LastSizes (
    set "RebuildPCH=1"
) else if not "!CurrSizes!"=="!LastSizes!" (
    echo [PCH] Header/source size changed. Rebuilding PCH...
    set "RebuildPCH=1"
) else (
    echo [PCH] No change in header/source size. Skipping PCH rebuild.
)

:: Compile PCH if needed
if defined RebuildPCH (
    echo [PCH] Compiling precompiled header: %PCHHeader%…
    %Compiler% /c /Yc"%PCHHeader%" %CFlags% /Fp"%PCHOut%" /Fo"%PCHObj%" "%PCHSource%"
    if errorlevel 1 goto BuildFailed

    >"%LastSizeFile%" echo !CurrSizes!
    echo [PCH] Updated size file with: !CurrSizes!
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
    %Compiler% /c /Yu"pch.h" %CFlags% /Fp"%PCHOut%" /Fo"!Obj!" "!Src!"
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
