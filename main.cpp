#include <windows.h>
#include <commctrl.h>
#include "MainWindow.h"
#include "SettingsDialog.h"
#include "FeedbackDialog.h"
#include "AboutDialog.h"
#include "NoticeWindow.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "advapi32.lib")

// Enable Visual Styles (Common Controls 6.0)
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    // High-DPI Awareness
    SetProcessDPIAware();

    // Acquire privileges for kernel NT memory optimization
    MemoryEngine::Initialize();

    // Initialize Common Controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    // Direct Dialog Launch for testing & automation
    if (lpCmdLine && wcsstr(lpCmdLine, L"-settings")) {
        AppConfig cfg;
        cfg.Load();
        SettingsDialog::Show(NULL, cfg);
        return 0;
    }

    if (lpCmdLine && wcsstr(lpCmdLine, L"-feedback")) {
        AppConfig cfg;
        cfg.Load();
        FeedbackDialog::Show(NULL, cfg);
        return 0;
    }

    if (lpCmdLine && wcsstr(lpCmdLine, L"-about")) {
        ULONG_PTR gdiToken;
        Gdiplus::GdiplusStartupInput gdiInput;
        Gdiplus::GdiplusStartup(&gdiToken, &gdiInput, NULL);
        AppConfig cfg;
        cfg.Load();
        AboutDialog::Show(NULL, cfg);
        Gdiplus::GdiplusShutdown(gdiToken);
        return 0;
    }

    if (lpCmdLine && wcsstr(lpCmdLine, L"-notice")) {
        ULONG_PTR gdiToken;
        Gdiplus::GdiplusStartupInput gdiInput;
        Gdiplus::GdiplusStartup(&gdiToken, &gdiInput, NULL);
        AppConfig cfg;
        cfg.Load();
        NoticeWindow::Show(NULL, L"2.54 GB", 30, nullptr, cfg.language, cfg.darkMode);
        MSG msg;
        while (GetMessageW(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        Gdiplus::GdiplusShutdown(gdiToken);
        return 0;
    }

    // 1. Single-instance mutex check
    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"Local\\WiseMemoryOptimizerNative_SingleInstance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND hExisting = FindWindowW(L"WiseMemoryOptimizerNativeClass", NULL);
        if (hExisting) {
            ShowWindow(hExisting, SW_RESTORE);
            SetForegroundWindow(hExisting);
        }
        if (hMutex) CloseHandle(hMutex);
        return 0;
    }

    // Command-line check for silent tray start
    bool startInTray = false;
    if (lpCmdLine && wcsstr(lpCmdLine, L"-tray")) {
        startInTray = true;
    }

    // Run Main Window
    MainWindow::CreateAndRun(hInstance, nCmdShow, startInTray);

    // Cleanup mutex
    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
    return 0;
}
