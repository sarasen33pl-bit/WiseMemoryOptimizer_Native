#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <string>
#include "Config.h"
#include "MemoryEngine.h"

#pragma comment(lib, "gdiplus.lib")

class MainWindow {
public:
    static bool CreateAndRun(HINSTANCE hInstance, int nCmdShow, bool startInTray);

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    static void OnPaint(HWND hWnd);
    static void OnAnimationTick(HWND hWnd);
    static void StartOptimization(HWND hWnd);
    static DWORD WINAPI OptimizationWorkerThread(LPVOID lpParam);

    static void ShowDropdownMenu(HWND hWnd, int x, int y);
    static void ShowTrayMenu(HWND hWnd);
    static void UpdateTrayIcon(HWND hWnd, bool create = false);
    static HICON GenerateTrayUsageIcon(int percent);
    static void RegisterAppHotkey(HWND hWnd);
    static void UnregisterAppHotkey(HWND hWnd);
    static void ToggleAdvanced(HWND hWnd);
    static Gdiplus::Image* LoadImageFromResource(HMODULE hMod, int resId);

    static HWND s_hWnd;
    static AppConfig s_config;
    static MemoryMetrics s_metrics;
    static ULONG_PTR s_gdiplusToken;
    static Gdiplus::Image* s_pAppIconImage;
    static HICON s_hDynamicTrayIcon;

    // Animation & State
    static bool s_isOptimizing;
    static float s_animWavePhase;
    static float s_animIconAngle;
    static float s_displayLoadPercent;
    static float s_targetLoadPercent;

    // Advanced Panel Smooth Horizontal Slide Animation
    static bool s_isAdvancedOpen;
    static bool s_isAnimatingWidth;
    static int  s_currentWidth;
    static int  s_targetWidth;

    static std::wstring s_freedText;
    static std::wstring s_freedTime;
    static time_t       s_lastOptTimestamp;
    static std::wstring FormatTimestamp(time_t t, int langId);

    static int s_btnOptHover;   // 0: none, 1: left, 2: right
    static int s_titleBtnHover; // 0: none, 1: info, 2: settings, 3: close
    static bool s_isTrackingMouse;

    static ULONGLONG s_lastAutoOptTick;
    static NOTIFYICONDATAW s_nid;
};
