@echo off
setlocal

:: Output folder for objs and exe
set BuildDir=build
if not exist %BuildDir% mkdir %BuildDir%

:: Compiler and linker
set Compiler=cl
set Linker=link

:: Source files list
set SourceFiles=src\main.cpp ^
    C:\Users\ioann\Programming\tanker\src\imgui\imgui.cpp ^
    C:\Users\ioann\Programming\tanker\src\imgui\imgui_draw.cpp ^
    C:\Users\ioann\Programming\tanker\src\imgui\imgui_widgets.cpp ^
    C:\Users\ioann\Programming\tanker\src\imgui\imgui_tables.cpp ^
    C:\Users\ioann\Programming\tanker\src\imgui\imgui_demo.cpp ^
    C:\Users\ioann\Programming\tanker\src\imgui\backends\imgui_impl_glfw.cpp ^
    C:\Users\ioann\Programming\tanker\src\imgui\backends\imgui_impl_vulkan.cpp

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

:: Compiler flags (compile only)
set CFlags=/nologo /EHsc /MP /Zi /MDd /W4 /std:c++17 %IncludeDirs%

:: Linker flags
set LFlags=/SUBSYSTEM:console %LibDirs% %Libs%

:: Clean previous build files
if exist %BuildDir%\*.obj del %BuildDir%\*.obj
if exist %BuildDir%\tanker.exe del %BuildDir%\tanker.exe

:: Compile each source file separately
for %%f in (%SourceFiles%) do (
    echo Compiling %%~nxf
    %Compiler% /c %CFlags% /Fo%BuildDir%\%%~nf.obj "%%f"
    if errorlevel 1 goto BuildFailed
)

:: Link all object files
echo Linking...
%Linker% %LFlags% %BuildDir%\*.obj /OUT:%BuildDir%\tanker.exe
if errorlevel 1 goto BuildFailed

echo.
echo Build succeeded! Output is %BuildDir%\tanker.exe

:: Run the executable
echo Running the program...
%BuildDir%\tanker.exe

goto BuildEnd

:BuildFailed
echo.
echo Build failed!
pause

:BuildEnd
endlocal
