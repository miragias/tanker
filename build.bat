@echo off
setlocal EnableDelayedExpansion

:: Output folder for objs and exe
set BuildDir=build
if not exist %BuildDir% mkdir %BuildDir%

:: Compiler and linker
set Compiler=cl
set Linker=link

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
set CFlags=/nologo /EHsc /MP /Zi /MDd /W4 /std:c++17 /wd4100 %IncludeDirs%

:: Linker flags
set LFlags=/SUBSYSTEM:console /DEBUG %LibDirs% %Libs%

:: Track if any file was recompiled
set Recompiled=0

:: Compile each source file only if needed
for %%f in (%SourceFiles%) do (
    set "Src=%%f"
    set "Obj=%BuildDir%\%%~nf.obj"

    REM Get full paths for robocopy to work correctly
    for %%A in ("!Src!") do set "FullSrc=%%~fA"
    for %%B in ("!Obj!") do set "FullObj=%%~fB"

    if not exist "!FullObj!" (
        %Compiler% /c %CFlags% /Fo"!Obj!" "!Src!"
        if errorlevel 1 goto BuildFailed
        set Recompiled=1
    ) else (
        for %%A in ("!FullSrc!") do set "TimeSrc=%%~tA"
        for %%B in ("!FullObj!") do set "TimeObj=%%~tB"
        if "!TimeSrc!" GTR "!TimeObj!" (
            %Compiler% /c %CFlags% /Fo"!Obj!" "!Src!"
            if errorlevel 1 goto BuildFailed
            set Recompiled=1
        ) else (
            REM echo Skipping !Src! (up-to-date)
        )
    )
)

:: Link only if needed or if exe is missing
if "%Recompiled%"=="1" (
    echo Linking...
    %Linker% %LFlags% %BuildDir%\*.obj /OUT:%BuildDir%\tanker.exe
    if errorlevel 1 goto BuildFailed
) else (
    if not exist %BuildDir%\tanker.exe (
        echo No recompilation, but EXE missing. Linking...
        %Linker% %LFlags% %BuildDir%\*.obj /OUT:%BuildDir%\tanker.exe
        if errorlevel 1 goto BuildFailed
    ) else (
        echo No changes detected. Build skipped.
    )
)

REM echo.
REM echo Build succeeded! Output is %BuildDir%\tanker.exe

:: Run the executable
%BuildDir%\tanker.exe

goto BuildEnd

:BuildFailed
echo.
echo Build failed!
pause
goto BuildEnd

:BuildEnd
endlocal
