#include "MainWindow.h"
#include "SettingsDialog.h"
#include "FeedbackDialog.h"
#include "AboutDialog.h"
#include "NoticeWindow.h"
#include "Localization.h"
#include <time.h>
#include <shellapi.h>
#include <dwmapi.h>
#include <math.h>
#include <vector>

#pragma comment(lib, "dwmapi.lib")

using namespace Gdiplus;

#define TIMER_ANIM_ID      101
#define TIMER_METRICS_ID   102
#define WM_TRAYICON_MSG    (WM_USER + 100)
#define WM_OPT_FINISHED    (WM_USER + 201)

#define IDM_TRAY_OPEN      3001
#define IDM_TRAY_OPT_NOW   3002
#define IDM_TRAY_SETTINGS  3003
#define IDM_TRAY_ABOUT     3004
#define IDM_TRAY_EXIT      3005

#define IDM_TRAY_AUTO_DIS  3020
#define IDM_TRAY_AUTO_LOW  3021
#define IDM_TRAY_AUTO_5    3022
#define IDM_TRAY_AUTO_10   3023
#define IDM_TRAY_AUTO_30   3024
#define IDM_TRAY_AUTO_60   3025

#define IDM_OPT_TRIM       3011
#define IDM_OPT_STANDBY    3012
#define IDM_OPT_DEFRAG     3013

HWND MainWindow::s_hWnd = NULL;
AppConfig MainWindow::s_config;
MemoryMetrics MainWindow::s_metrics = {};
ULONG_PTR MainWindow::s_gdiplusToken = 0;
Gdiplus::Image* MainWindow::s_pAppIconImage = nullptr;
HICON MainWindow::s_hDynamicTrayIcon = NULL;

bool MainWindow::s_isOptimizing = false;
float MainWindow::s_animWavePhase = 0.0f;
float MainWindow::s_animIconAngle = 0.0f;
float MainWindow::s_displayLoadPercent = 35.0f;
float MainWindow::s_targetLoadPercent = 35.0f;

bool MainWindow::s_isAdvancedOpen = false;
bool MainWindow::s_isAnimatingWidth = false;
int  MainWindow::s_currentWidth = 346;
int  MainWindow::s_targetWidth = 346;

std::wstring MainWindow::s_freedText = L"2.54 GB";
std::wstring MainWindow::s_freedTime = L"8:13:22 AM";
time_t       MainWindow::s_lastOptTimestamp = 0;

int MainWindow::s_btnOptHover = 0;
int MainWindow::s_titleBtnHover = 0;
bool MainWindow::s_isTrackingMouse = false;
ULONGLONG MainWindow::s_lastAutoOptTick = 0;
NOTIFYICONDATAW MainWindow::s_nid = { sizeof(NOTIFYICONDATAW) };

static const int WINDOW_WIDTH_COLLAPSED = 346;
static const int WINDOW_WIDTH_EXPANDED  = 800;
static const int WINDOW_HEIGHT          = 473;

static std::wstring FormatBytesGB(ULONGLONG bytes) {
    double gb = (double)bytes / (1024.0 * 1024.0 * 1024.0);
    wchar_t buf[32];
    swprintf_s(buf, L"%.2f GB", gb);
    return buf;
}

static std::wstring FormatBytesMB(ULONGLONG bytes) {
    double mb = (double)bytes / (1024.0 * 1024.0);
    wchar_t buf[32];
    swprintf_s(buf, L"%.2f MB", mb);
    return buf;
}

std::wstring MainWindow::FormatTimestamp(time_t t, int langId) {
    if (t == 0) return L"";
    struct tm timeinfo;
    localtime_s(&timeinfo, &t);
    int hour12 = timeinfo.tm_hour % 12;
    if (hour12 == 0) hour12 = 12;
    wchar_t buf[64];
    if (langId == 1) { // Arabic
        const wchar_t* ampm = (timeinfo.tm_hour >= 12) ? L"م" : L"ص";
        swprintf_s(buf, L"%d:%02d:%02d %s", hour12, timeinfo.tm_min, timeinfo.tm_sec, ampm);
    } else { // English
        const wchar_t* ampm = (timeinfo.tm_hour >= 12) ? L"PM" : L"AM";
        swprintf_s(buf, L"%d:%02d:%02d %s", hour12, timeinfo.tm_min, timeinfo.tm_sec, ampm);
    }
    return buf;
}

static void LogDbg(LPCWSTR fmt, ...) {
    wchar_t buf[512];
    va_list args;
    va_start(args, fmt);
    vswprintf_s(buf, fmt, args);
    va_end(args);
    OutputDebugStringW(buf);
}

struct OptWorkerParam {
    HWND hWnd;
    bool clearMem;
    bool clearStandby;
    bool defragMem;
    bool clearClipboard;
};

Gdiplus::Image* MainWindow::LoadImageFromResource(HMODULE hMod, int resId) {
    HICON hIcon = (HICON)LoadImageW(hMod, MAKEINTRESOURCEW(1), IMAGE_ICON, 24, 24, LR_DEFAULTCOLOR);
    if (!hIcon) {
        hIcon = LoadIconW(hMod, MAKEINTRESOURCEW(1));
    }
    if (!hIcon) return nullptr;
    Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromHICON(hIcon);
    DestroyIcon(hIcon);
    return bmp;
}

HICON MainWindow::GenerateTrayUsageIcon(int percent) {
    int size = 16;
    HDC hdcScreen = GetDC(NULL);
    HDC memDC = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = size;
    bmi.bmiHeader.biHeight = -size; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = nullptr;
    HBITMAP hColorBm = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    HBITMAP hOldBm = (HBITMAP)SelectObject(memDC, hColorBm);

    {
        Graphics g(memDC);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetTextRenderingHint(TextRenderingHintSingleBitPerPixelGridFit);

        // Rounded background rectangle
        Color bgCol = (percent >= 90) ? Color(255, 225, 29, 72) :
                      (percent >= 75) ? Color(255, 245, 158, 11) :
                                        Color(255, 69, 186, 31);
        SolidBrush bgBrush(bgCol);
        g.FillRectangle(&bgBrush, 0, 0, size, size);

        // Draw percentage number (e.g. "35")
        std::wstring str = std::to_wstring(percent);
        FontFamily ff(L"Tahoma");
        Font font(&ff, 7.5f, FontStyleBold, UnitPoint);
        SolidBrush whiteBrush(Color(255, 255, 255, 255));

        StringFormat sf;
        sf.SetAlignment(StringAlignmentCenter);
        sf.SetLineAlignment(StringAlignmentCenter);
        RectF r((REAL)0, (REAL)0, (REAL)size, (REAL)size);
        g.DrawString(str.c_str(), -1, &font, r, &sf, &whiteBrush);
    }

    SelectObject(memDC, hOldBm);
    DeleteDC(memDC);
    ReleaseDC(NULL, hdcScreen);

    // Create 1-bit mask bitmap
    HBITMAP hMaskBm = CreateBitmap(size, size, 1, 1, NULL);

    ICONINFO ii = {};
    ii.fIcon = TRUE;
    ii.hbmColor = hColorBm;
    ii.hbmMask = hMaskBm;

    HICON hIcon = CreateIconIndirect(&ii);

    DeleteObject(hColorBm);
    DeleteObject(hMaskBm);

    return hIcon;
}

