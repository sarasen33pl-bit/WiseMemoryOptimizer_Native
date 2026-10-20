@echo off
setlocal
echo ===============================================================
echo Compiling Wise Memory Optimizer Native (C++20 AAA 60FPS Engine)
echo ===============================================================

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64

cd /d "C:\Users\Administrator\Desktop\New folder (9)\WiseMemoryOptimizer_Native"

echo Compiling Resources...
rc.exe /nologo /fo resource.res resource.rc

echo Compiling C++20 Sources...
cl.exe /nologo /O2 /Oi /Ot /GL /std:c++20 /EHsc /W3 /utf-8 /DUNICODE /D_UNICODE ^
    main.cpp MainWindow.cpp SettingsDialog.cpp FeedbackDialog.cpp AboutDialog.cpp MemoryEngine.cpp NoticeWindow.cpp ^
    resource.res ^
    /link /LTCG /SUBSYSTEM:WINDOWS /OUT:WiseMemoryOptimizer_Native.exe ^
    user32.lib gdi32.lib shell32.lib comctl32.lib advapi32.lib psapi.lib gdiplus.lib ole32.lib dwmapi.lib

set BUILD_RES=%ERRORLEVEL%
if %BUILD_RES% equ 0 (
    echo.
    echo ===============================================================
    echo [SUCCESS] Build completed successfully!
    echo ===============================================================
    taskkill /f /im WiseMemoryOptimizer_Native.exe >nul 2>&1
    copy /y WiseMemoryOptimizer_Native.exe "..\WiseMemoryOptimizer_Native.exe"
) else (
    echo.
    echo [ERROR] Build failed with error code %BUILD_RES%
)

endlocal
