@echo off
setlocal EnableDelayedExpansion

:: Output folder for objs and dll
set BuildDir=build

if not exist %BuildDir% mkdir %BuildDir%

:: Compiler and linker
set Compiler=cl
set Linker=link

:: Source files list (your game logic)
set SourceFiles=src\Game.cpp

:: Include directories (reuse your existing includes)
set IncludeDirs=/I"C:\Lib\stb" ^
 /I"C:\Users\ioann\Programming\tanker\src\imgui" ^
 /I"C:\Lib\glm-0.9.9.8\glm" ^
 /I"C:\Users\ioann\Bin\glfw-3.4\Include" ^
 /I"C:\VulkanSDK\1.4.313.2\Include" ^
 /I"C:\Users\ioann\Bin\glfw-3.4\lib-vc2022"

:: Library directories (only if your DLL needs libs, else empty)
set LibDirs=/LIBPATH:"C:\Lib\lib-vc2019" ^
 /LIBPATH:"C:\VulkanSDK\1.4.313.2\Lib" ^
 /LIBPATH:"C:\Users\ioann\Bin\glfw-3.4\lib-vc2022"

:: Libraries to link (keep empty or minimal)
set Libs=vulkan-1.lib glfw3.lib gdi32.lib user32.lib kernel32.lib opengl32.lib

:: Compiler flags
set CFlags=/nologo /EHsc /MP /Zi /MDd /W4 /std:c++17 /Zc:__cplusplus /wd4100 %IncludeDirs%

:: Linker flags - /DLL creates a DLL, /DEBUG adds debug info
set LFlags=/DLL /DEBUG %LibDirs% %Libs%

echo Compiling game DLL...
for %%f in (%SourceFiles%) do (
    set "Src=%%f"
    set "Obj=%BuildDir%\%%~nf.obj"
    echo   !Src!
    %Compiler% /c %CFlags% /Fo"!Obj!" "!Src!"
    if errorlevel 1 goto BuildFailed
)

echo Linking game DLL...
%Linker% %LFlags% %BuildDir%\*.obj /OUT:%BuildDir%\Game.dll
if errorlevel 1 goto BuildFailed

echo Build succeeded! DLL is at %BuildDir%\Game.dll
goto BuildEnd

:BuildFailed
echo.
echo Build failed!
pause
goto BuildEnd

:BuildEnd
endlocal