bool MainWindow::CreateAndRun(HINSTANCE hInstance, int nCmdShow, bool startInTray) {
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&s_gdiplusToken, &gdiplusStartupInput, NULL);

    // 1. Load authentic RAM chip icon
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring iconPath(exePath);
    size_t p = iconPath.find_last_of(L"\\/");
    if (p != std::wstring::npos) iconPath = iconPath.substr(0, p + 1);
    std::wstring path1 = iconPath + L"assets\\ram_icon.png";
    std::wstring path2 = iconPath + L"WiseMemoryOptimizer_Native\\assets\\ram_icon.png";
    std::wstring path3 = iconPath + L"assets\\icon_1.png";
    s_pAppIconImage = Gdiplus::Image::FromFile(path1.c_str());
    if (!s_pAppIconImage || s_pAppIconImage->GetLastStatus() != Ok) {
        s_pAppIconImage = Gdiplus::Image::FromFile(path2.c_str());
    }
    if (!s_pAppIconImage || s_pAppIconImage->GetLastStatus() != Ok) {
        s_pAppIconImage = Gdiplus::Image::FromFile(path3.c_str());
    }
    if (!s_pAppIconImage || s_pAppIconImage->GetLastStatus() != Ok) {
        s_pAppIconImage = LoadImageFromResource(hInstance, 1);
    }

    s_config.Load();
    MemoryEngine::Initialize();
    s_metrics = MemoryEngine::QueryMetrics();
    s_displayLoadPercent = (float)s_metrics.loadPercentage;
    s_targetLoadPercent = (float)s_metrics.loadPercentage;
    s_lastOptTimestamp = time(NULL);
    s_freedTime = FormatTimestamp(s_lastOptTimestamp, s_config.language);
    s_lastAutoOptTick = GetTickCount64();

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"WiseMemoryOptimizerNativeClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCEW(1));
    if (!wc.hIcon) wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.style = CS_DROPSHADOW;
    RegisterClassExW(&wc);

    int scrW = GetSystemMetrics(SM_CXSCREEN);
    int scrH = GetSystemMetrics(SM_CYSCREEN);
    int x = (scrW - WINDOW_WIDTH_COLLAPSED) / 2;
    int y = (scrH - WINDOW_HEIGHT) / 2;
    if (s_config.windowLeft >= 0 && s_config.windowTop >= 0 &&
        s_config.windowLeft < scrW - 100 && s_config.windowTop < scrH - 100) {
        x = s_config.windowLeft;
        y = s_config.windowTop;
    }

    s_currentWidth = WINDOW_WIDTH_COLLAPSED;
    s_targetWidth = WINDOW_WIDTH_COLLAPSED;

    s_hWnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        L"WiseMemoryOptimizerNativeClass",
        L"Wise Memory Optimizer",
        WS_POPUP | WS_BORDER | WS_MINIMIZEBOX,
        x, y, WINDOW_WIDTH_COLLAPSED, WINDOW_HEIGHT,
        NULL, NULL, hInstance, NULL
    );

    if (!s_hWnd) {
        return false;
    }

    BOOL useDark = s_config.darkMode ? TRUE : FALSE;
    DwmSetWindowAttribute(s_hWnd, 20, &useDark, sizeof(useDark));
    DwmSetWindowAttribute(s_hWnd, 19, &useDark, sizeof(useDark));

    LogDbg(L"1. Before UpdateTrayIcon");
    UpdateTrayIcon(s_hWnd, true);
    LogDbg(L"2. Before RegisterAppHotkey");
    RegisterAppHotkey(s_hWnd);
    LogDbg(L"3. Before SetTimer");
    SetTimer(s_hWnd, TIMER_ANIM_ID, 16, NULL);
    SetTimer(s_hWnd, TIMER_METRICS_ID, 1000, NULL);

    if (!startInTray) {
        LogDbg(L"4. Before ShowWindow");
        ShowWindow(s_hWnd, SW_SHOW);
        LogDbg(L"5. Before UpdateWindow");
        UpdateWindow(s_hWnd);
        LogDbg(L"6. After UpdateWindow");
    } else {
        ShowWindow(s_hWnd, SW_HIDE);
    }

    LogDbg(L"7. Entering message loop");
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    LogDbg(L"8. Exited message loop, msg=%u", msg.message);

    UnregisterAppHotkey(s_hWnd);
    Shell_NotifyIconW(NIM_DELETE, &s_nid);
    if (s_hDynamicTrayIcon) { DestroyIcon(s_hDynamicTrayIcon); s_hDynamicTrayIcon = NULL; }
    if (s_pAppIconImage) { delete s_pAppIconImage; s_pAppIconImage = nullptr; }
    GdiplusShutdown(s_gdiplusToken);

    return true;
}

void MainWindow::RegisterAppHotkey(HWND hWnd) {
    if (!s_config.shortcutEnabled) return;

    UINT fsModifiers = MOD_NOREPEAT;
    if (s_config.shortcutMod == 0) fsModifiers |= (MOD_CONTROL | MOD_ALT);
    else if (s_config.shortcutMod == 1) fsModifiers |= (MOD_ALT | MOD_SHIFT);
    else if (s_config.shortcutMod == 2) fsModifiers |= (MOD_CONTROL | MOD_SHIFT);

    UINT vk = (UINT)towupper(s_config.shortcutKey);
    RegisterHotKey(hWnd, 9001, fsModifiers, vk);
}

void MainWindow::UnregisterAppHotkey(HWND hWnd) {
    UnregisterHotKey(hWnd, 9001);
}

void MainWindow::UpdateTrayIcon(HWND hWnd, bool create) {
    s_nid.cbSize = sizeof(NOTIFYICONDATAW);
    s_nid.hWnd = hWnd;
    s_nid.uID = 1;
    s_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    s_nid.uCallbackMessage = WM_TRAYICON_MSG;

    if (s_config.showUsageInTray) {
        if (s_hDynamicTrayIcon) {
            DestroyIcon(s_hDynamicTrayIcon);
            s_hDynamicTrayIcon = NULL;
        }
        s_hDynamicTrayIcon = GenerateTrayUsageIcon((int)s_displayLoadPercent);
        s_nid.hIcon = s_hDynamicTrayIcon;
    } else {
        if (s_hDynamicTrayIcon) {
            DestroyIcon(s_hDynamicTrayIcon);
            s_hDynamicTrayIcon = NULL;
        }
        HICON hIcon = LoadIcon(GetModuleHandleW(NULL), MAKEINTRESOURCEW(1));
        if (!hIcon) hIcon = LoadIcon(NULL, IDI_APPLICATION);
        s_nid.hIcon = hIcon;
    }

    std::wstring tip = L"Wise Memory Optimizer - In Use: " + std::to_wstring((int)s_displayLoadPercent) + L"%";
    wcscpy_s(s_nid.szTip, tip.c_str());

    if (create) {
        Shell_NotifyIconW(NIM_ADD, &s_nid);
    } else {
        Shell_NotifyIconW(NIM_MODIFY, &s_nid);
    }
}

