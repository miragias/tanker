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
set CFlags=/nologo /EHsc /MP /Zi /MDd /W4 /std:c++17 /Zc:__cplusplus /wd4100 %IncludeDirs%

:: Linker flags
set LFlags=/SUBSYSTEM:console /DEBUG %LibDirs% %Libs%

:: Simple approach: always compile (fast for single file projects)
echo Compiling...
for %%f in (%SourceFiles%) do (
    set "Src=%%f"
    set "Obj=%BuildDir%\%%~nf.obj"
    echo   !Src!
    %Compiler% /c %CFlags% /Fo"!Obj!" "!Src!"
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
