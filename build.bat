@echo off
setlocal

:: Compiler and linker setup
set Compiler=cl
set Linker=link

:: Source files (your main + imgui sources)
set Source=src\main.cpp ^
    C:\Users\ioann\Programming\tanker\src\imgui\imgui.cpp ^
    C:\Users\ioann\Programming\tanker\src\imgui\imgui_draw.cpp ^
    C:\Users\ioann\Programming\tanker\src\imgui\imgui_widgets.cpp ^
    C:\Users\ioann\Programming\tanker\src\imgui\imgui_tables.cpp ^
    C:\Users\ioann\Programming\tanker\src\imgui\imgui_demo.cpp ^
    C:\Users\ioann\Programming\tanker\src\imgui\backends\imgui_impl_glfw.cpp ^
    C:\Users\ioann\Programming\tanker\src\imgui\backends\imgui_impl_vulkan.cpp

:: Output binary name
set Output=tanker.exe

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
set CFlags=/nologo /EHsc /Zi /MDd /W4 /std:c++17 %IncludeDirs%

:: Linker flags
set LFlags=/SUBSYSTEM:console %LibDirs% %Libs%

:: Clean old output
if exist %Output% del %Output%
if exist main.obj del main.obj
if exist main.pdb del main.pdb

:: Build
%Compiler% %CFlags% %Source% /link %LFlags% /OUT:%Output%

:: Pause to see output
echo.
echo Build finished with error level %ERRORLEVEL%
pause

endlocal
