@echo off
setlocal

echo ============================================================
echo  Kinect Game Input - Build
echo ============================================================

:: Initialize MSVC environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=amd64 >nul 2>&1
if errorlevel 1 (
    echo ERROR: Failed to initialize MSVC environment.
    exit /b 1
)

:: Create build directory
if not exist build mkdir build
if not exist dist mkdir dist

:: Compile resources
rc /nologo /I include /fo build\app.res src\app\app.rc
if errorlevel 1 (
    echo ERROR: Failed to compile resources.
    exit /b 1
)

:: Compile
cl /nologo /EHsc /std:c++17 /Zi /Od ^
    /I "include" ^
    /I "third_party\imgui" ^
    /I "third_party\imgui\backends" ^
    /I "%KINECTSDK20_DIR%inc" ^
    /DUNICODE /D_UNICODE /DWIN32 ^
    src\globals.cpp ^
    src\app\main.cpp ^
    src\app\dx11.cpp ^
    src\kinect\kinect.cpp ^
    src\input\input.cpp ^
    src\ui\ui.cpp ^
    src\config\config.cpp ^
    third_party\imgui\imgui.cpp ^
    third_party\imgui\imgui_draw.cpp ^
    third_party\imgui\imgui_tables.cpp ^
    third_party\imgui\imgui_widgets.cpp ^
    third_party\imgui\backends\imgui_impl_win32.cpp ^
    third_party\imgui\backends\imgui_impl_dx11.cpp ^
    /Fe:build\KinectTool.exe ^
    /Fo:build\ ^
    /Fd:build\KinectTool.pdb ^
    build\app.res ^
    /link /SUBSYSTEM:WINDOWS ^
    /LIBPATH:"%KINECTSDK20_DIR%lib\x64" ^
    d3d11.lib dxgi.lib d2d1.lib ^
    kinect20.lib ^
    user32.lib gdi32.lib shell32.lib ole32.lib dwmapi.lib

if errorlevel 1 (
    echo.
    echo BUILD FAILED
    exit /b 1
)

copy /y build\KinectTool.exe dist\KinectTool.exe >nul

echo.
echo BUILD SUCCEEDED -^> dist\KinectTool.exe