void MainWindow::ToggleAdvanced(HWND hWnd) {
    s_isAdvancedOpen = !s_isAdvancedOpen;
    s_targetWidth = s_isAdvancedOpen ? WINDOW_WIDTH_EXPANDED : WINDOW_WIDTH_COLLAPSED;
    s_isAnimatingWidth = true;
}

DWORD WINAPI MainWindow::OptimizationWorkerThread(LPVOID lpParam) {
    OptWorkerParam* pParam = (OptWorkerParam*)lpParam;
    if (!pParam) return 0;

    std::wstring freedStr;
    ULONGLONG freedBytes = MemoryEngine::OptimizeEx(pParam->clearMem, pParam->clearStandby, pParam->defragMem, pParam->clearClipboard, &freedStr);

    std::wstring* pResult = new std::wstring(freedStr);
    PostMessageW(pParam->hWnd, WM_OPT_FINISHED, (WPARAM)pResult, 0);

    delete pParam;
    return 0;
}

void MainWindow::StartOptimization(HWND hWnd) {
    if (s_isOptimizing) return;
    s_isOptimizing = true;
    s_lastAutoOptTick = GetTickCount64();

    OptWorkerParam* pParam = new OptWorkerParam();
    pParam->hWnd = hWnd;
    pParam->clearMem = s_config.clearMem;
    pParam->clearStandby = s_config.clearStandby;
    pParam->defragMem = s_config.defragMem;
    pParam->clearClipboard = s_config.clearClipboardOnOpt;

    HANDLE hThread = CreateThread(NULL, 0, OptimizationWorkerThread, pParam, 0, NULL);
    if (hThread) CloseHandle(hThread);
}

void MainWindow::OnAnimationTick(HWND hWnd) {
    // 1. Fluid wave motion
    float waveSpeed = s_isOptimizing ? 0.25f : 0.05f;
    s_animWavePhase += waveSpeed;
    if (s_animWavePhase > 62.83f) s_animWavePhase -= 62.83f;

    // 2. Button icon rotation during optimization
    if (s_isOptimizing) {
        s_animIconAngle += 15.0f;
        if (s_animIconAngle >= 360.0f) s_animIconAngle -= 360.0f;
    } else {
        s_animIconAngle = 0.0f;
    }

    // 3. Smooth percentage interpolation
    float diff = s_targetLoadPercent - s_displayLoadPercent;
    if (fabsf(diff) > 0.05f) {
        s_displayLoadPercent += diff * 0.08f;
    } else {
        s_displayLoadPercent = s_targetLoadPercent;
    }

    // 4. Smooth window WIDTH horizontal accordion slide animation
    if (s_isAnimatingWidth) {
        int wDiff = s_targetWidth - s_currentWidth;
        if (abs(wDiff) > 2) {
            s_currentWidth += (int)(wDiff * 0.35f);
            RECT rc;
            GetWindowRect(hWnd, &rc);
            SetWindowPos(hWnd, NULL, rc.left, rc.top, s_currentWidth, WINDOW_HEIGHT, SWP_NOZORDER);
        } else {
            s_currentWidth = s_targetWidth;
            s_isAnimatingWidth = false;
            RECT rc;
            GetWindowRect(hWnd, &rc);
            SetWindowPos(hWnd, NULL, rc.left, rc.top, s_currentWidth, WINDOW_HEIGHT, SWP_NOZORDER);
        }
    }

    InvalidateRect(hWnd, NULL, FALSE);
}

void MainWindow::ShowDropdownMenu(HWND hWnd, int x, int y) {
    AppStrings strings = Localization::GetStrings(s_config.language);
    HMENU hMenu = CreatePopupMenu();
    InsertMenuW(hMenu, 0, MF_BYPOSITION | MF_STRING | (s_config.clearMem ? MF_CHECKED : MF_UNCHECKED), IDM_OPT_TRIM, strings.freeUpRAM.c_str());
    InsertMenuW(hMenu, 1, MF_BYPOSITION | MF_STRING | (s_config.clearStandby ? MF_CHECKED : MF_UNCHECKED), IDM_OPT_STANDBY, strings.clearStandbyRAM.c_str());
    InsertMenuW(hMenu, 2, MF_BYPOSITION | MF_STRING | (s_config.defragMem ? MF_CHECKED : MF_UNCHECKED), IDM_OPT_DEFRAG, strings.defragRAM.c_str());

    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, x, y, 0, hWnd, NULL);
    DestroyMenu(hMenu);
}

void MainWindow::ShowTrayMenu(HWND hWnd) {
    AppStrings strings = Localization::GetStrings(s_config.language);
    HMENU hMenu = CreatePopupMenu();
    InsertMenuW(hMenu, 0, MF_BYPOSITION | MF_STRING | MF_DEFAULT, IDM_TRAY_OPEN, strings.trayOpen.c_str());
    InsertMenuW(hMenu, 1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
    InsertMenuW(hMenu, 2, MF_BYPOSITION | MF_STRING, IDM_TRAY_OPT_NOW, strings.trayOptNow.c_str());

    HMENU hSubAuto = CreatePopupMenu();
    bool isAutoDisabled = !s_config.autoOptEnabled;
    bool isLowMem = s_config.autoOptEnabled && (s_config.autoOptType == 0);
    bool isM5 = s_config.autoOptEnabled && (s_config.autoOptType == 1) && (s_config.autoOptMinutes == 5);
    bool isM10 = s_config.autoOptEnabled && (s_config.autoOptType == 1) && (s_config.autoOptMinutes == 10);
    bool isM30 = s_config.autoOptEnabled && (s_config.autoOptType == 1) && (s_config.autoOptMinutes == 30);
    bool isM60 = s_config.autoOptEnabled && (s_config.autoOptType == 1) && (s_config.autoOptMinutes >= 45);

    InsertMenuW(hSubAuto, 0, MF_BYPOSITION | MF_STRING | (isAutoDisabled ? MF_CHECKED : MF_UNCHECKED), IDM_TRAY_AUTO_DIS, strings.trayDisable.c_str());
    InsertMenuW(hSubAuto, 1, MF_BYPOSITION | MF_STRING | (isLowMem ? MF_CHECKED : MF_UNCHECKED), IDM_TRAY_AUTO_LOW, strings.trayLowMem.c_str());
    InsertMenuW(hSubAuto, 2, MF_BYPOSITION | MF_STRING | (isM5 ? MF_CHECKED : MF_UNCHECKED), IDM_TRAY_AUTO_5, strings.tray5m.c_str());
    InsertMenuW(hSubAuto, 3, MF_BYPOSITION | MF_STRING | (isM10 ? MF_CHECKED : MF_UNCHECKED), IDM_TRAY_AUTO_10, strings.tray10m.c_str());
    InsertMenuW(hSubAuto, 4, MF_BYPOSITION | MF_STRING | (isM30 ? MF_CHECKED : MF_UNCHECKED), IDM_TRAY_AUTO_30, strings.tray30m.c_str());
    InsertMenuW(hSubAuto, 5, MF_BYPOSITION | MF_STRING | (isM60 ? MF_CHECKED : MF_UNCHECKED), IDM_TRAY_AUTO_60, strings.tray60m.c_str());

    InsertMenuW(hMenu, 3, MF_BYPOSITION | MF_POPUP, (UINT_PTR)hSubAuto, strings.trayAutoOpt.c_str());
    InsertMenuW(hMenu, 4, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
    InsertMenuW(hMenu, 5, MF_BYPOSITION | MF_STRING, IDM_TRAY_SETTINGS, strings.traySettings.c_str());
    InsertMenuW(hMenu, 6, MF_BYPOSITION | MF_STRING, IDM_TRAY_ABOUT, strings.trayAbout.c_str());
    InsertMenuW(hMenu, 7, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
    InsertMenuW(hMenu, 8, MF_BYPOSITION | MF_STRING, IDM_TRAY_EXIT, strings.trayExit.c_str());

    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
    DestroyMenu(hMenu);
}

void MainWindow::OnPaint(HWND hWnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);

    RECT clientRc;
    GetClientRect(hWnd, &clientRc);
    int width = clientRc.right - clientRc.left;
    int height = clientRc.bottom - clientRc.top;
    if (width <= 0 || height <= 0) {
        EndPaint(hWnd, &ps);
        return;
    }

    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBM = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP oldBM = (HBITMAP)SelectObject(memDC, memBM);

    {
        Graphics g(memDC);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
        g.SetPixelOffsetMode(PixelOffsetModeHighQuality);

        bool dark = s_config.darkMode;
        AppStrings strings = Localization::GetStrings(s_config.language);
        Color bgCol = dark ? Color(255, 30, 30, 30) : Color(255, 255, 255, 255);
        Color titleTextCol = dark ? Color(255, 230, 230, 230) : Color(255, 45, 55, 72);
        Color borderCol = dark ? Color(255, 60, 60, 60) : Color(255, 220, 225, 230);

        SolidBrush bgBrush(bgCol);
        g.FillRectangle(&bgBrush, 0, 0, width, height);

        FontFamily ff(L"Tahoma");
        Font fontTitle(&ff, 9, FontStyleRegular, UnitPoint);
        Font fontSmall(&ff, 8.5f, FontStyleRegular, UnitPoint);
        Font fontBold(&ff, 9, FontStyleBold, UnitPoint);
        Font fontHuge(&ff, 26, FontStyleBold, UnitPoint);
        Font fontBtn(&ff, 11, FontStyleBold, UnitPoint);

        // 1. Title bar (Height ~ 32px)
        if (s_pAppIconImage && s_pAppIconImage->GetLastStatus() == Ok) {
            g.DrawImage(s_pAppIconImage, 10, 7, 20, 18);
        } else {
            SolidBrush greenChipBrush(Color(255, 69, 186, 31));
            g.FillRectangle(&greenChipBrush, 10, 8, 18, 14);
            Pen penChipPin(Color(255, 200, 200, 200), 1);
            for (int px = 12; px < 26; px += 3) {
                g.DrawLine(&penChipPin, px, 6, px, 8);
                g.DrawLine(&penChipPin, px, 22, px, 24);
            }
        }
        SolidBrush textTitleBrush(titleTextCol);
        g.DrawString(L"Wise Memory Optimizer", -1, &fontTitle, PointF(36.0f, 9.0f), &textTitleBrush);

        // Vector Title bar Buttons: [✉ Feedback], [⚙/Nut Settings], [✕ Close]
        int btnCloseX = width - 36, btnCloseY = 6, btnCloseW = 30, btnCloseH = 24;
        int btnSetX   = width - 72, btnSetY   = 6, btnSetW   = 28, btnSetH   = 24;
        int btnFeedX  = width - 108, btnFeedY  = 6, btnFeedW  = 28, btnFeedH  = 24;

        // Draw Feedback Button (Envelope Vector)
        {
            if (s_titleBtnHover == 1) {
                SolidBrush hovBrush(dark ? Color(255, 55, 55, 55) : Color(255, 238, 242, 246));
                g.FillRectangle(&hovBrush, btnFeedX, btnFeedY, btnFeedW, btnFeedH);
            }
            Pen envPen(dark ? Color(255, 200, 200, 200) : Color(255, 115, 125, 135), 1.3f);
            int ex = btnFeedX + 6, ey = btnFeedY + 6, ew = 16, eh = 12;
            g.DrawRectangle(&envPen, ex, ey, ew, eh);
            Point flapPts[] = { Point(ex, ey), Point(ex + 8, ey + 6), Point(ex + ew, ey) };
            g.DrawLines(&envPen, flapPts, 3);
        }

        // Draw Settings Button (Hexagonal Nut Vector with Circular Hole)
        {
            if (s_titleBtnHover == 2) {
                SolidBrush hovBrush(dark ? Color(255, 55, 55, 55) : Color(255, 238, 242, 246));
                g.FillRectangle(&hovBrush, btnSetX, btnSetY, btnSetW, btnSetH);
            }
            Pen nutPen(dark ? Color(255, 200, 200, 200) : Color(255, 115, 125, 135), 1.4f);
            float cx = (float)btnSetX + 14.0f, cy = (float)btnSetY + 12.0f;
            float rOuter = 6.8f;
            PointF hexPts[6];
            for (int i = 0; i < 6; ++i) {
                float angle = (float)i * 1.04719755f; // 60 degrees in rad
                hexPts[i] = PointF(cx + rOuter * cosf(angle), cy + rOuter * sinf(angle));
            }
            g.DrawPolygon(&nutPen, hexPts, 6);
            g.DrawEllipse(&nutPen, cx - 2.8f, cy - 2.8f, 5.6f, 5.6f);
        }

        // Draw Close Button (Cross Lines Vector)
        {
            if (s_titleBtnHover == 3) {
                SolidBrush hovBrush(Color(255, 232, 17, 35));
                g.FillRectangle(&hovBrush, btnCloseX, btnCloseY, btnCloseW, btnCloseH);
            }
            Pen closePen((s_titleBtnHover == 3) ? Color(255, 255, 255, 255) : (dark ? Color(255, 200, 200, 200) : Color(255, 115, 125, 135)), 1.4f);
            int midX = btnCloseX + btnCloseW / 2, midY = btnCloseY + btnCloseH / 2;
            g.DrawLine(&closePen, midX - 5, midY - 5, midX + 5, midY + 5);
            g.DrawLine(&closePen, midX + 5, midY - 5, midX - 5, midY + 5);
        }

        // 2. Liquid Water Circle Progress (FoxProgressEx)
        int circleD = 180;
        int circleX = (346 - circleD) / 2;
        int circleY = 46;

        float displayP = s_displayLoadPercent;
        if (displayP < 1.0f) displayP = 1.0f;
        if (displayP > 99.0f) displayP = 99.0f;
        float availP = 100.0f - displayP;

        // Clip region to circle
        GraphicsPath circlePath;
        circlePath.AddEllipse((REAL)circleX, (REAL)circleY, (REAL)circleD, (REAL)circleD);
        g.SetClip(&circlePath);

        // Fill background with Solid Vibrant Green (#45BA1F)
        SolidBrush greenBackBrush(Color(255, 69, 186, 31));
        g.FillRectangle(&greenBackBrush, circleX - 2, circleY - 2, circleD + 4, circleD + 4);

        // Water level calculation
        float waterH = ((float)circleD * (displayP / 100.0f));
        float waterBaseline = (float)(circleY + circleD) - waterH;

        // Generate fluid wave surface polygon
        float waveAmp1 = s_isOptimizing ? 6.0f : 2.0f;
        float waveAmp2 = s_isOptimizing ? 3.5f : 1.2f;

        std::vector<PointF> wavePts;
        wavePts.push_back(PointF((REAL)circleX - 10, (REAL)(circleY + circleD + 10)));

        for (int px = circleX - 10; px <= circleX + circleD + 10; px += 4) {
            float py = waterBaseline + 
                waveAmp1 * sinf(0.045f * (float)px + s_animWavePhase) +
                waveAmp2 * cosf(0.035f * (float)px + s_animWavePhase * 1.35f);
            wavePts.push_back(PointF((REAL)px, py));
        }

        wavePts.push_back(PointF((REAL)(circleX + circleD + 10), (REAL)(circleY + circleD + 10)));

        // Fill orange liquid level (#F39C12)
        SolidBrush orangeLiquidBrush(Color(255, 243, 156, 18));
        g.FillPolygon(&orangeLiquidBrush, wavePts.data(), (INT)wavePts.size());

        // Center Percentage Text (e.g. "35%")
        wchar_t pctBuf[16];
        swprintf_s(pctBuf, L"%.0f%%", s_displayLoadPercent);

        RectF pieRect((REAL)circleX, (REAL)circleY, (REAL)circleD, (REAL)circleD);
        StringFormat sfCenter;
        sfCenter.SetAlignment(StringAlignmentCenter);
        sfCenter.SetLineAlignment(StringAlignmentCenter);

        SolidBrush whiteText(Color(255, 255, 255, 255));
        g.DrawString(pctBuf, -1, &fontHuge, pieRect, &sfCenter, &whiteText);

        g.ResetClip();

        // 3. Status Badges
        int cardW = 280;
        int cardH = 34;
        int cardX = (346 - cardW) / 2;

        // Orange Badge (In Use)
        int card1Y = 246;
        Color c1Bg = dark ? Color(255, 65, 45, 20) : Color(255, 254, 243, 226); // #FEF3E2
        Color c1Brd = dark ? Color(255, 120, 80, 40) : Color(255, 254, 215, 170); // #FED7AA
        SolidBrush c1BgBrush(c1Bg);
        Pen c1BrdPen(c1Brd, 1);
        g.FillRectangle(&c1BgBrush, cardX, card1Y, cardW, cardH);
        g.DrawRectangle(&c1BrdPen, cardX, card1Y, cardW, cardH);

        SolidBrush sqOrange(Color(255, 243, 156, 18));
        g.FillRectangle(&sqOrange, cardX + 10, card1Y + 10, 14, 14);

        std::wstring inUseStr = strings.inUse + FormatBytesGB(s_metrics.usedPhysBytes) +
            L" (" + std::to_wstring((int)s_displayLoadPercent) + L"%)";
        SolidBrush textC1(dark ? Color(255, 250, 200, 150) : Color(255, 217, 119, 6)); // #D97706
        g.DrawString(inUseStr.c_str(), -1, &fontBold, PointF((REAL)cardX + 32, (REAL)card1Y + 8), &textC1);

        // Green Badge (Available)
        int card2Y = 288;
        Color c2Bg = dark ? Color(255, 25, 55, 30) : Color(255, 240, 253, 244); // #F0FDF4
        Color c2Brd = dark ? Color(255, 50, 110, 60) : Color(255, 187, 247, 208); // #BBF7D0
        SolidBrush c2BgBrush(c2Bg);
        Pen c2BrdPen(c2Brd, 1);
        g.FillRectangle(&c2BgBrush, cardX, card2Y, cardW, cardH);
        g.DrawRectangle(&c2BrdPen, cardX, card2Y, cardW, cardH);

        SolidBrush sqGreen(Color(255, 69, 186, 31));
        g.FillRectangle(&sqGreen, cardX + 10, card2Y + 10, 14, 14);

        std::wstring availStr = strings.available + FormatBytesGB(s_metrics.availPhysBytes) +
            L" (" + std::to_wstring((int)availP) + L"%)";
        SolidBrush textC2(dark ? Color(255, 170, 235, 170) : Color(255, 22, 163, 74)); // #16A34A
        g.DrawString(availStr.c_str(), -1, &fontBold, PointF((REAL)cardX + 32, (REAL)card2Y + 8), &textC2);

        // 4. Freed Memory Status Line
        int freedY = 330;
        SolidBrush grayText(Color(255, 156, 163, 175)); // #9CA3AF
        SolidBrush greenVal(Color(255, 34, 197, 94));   // #22C55E

        std::wstring strFreedLead = strings.freed;
        std::wstring strFreedVal = s_freedText;
        std::wstring strFreedTail = s_freedTime.empty() ? L"" : (strings.atTime + s_freedTime);

        RectF rLead, rVal, rTail;
        g.MeasureString(strFreedLead.c_str(), -1, &fontSmall, PointF(0, 0), &rLead);
        g.MeasureString(strFreedVal.c_str(), -1, &fontBold, PointF(0, 0), &rVal);
        g.MeasureString(strFreedTail.c_str(), -1, &fontSmall, PointF(0, 0), &rTail);

        float totalW = rLead.Width + rVal.Width + rTail.Width;
        float startX = (346.0f - totalW) / 2.0f;

        g.DrawString(strFreedLead.c_str(), -1, &fontSmall, PointF(startX, (REAL)freedY), &grayText);
        g.DrawString(strFreedVal.c_str(), -1, &fontBold, PointF(startX + rLead.Width, (REAL)freedY), &greenVal);
        g.DrawString(strFreedTail.c_str(), -1, &fontSmall, PointF(startX + rLead.Width + rVal.Width, (REAL)freedY), &grayText);

        // 5. Split Optimize Button
        int btnX = cardX;
        int btnY = 360;
        int btnW = cardW;
        int btnH = 44;

        Color btnNormal(255, 69, 186, 31);   // #45BA1F
        Color btnHover(255, 79, 200, 37);

        Color leftCol = (s_btnOptHover == 1) ? btnHover : btnNormal;
        Color rightCol = (s_btnOptHover == 2) ? btnHover : btnNormal;

        SolidBrush brushBtnL(leftCol);
        SolidBrush brushBtnR(rightCol);

        int splitW = 44;
        int splitX = btnX + btnW - splitW;

        g.FillRectangle(&brushBtnL, btnX, btnY, btnW - splitW, btnH);
        g.FillRectangle(&brushBtnR, splitX, btnY, splitW, btnH);

        Pen divPen(Color(120, 255, 255, 255), 1);
        g.DrawLine(&divPen, splitX, btnY + 7, splitX, btnY + btnH - 7);

        // Vector Shuffle Icon (Two Crossed Curved Arrows with Arrowheads)
        float iconCX = (float)btnX + 54.0f;
        float iconCY = (float)btnY + (float)btnH / 2.0f;

        GraphicsState gState = g.Save();
        if (s_isOptimizing) {
            g.TranslateTransform(iconCX, iconCY);
            g.RotateTransform(s_animIconAngle);
            g.TranslateTransform(-iconCX, -iconCY);
        }

        Pen arrowPen(Color(255, 255, 255, 255), 2.2f);
        arrowPen.SetStartCap(LineCapRound);

        // Upper branch curving down-right
        GraphicsPath path1;
        path1.AddBezier(iconCX - 12.0f, iconCY - 5.0f, iconCX - 3.0f, iconCY - 5.0f, iconCX + 2.0f, iconCY + 5.0f, iconCX + 9.0f, iconCY + 5.0f);
        g.DrawPath(&arrowPen, &path1);

        // Lower branch curving up-right
        GraphicsPath path2;
        path2.AddBezier(iconCX - 12.0f, iconCY + 5.0f, iconCX - 3.0f, iconCY + 5.0f, iconCX + 2.0f, iconCY - 5.0f, iconCX + 9.0f, iconCY - 5.0f);
        g.DrawPath(&arrowPen, &path2);

        // Arrowheads (filled triangles)
        SolidBrush arrowBrush(Color(255, 255, 255, 255));
        PointF triUpper[] = { PointF(iconCX + 13.0f, iconCY - 5.0f), PointF(iconCX + 7.5f, iconCY - 8.5f), PointF(iconCX + 7.5f, iconCY - 1.5f) };
        g.FillPolygon(&arrowBrush, triUpper, 3);

        PointF triLower[] = { PointF(iconCX + 13.0f, iconCY + 5.0f), PointF(iconCX + 7.5f, iconCY + 1.5f), PointF(iconCX + 7.5f, iconCY + 8.5f) };
        g.FillPolygon(&arrowBrush, triLower, 3);

        g.Restore(gState);

        LPCWSTR btnLabel = s_isOptimizing ? strings.optimizing.c_str() : strings.optimize.c_str();
        float textOffX = (s_config.language == 1) ? 70.0f : 78.0f;
        g.DrawString(btnLabel, -1, &fontBtn, PointF((REAL)btnX + textOffX, (REAL)btnY + 12.0f), &whiteText);

        // Right Split Button Vector Chevron ∨
        float chevCX = (float)splitX + (float)splitW / 2.0f;
        float chevCY = (float)btnY + (float)btnH / 2.0f;
        Pen chevPen(Color(255, 255, 255, 255), 1.8f);
        chevPen.SetStartCap(LineCapRound);
        chevPen.SetEndCap(LineCapRound);
        g.DrawLine(&chevPen, chevCX - 5.0f, chevCY - 2.5f, chevCX, chevCY + 2.5f);
        g.DrawLine(&chevPen, chevCX, chevCY + 2.5f, chevCX + 5.0f, chevCY - 2.5f);

        // 6. Footer (Y = 435)
        int footerY = 435;
        g.DrawString(L"v4.2.4.132", -1, &fontSmall, PointF((REAL)btnX, (REAL)footerY), &grayText);

        SolidBrush advBrush(dark ? Color(255, 96, 165, 250) : Color(255, 30, 64, 175)); // Soft Blue in Dark Mode / Dark Navy in Light Mode
        StringFormat sfRight;
        sfRight.SetAlignment(StringAlignmentFar);
        RectF advRect((REAL)btnX, (REAL)footerY, (REAL)btnW, 20.0f);
        LPCWSTR advLabel = strings.advanced.c_str();
        g.DrawString(advLabel, -1, &fontSmall, advRect, &sfRight, &advBrush);

        // 7. Horizontal Accordion Panel (Right side, when width > 346)
        if (width > 350) {
            // Vertical dividing line at X = 346
            Pen vDivPen(dark ? Color(255, 55, 65, 81) : Color(255, 229, 231, 235), 1);
            g.DrawLine(&vDivPen, 346, 40, 346, 445);

            // Dashed horizontal dividing line at Y = 160
            Pen dashedPen(dark ? Color(255, 55, 65, 81) : Color(255, 229, 231, 235), 1);
            REAL dashPattern[] = { 4.0f, 4.0f };
            dashedPen.SetDashPattern(dashPattern, 2);
            g.DrawLine(&dashedPen, 368, 160, 776, 160);

            // 5 Metric Columns (Top Section)
            struct MetricCol {
                Color dotColor;
                std::wstring label;
                std::wstring value;
            } cols[5] = {
                { Color(255, 156, 163, 175), strings.colHardwareReserved, L"0 Bytes" },
                { Color(255, 245, 158, 11),  strings.colInUsed, FormatBytesGB(s_metrics.usedPhysBytes) },
                { Color(255, 234, 179, 8),   strings.colModified, FormatBytesMB(s_metrics.modifiedEstimateBytes) },
                { Color(255, 59, 130, 246),  strings.colStandby, FormatBytesGB(s_metrics.standbyEstimateBytes) },
                { Color(255, 34, 197, 94),   strings.colFree, FormatBytesGB(s_metrics.freeEstimateBytes) }
            };

            int colStartX = 368;
            int colStep = 82;
            int colY = 184;

            StringFormat sfColCenter;
            sfColCenter.SetAlignment(StringAlignmentCenter);

            SolidBrush labelColBrush(dark ? Color(255, 200, 200, 200) : Color(255, 55, 65, 81));
            SolidBrush valColBrush(dark ? Color(255, 240, 240, 240) : Color(255, 17, 24, 39));

            for (int i = 0; i < 5; ++i) {
                int cx = colStartX + i * colStep;
                int dotD = 10;
                SolidBrush dotBrush(cols[i].dotColor);
                g.FillEllipse(&dotBrush, cx, colY, dotD, dotD);

                RectF rLbl((REAL)cx + 14, (REAL)colY - 2, 70.0f, 32.0f);
                g.DrawString(cols[i].label.c_str(), -1, &fontSmall, rLbl, NULL, &labelColBrush);

                RectF rVal((REAL)cx + 14, (REAL)colY + (i == 0 ? 32 : 18), 70.0f, 20.0f);
                g.DrawString(cols[i].value.c_str(), -1, &fontSmall, rVal, NULL, &valColBrush);
            }

            // Bottom Rounded Dashed Box
            int boxX = 368, boxY = 275, boxW = 408, boxH = 110;
            Pen boxDashedPen(dark ? Color(255, 55, 65, 81) : Color(255, 226, 232, 240), 1);
            REAL boxPattern[] = { 3.0f, 3.0f };
            boxDashedPen.SetDashPattern(boxPattern, 2);
            g.DrawRectangle(&boxDashedPen, boxX, boxY, boxW, boxH);

            // 2x2 Grid inside Box
            SolidBrush gridLblBrush(dark ? Color(255, 170, 175, 185) : Color(255, 75, 85, 99));
            SolidBrush gridValBrush(dark ? Color(255, 235, 240, 245) : Color(255, 31, 41, 55));

            int row1Y = boxY + 22;
            int row2Y = boxY + 66;
            int c1X = boxX + 24;
            int c2X = boxX + 210;

            // Row 1
            std::wstring strAvail = strings.available + FormatBytesGB(s_metrics.availPhysBytes);
            std::wstring strCached = strings.colCached + FormatBytesGB(s_metrics.systemCacheBytes);
            g.DrawString(strAvail.c_str(), -1, &fontBold, PointF((REAL)c1X, (REAL)row1Y), &gridValBrush);
            g.DrawString(strCached.c_str(), -1, &fontBold, PointF((REAL)c2X, (REAL)row1Y), &gridValBrush);

            // Row 2
            std::wstring strTotal = strings.colTotal + FormatBytesGB(s_metrics.totalPhysBytes);
            std::wstring strInstalled = strings.colInstalled + FormatBytesGB(s_metrics.totalPhysBytes);
            g.DrawString(strTotal.c_str(), -1, &fontBold, PointF((REAL)c1X, (REAL)row2Y), &gridValBrush);
            g.DrawString(strInstalled.c_str(), -1, &fontBold, PointF((REAL)c2X, (REAL)row2Y), &gridValBrush);
        }

        // Outer 1px window border
        Pen borderPen(borderCol, 1);
        g.DrawRectangle(&borderPen, 0, 0, width - 1, height - 1);
    }

    BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBM);
    DeleteObject(memBM);
    DeleteDC(memDC);

    EndPaint(hWnd, &ps);
}

LRESULT CALLBACK MainWindow::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
        OnPaint(hWnd);
        return 0;

    case WM_TIMER:
        if (wParam == TIMER_ANIM_ID) {
            OnAnimationTick(hWnd);
        } else if (wParam == TIMER_METRICS_ID) {
            if (!s_isOptimizing) {
                s_metrics = MemoryEngine::QueryMetrics();
                s_targetLoadPercent = (float)s_metrics.loadPercentage;
                UpdateTrayIcon(hWnd);

                // Auto-Optimization check
                if (s_config.autoOptEnabled) {
                    ULONGLONG now = GetTickCount64();
                    if (s_config.autoOptType == 1) { // Interval
                        ULONGLONG intervalMs = (ULONGLONG)s_config.autoOptMinutes * 60ULL * 1000ULL;
                        if (now - s_lastAutoOptTick >= intervalMs) {
                            s_lastAutoOptTick = now;
                            StartOptimization(hWnd);
                        }
                    } else { // Threshold
                        int availPercent = 100 - (int)s_displayLoadPercent;
                        if (availPercent <= s_config.autoOptThreshold) {
                            bool canRun = true;
                            if (s_config.autoOptCpuIdle) {
                                double cpu = MemoryEngine::GetCpuUsage();
                                if (cpu > 15.0) canRun = false;
                            }
                            if (canRun && (now - s_lastAutoOptTick >= 60000ULL)) {
                                s_lastAutoOptTick = now;
                                StartOptimization(hWnd);
                            }
                        }
                    }
                }
            }
        }
        return 0;

    case WM_OPT_FINISHED: {
        std::wstring* pFreed = (std::wstring*)wParam;
        if (pFreed) {
            s_freedText = *pFreed;
            delete pFreed;
        }
        s_lastOptTimestamp = time(NULL);
        s_freedTime = FormatTimestamp(s_lastOptTimestamp, s_config.language);
        s_metrics = MemoryEngine::QueryMetrics();
        s_targetLoadPercent = (float)s_metrics.loadPercentage;
        s_isOptimizing = false;

        UpdateTrayIcon(hWnd);
        InvalidateRect(hWnd, NULL, TRUE);

        if (s_config.showNotificationOnOpt && !s_config.notShowNoticeAgain) {
            NoticeWindow::Show(hWnd, s_freedText, (int)s_displayLoadPercent, &s_config.notShowNoticeAgain, s_config.language, s_config.darkMode);
            if (s_config.notShowNoticeAgain) s_config.Save();
        }
        return 0;
    }

    case WM_HOTKEY:
        if (wParam == 9001) {
            StartOptimization(hWnd);
        }
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            SendMessageW(hWnd, WM_CLOSE, 0, 0);
            return 0;
        }
        break;

    case WM_TRAYICON_MSG:
        if (lParam == WM_LBUTTONUP || lParam == WM_LBUTTONDBLCLK) {
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
        } else if (lParam == WM_RBUTTONUP) {
            ShowTrayMenu(hWnd);
        }
        return 0;

    case WM_LBUTTONDOWN: {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        RECT rcClient;
        GetClientRect(hWnd, &rcClient);
        int w = rcClient.right - rcClient.left;
        LogDbg(L"WM_LBUTTONDOWN x=%d, y=%d, w=%d", x, y, w);

        // Titlebar Drag & Buttons: 0 <= y <= 32
        if (y <= 32) {
            if (x >= w - 108 && x <= w - 80 && y >= 6 && y <= 30) {
                FeedbackDialog::Show(hWnd, s_config);
                return 0;
            } else if (x >= w - 72 && x <= w - 44 && y >= 6 && y <= 30) {
                UnregisterAppHotkey(hWnd);
                SettingsDialog::Show(hWnd, s_config);
                s_lastAutoOptTick = GetTickCount64();
                s_freedTime = FormatTimestamp(s_lastOptTimestamp, s_config.language);
                BOOL useDark = s_config.darkMode ? TRUE : FALSE;
                DwmSetWindowAttribute(hWnd, 20, &useDark, sizeof(useDark));
                DwmSetWindowAttribute(hWnd, 19, &useDark, sizeof(useDark));
                RegisterAppHotkey(hWnd);
                UpdateTrayIcon(hWnd);
                InvalidateRect(hWnd, NULL, TRUE);
                return 0;
            } else if (x >= w - 36 && x <= w - 6 && y >= 6 && y <= 30) {
                SendMessageW(hWnd, WM_CLOSE, 0, 0);
                return 0;
            } else {
                ReleaseCapture();
                SendMessageW(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
                return 0;
            }
        }

        // Split Optimize button
        int cardW = 280;
        int btnX = (346 - cardW) / 2;
        int btnY = 360;
        int btnW = cardW;
        int btnH = 44;
        if (x >= btnX && x <= btnX + btnW && y >= btnY && y <= btnY + btnH) {
            if (x >= btnX + btnW - 44) {
                RECT rc;
                GetWindowRect(hWnd, &rc);
                ShowDropdownMenu(hWnd, rc.left + btnX + btnW - 44, rc.top + btnY + btnH);
            } else {
                if (!s_config.clearMem && !s_config.clearStandby && !s_config.defragMem) {
                    AppStrings str = Localization::GetStrings(s_config.language);
                    MessageBoxW(hWnd, str.warnNoOpt.c_str(), str.appTitle.c_str(), MB_OK | MB_ICONWARNING);
                } else {
                    StartOptimization(hWnd);
                }
            }
            return 0;
        }

        // Version label click to open About Dialog (x from btnX to btnX + 70, y from 430 to 455)
        if (x >= btnX && x <= btnX + 70 && y >= 430 && y <= 455) {
            AboutDialog::Show(hWnd, s_config);
            return 0;
        }

        // Advanced link (x from 240 to 325, y from 430 to 455)
        if (x >= 240 && x <= 325 && y >= 430 && y <= 455) {
            ToggleAdvanced(hWnd);
            return 0;
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        if (!s_isTrackingMouse) {
            TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT) };
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hWnd;
            TrackMouseEvent(&tme);
            s_isTrackingMouse = true;
        }

        RECT rcClient;
        GetClientRect(hWnd, &rcClient);
        int w = rcClient.right - rcClient.left;

        int oldBtnHov = s_btnOptHover;
        int oldTitleHov = s_titleBtnHover;

        if (y >= 6 && y <= 30) {
            if (x >= w - 108 && x <= w - 80) s_titleBtnHover = 1;
            else if (x >= w - 72 && x <= w - 44) s_titleBtnHover = 2;
            else if (x >= w - 36 && x <= w - 6) s_titleBtnHover = 3;
            else s_titleBtnHover = 0;
        } else {
            s_titleBtnHover = 0;
        }

        int cardW = 280;
        int btnX = (346 - cardW) / 2;
        int btnY = 360;
        int btnW = cardW;
        int btnH = 44;
        if (x >= btnX && x <= btnX + btnW && y >= btnY && y <= btnY + btnH) {
            if (x >= btnX + btnW - 44) s_btnOptHover = 2;
            else s_btnOptHover = 1;
        } else {
            s_btnOptHover = 0;
        }

        if (s_btnOptHover != oldBtnHov || s_titleBtnHover != oldTitleHov) {
            InvalidateRect(hWnd, NULL, FALSE);
        }

        if ((x >= 240 && x <= 325 && y >= 430 && y <= 455) || s_btnOptHover > 0 || s_titleBtnHover > 0) {
            SetCursor(LoadCursor(NULL, IDC_HAND));
        } else {
            SetCursor(LoadCursor(NULL, IDC_ARROW));
        }
        return 0;
    }

    case WM_MOUSELEAVE: {
        s_isTrackingMouse = false;
        if (s_btnOptHover != 0 || s_titleBtnHover != 0) {
            s_btnOptHover = 0;
            s_titleBtnHover = 0;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return 0;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        switch (id) {
        case IDM_OPT_TRIM:
            s_config.clearMem = !s_config.clearMem;
            s_config.Save();
            break;
        case IDM_OPT_STANDBY:
            s_config.clearStandby = !s_config.clearStandby;
            s_config.Save();
            break;
        case IDM_OPT_DEFRAG:
            s_config.defragMem = !s_config.defragMem;
            s_config.Save();
            break;

        case IDM_TRAY_OPT_NOW:
            if (!s_config.clearMem && !s_config.clearStandby && !s_config.defragMem) {
                AppStrings str = Localization::GetStrings(s_config.language);
                MessageBoxW(hWnd, str.warnNoOpt.c_str(), str.appTitle.c_str(), MB_OK | MB_ICONWARNING);
            } else {
                StartOptimization(hWnd);
            }
            break;

        case IDM_TRAY_OPEN:
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
            break;

        case IDM_TRAY_SETTINGS:
            UnregisterAppHotkey(hWnd);
            SettingsDialog::Show(hWnd, s_config);
            s_lastAutoOptTick = GetTickCount64();
            s_freedTime = FormatTimestamp(s_lastOptTimestamp, s_config.language);
            {
                BOOL useDark = s_config.darkMode ? TRUE : FALSE;
                DwmSetWindowAttribute(hWnd, 20, &useDark, sizeof(useDark));
                DwmSetWindowAttribute(hWnd, 19, &useDark, sizeof(useDark));
            }
            RegisterAppHotkey(hWnd);
            UpdateTrayIcon(hWnd);
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case IDM_TRAY_ABOUT:
            AboutDialog::Show(hWnd, s_config);
            break;

        case IDM_TRAY_AUTO_DIS:
            s_config.autoOptEnabled = false;
            s_lastAutoOptTick = GetTickCount64();
            s_config.Save();
            break;
        case IDM_TRAY_AUTO_LOW:
            s_config.autoOptEnabled = true;
            s_config.autoOptType = 0;
            s_lastAutoOptTick = GetTickCount64();
            s_config.Save();
            break;
        case IDM_TRAY_AUTO_5:
            s_config.autoOptEnabled = true;
            s_config.autoOptType = 1;
            s_config.autoOptMinutes = 5;
            s_lastAutoOptTick = GetTickCount64();
            s_config.Save();
            break;
        case IDM_TRAY_AUTO_10:
            s_config.autoOptEnabled = true;
            s_config.autoOptType = 1;
            s_config.autoOptMinutes = 10;
            s_lastAutoOptTick = GetTickCount64();
            s_config.Save();
            break;
        case IDM_TRAY_AUTO_30:
            s_config.autoOptEnabled = true;
            s_config.autoOptType = 1;
            s_config.autoOptMinutes = 30;
            s_lastAutoOptTick = GetTickCount64();
            s_config.Save();
            break;
        case IDM_TRAY_AUTO_60:
            s_config.autoOptEnabled = true;
            s_config.autoOptType = 1;
            s_config.autoOptMinutes = 60;
            s_lastAutoOptTick = GetTickCount64();
            s_config.Save();
            break;

        case IDM_TRAY_EXIT:
            DestroyWindow(hWnd);
            break;
        }
        return 0;
    }

    case WM_MOVE: {
        RECT rc;
        GetWindowRect(hWnd, &rc);
        s_config.windowLeft = rc.left;
        s_config.windowTop = rc.top;
        return 0;
    }

    case WM_CLOSE:
        if (s_config.minimizeToTrayOnClose) {
            ShowWindow(hWnd, SW_HIDE);
        } else {
            DestroyWindow(hWnd);
        }
        return 0;

    case WM_DESTROY:
        s_config.Save();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}
