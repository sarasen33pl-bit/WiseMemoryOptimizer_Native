#include "SettingsDialog.h"
#include "Localization.h"
#include <commctrl.h>
#include <dwmapi.h>
#include <string>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")

#define IDC_CHK_MIN_TRAY      2001
#define IDC_CHK_STARTUP       2002
#define IDC_CHK_CLIPBOARD     2003
#define IDC_CHK_UPDATE        2004
#define IDC_CHK_NOTIFY        2005
#define IDC_CHK_DARKMODE      2006
#define IDC_CHK_TRAYUSAGE     2007
#define IDC_CHK_SHORTCUT      2008
#define IDC_CMB_SHORTMOD      2009
#define IDC_CMB_SHORTKEY      2010

#define IDC_CHK_AUTOOPT       2011
#define IDC_RAD_THRESHOLD     2012
#define IDC_SLD_THRESHOLD     2013
#define IDC_LBL_THRESHVAL     2014
#define IDC_CHK_CPUIDLE       2015
#define IDC_RAD_INTERVAL      2016
#define IDC_CMB_INTERVAL      2017

#define IDC_LBL_LANG          2018
#define IDC_CMB_LANG          2019
#define IDC_BTN_OK            2020

AppConfig* SettingsDialog::s_pConfig = nullptr;
static HFONT s_hFont = NULL;
static HFONT s_hFontBold = NULL;
static HBRUSH s_hBgBrush = NULL;
static COLORREF s_bgColor = RGB(255, 255, 255);
static COLORREF s_textColor = RGB(35, 35, 35);
static int s_currentDlgLang = 0;
static int s_div1Y = 0;
static int s_div2Y = 0;
static int s_div3Y = 0;

bool SettingsDialog::Show(HWND hParent, AppConfig& config) {
    s_pConfig = &config;
    s_currentDlgLang = config.language;
    AppStrings strings = Localization::GetStrings(s_currentDlgLang);

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = (WNDPROC)DialogProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = L"WiseSettingsClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hIcon = LoadIcon(GetModuleHandleW(NULL), MAKEINTRESOURCEW(1));
    RegisterClassExW(&wc);

    int clientW = 390;
    int clientH = 460;
    RECT rcWin = { 0, 0, clientW, clientH };
    AdjustWindowRectEx(&rcWin, WS_POPUP | WS_CAPTION | WS_SYSMENU, FALSE, WS_EX_DLGMODALFRAME);
    int winW = rcWin.right - rcWin.left;
    int winH = rcWin.bottom - rcWin.top;

    int scrW = GetSystemMetrics(SM_CXSCREEN);
    int scrH = GetSystemMetrics(SM_CYSCREEN);
    int x = (scrW - winW) / 2;
    int y = (scrH - winH) / 2;
    if (hParent && IsWindow(hParent)) {
        RECT rcParent;
        GetWindowRect(hParent, &rcParent);
        x = rcParent.left + (rcParent.right - rcParent.left - winW) / 2;
        y = rcParent.top + (rcParent.bottom - rcParent.top - winH) / 2;
    }

    HWND hWnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        L"WiseSettingsClass",
        strings.settingsTitle.c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        x, y, winW, winH,
        hParent, NULL, GetModuleHandleW(NULL), NULL
    );

    if (!hWnd) return false;

    BOOL useDark = config.darkMode ? TRUE : FALSE;
    DwmSetWindowAttribute(hWnd, 20, &useDark, sizeof(useDark));
    DwmSetWindowAttribute(hWnd, 19, &useDark, sizeof(useDark));

    if (hParent && IsWindow(hParent)) EnableWindow(hParent, FALSE);

    MSG msg;
    while (IsWindow(hWnd) && GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (hParent && IsWindow(hParent)) {
        EnableWindow(hParent, TRUE);
        SetActiveWindow(hParent);
    }

    return true;
}

void SettingsDialog::InitControls(HWND hDlg, const AppConfig& cfg) {
    s_hFont = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    s_hFontBold = CreateFontW(15, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    if (cfg.darkMode) {
        s_bgColor = RGB(35, 35, 35);
        s_textColor = RGB(230, 230, 230);
    } else {
        s_bgColor = RGB(255, 255, 255);
        s_textColor = RGB(35, 35, 35);
    }
    s_hBgBrush = CreateSolidBrush(s_bgColor);

    HINSTANCE hInst = GetModuleHandleW(NULL);
    AppStrings strings = Localization::GetStrings(s_currentDlgLang);

    int curY = 16;
    const int itemH = 22;
    const int chkX = 26;

    auto MakeChk = [&](int id, const std::wstring& text, bool checked, int indent = 0) -> HWND {
        HWND h = CreateWindowExW(0, L"BUTTON", text.c_str(),
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            chkX + indent, curY, 340 - indent, itemH,
            hDlg, (HMENU)(UINT_PTR)id, hInst, NULL);
        SendMessageW(h, WM_SETFONT, (WPARAM)s_hFont, TRUE);
        SendMessageW(h, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
        curY += itemH + 3;
        return h;
    };

    MakeChk(IDC_CHK_MIN_TRAY, strings.minToTray, cfg.minimizeToTrayOnClose);
    MakeChk(IDC_CHK_STARTUP, strings.runAtStartup, cfg.runAtStartup);
    MakeChk(IDC_CHK_CLIPBOARD, strings.clearClipboard, cfg.clearClipboardOnOpt);
    MakeChk(IDC_CHK_UPDATE, strings.autoCheckUpdate, cfg.autoCheckUpdate);
    MakeChk(IDC_CHK_NOTIFY, strings.showNotice, cfg.showNotificationOnOpt);
    MakeChk(IDC_CHK_DARKMODE, strings.darkMode, cfg.darkMode);
    MakeChk(IDC_CHK_TRAYUSAGE, strings.showUsageInTray, cfg.showUsageInTray);

    // Shortcut line
    HWND hChkShort = CreateWindowExW(0, L"BUTTON", strings.shortcutKey.c_str(),
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        chkX, curY, 115, itemH, hDlg, (HMENU)IDC_CHK_SHORTCUT, hInst, NULL);
    SendMessageW(hChkShort, WM_SETFONT, (WPARAM)s_hFont, TRUE);
    SendMessageW(hChkShort, BM_SETCHECK, cfg.shortcutEnabled ? BST_CHECKED : BST_UNCHECKED, 0);

    HWND hCmbMod = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        chkX + 120, curY - 1, 105, 120, hDlg, (HMENU)IDC_CMB_SHORTMOD, hInst, NULL);
    SendMessageW(hCmbMod, WM_SETFONT, (WPARAM)s_hFont, TRUE);
    SendMessageW(hCmbMod, CB_ADDSTRING, 0, (LPARAM)L"CTRL+ALT");
    SendMessageW(hCmbMod, CB_ADDSTRING, 0, (LPARAM)L"ALT+SHIFT");
    SendMessageW(hCmbMod, CB_ADDSTRING, 0, (LPARAM)L"CTRL+SHIFT");
    SendMessageW(hCmbMod, CB_SETCURSEL, cfg.shortcutMod, 0);

    HWND hLblPlus = CreateWindowExW(0, L"STATIC", L"+",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        chkX + 228, curY + 1, 14, itemH, hDlg, NULL, hInst, NULL);
    SendMessageW(hLblPlus, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    HWND hCmbKey = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        chkX + 246, curY - 1, 60, 160, hDlg, (HMENU)IDC_CMB_SHORTKEY, hInst, NULL);
    SendMessageW(hCmbKey, WM_SETFONT, (WPARAM)s_hFont, TRUE);
    for (wchar_t c = L'A'; c <= L'Z'; ++c) {
        wchar_t buf[2] = { c, 0 };
        SendMessageW(hCmbKey, CB_ADDSTRING, 0, (LPARAM)buf);
    }
    SendMessageW(hCmbKey, CB_SETCURSEL, (cfg.shortcutKey >= L'A' && cfg.shortcutKey <= L'Z') ? (cfg.shortcutKey - L'A') : 0, 0);

    curY += itemH + 8;

    // Divider 1
    s_div1Y = curY;
    curY += 8;

    // Auto Optimization Checkbox
    MakeChk(IDC_CHK_AUTOOPT, strings.autoOpt, cfg.autoOptEnabled);

    // Radio 1: Threshold
    HWND hRadThresh = CreateWindowExW(0, L"BUTTON", strings.whenAvailLess.c_str(),
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        chkX + 16, curY, 320, itemH, hDlg, (HMENU)IDC_RAD_THRESHOLD, hInst, NULL);
    SendMessageW(hRadThresh, WM_SETFONT, (WPARAM)s_hFont, TRUE);
    SendMessageW(hRadThresh, BM_SETCHECK, (cfg.autoOptType == 0) ? BST_CHECKED : BST_UNCHECKED, 0);
    curY += itemH + 2;

    // Slider for threshold
    HWND hSlider = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
        WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS,
        chkX + 35, curY, 175, 26, hDlg, (HMENU)IDC_SLD_THRESHOLD, hInst, NULL);
    SendMessageW(hSlider, TBM_SETRANGE, TRUE, MAKELPARAM(5, 50));
    SendMessageW(hSlider, TBM_SETPOS, TRUE, cfg.autoOptThreshold);
    SendMessageW(hSlider, TBM_SETTICFREQ, 5, 0);

    std::wstring tStr = std::to_wstring(cfg.autoOptThreshold) + L" %";
    HWND hLblVal = CreateWindowExW(0, L"STATIC", tStr.c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        chkX + 220, curY + 3, 50, itemH, hDlg, (HMENU)IDC_LBL_THRESHVAL, hInst, NULL);
    SendMessageW(hLblVal, WM_SETFONT, (WPARAM)s_hFont, TRUE);
    curY += 28;

    // Checkbox: CPU idle
    HWND hChkIdle = CreateWindowExW(0, L"BUTTON", strings.cpuIdle.c_str(),
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        chkX + 35, curY, 300, itemH, hDlg, (HMENU)IDC_CHK_CPUIDLE, hInst, NULL);
    SendMessageW(hChkIdle, WM_SETFONT, (WPARAM)s_hFont, TRUE);
    SendMessageW(hChkIdle, BM_SETCHECK, cfg.autoOptCpuIdle ? BST_CHECKED : BST_UNCHECKED, 0);
    curY += itemH + 6;

    // Radio 2: Interval
    HWND hRadInterval = CreateWindowExW(0, L"BUTTON", strings.optEvery.c_str(),
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        chkX + 16, curY, 170, itemH, hDlg, (HMENU)IDC_RAD_INTERVAL, hInst, NULL);
    SendMessageW(hRadInterval, WM_SETFONT, (WPARAM)s_hFont, TRUE);
    SendMessageW(hRadInterval, BM_SETCHECK, (cfg.autoOptType == 1) ? BST_CHECKED : BST_UNCHECKED, 0);

    HWND hCmbInterval = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        chkX + 195, curY - 2, 115, 150, hDlg, (HMENU)IDC_CMB_INTERVAL, hInst, NULL);
    SendMessageW(hCmbInterval, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    int intervals[] = { 5, 10, 15, 30, 45, 60, 120 };
    int selIdx = 4; // default 45 min
    for (int i = 0; i < 7; ++i) {
        std::wstring s = std::to_wstring(intervals[i]) + L" " + strings.minuteUnit;
        SendMessageW(hCmbInterval, CB_ADDSTRING, 0, (LPARAM)s.c_str());
        if (intervals[i] == cfg.autoOptMinutes) selIdx = i;
    }
    SendMessageW(hCmbInterval, CB_SETCURSEL, selIdx, 0);
    curY += itemH + 12;

    // Divider 2
    s_div2Y = curY;
    curY += 10;

    // Language
    HWND hLblLang = CreateWindowExW(0, L"STATIC", strings.language.c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        chkX, curY + 2, 80, itemH, hDlg, (HMENU)IDC_LBL_LANG, hInst, NULL);
    SendMessageW(hLblLang, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    HWND hCmbLang = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        chkX + 90, curY, 160, 120, hDlg, (HMENU)IDC_CMB_LANG, hInst, NULL);
    SendMessageW(hCmbLang, WM_SETFONT, (WPARAM)s_hFont, TRUE);
    SendMessageW(hCmbLang, CB_ADDSTRING, 0, (LPARAM)L"English");
    SendMessageW(hCmbLang, CB_ADDSTRING, 0, (LPARAM)L"العربية (Arabic)");
    SendMessageW(hCmbLang, CB_SETCURSEL, cfg.language, 0);

    curY += itemH + 12;

    // Divider 3
    s_div3Y = curY;
    curY += 8;

    // OK Button
    HWND hBtnOk = CreateWindowExW(0, L"BUTTON", strings.btnOK.c_str(),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        285, curY, 80, 28, hDlg, (HMENU)IDC_BTN_OK, hInst, NULL);
    SendMessageW(hBtnOk, WM_SETFONT, (WPARAM)s_hFontBold, TRUE);

    UpdateAutoOptStates(hDlg);
}

void SettingsDialog::UpdateAutoOptStates(HWND hDlg) {
    BOOL autoOpt = (IsDlgButtonChecked(hDlg, IDC_CHK_AUTOOPT) == BST_CHECKED);
    BOOL thresh  = (IsDlgButtonChecked(hDlg, IDC_RAD_THRESHOLD) == BST_CHECKED);
    BOOL interv  = (IsDlgButtonChecked(hDlg, IDC_RAD_INTERVAL) == BST_CHECKED);

    EnableWindow(GetDlgItem(hDlg, IDC_RAD_THRESHOLD), autoOpt);
    EnableWindow(GetDlgItem(hDlg, IDC_SLD_THRESHOLD), autoOpt && thresh);
    EnableWindow(GetDlgItem(hDlg, IDC_CHK_CPUIDLE), autoOpt && thresh);

    EnableWindow(GetDlgItem(hDlg, IDC_RAD_INTERVAL), autoOpt);
    EnableWindow(GetDlgItem(hDlg, IDC_CMB_INTERVAL), autoOpt && interv);

    BOOL shortEnabled = (IsDlgButtonChecked(hDlg, IDC_CHK_SHORTCUT) == BST_CHECKED);
    EnableWindow(GetDlgItem(hDlg, IDC_CMB_SHORTMOD), shortEnabled);
    EnableWindow(GetDlgItem(hDlg, IDC_CMB_SHORTKEY), shortEnabled);
}

void SettingsDialog::SaveSettings(HWND hDlg, AppConfig& cfg) {
    cfg.minimizeToTrayOnClose = (IsDlgButtonChecked(hDlg, IDC_CHK_MIN_TRAY) == BST_CHECKED);
    cfg.runAtStartup          = (IsDlgButtonChecked(hDlg, IDC_CHK_STARTUP) == BST_CHECKED);
    cfg.clearClipboardOnOpt   = (IsDlgButtonChecked(hDlg, IDC_CHK_CLIPBOARD) == BST_CHECKED);
    cfg.autoCheckUpdate       = (IsDlgButtonChecked(hDlg, IDC_CHK_UPDATE) == BST_CHECKED);
    cfg.showNotificationOnOpt = (IsDlgButtonChecked(hDlg, IDC_CHK_NOTIFY) == BST_CHECKED);
    cfg.darkMode              = (IsDlgButtonChecked(hDlg, IDC_CHK_DARKMODE) == BST_CHECKED);
    cfg.showUsageInTray       = (IsDlgButtonChecked(hDlg, IDC_CHK_TRAYUSAGE) == BST_CHECKED);
    cfg.shortcutEnabled       = (IsDlgButtonChecked(hDlg, IDC_CHK_SHORTCUT) == BST_CHECKED);

    HWND hCmbMod = GetDlgItem(hDlg, IDC_CMB_SHORTMOD);
    int modIdx = (int)SendMessageW(hCmbMod, CB_GETCURSEL, 0, 0);
    if (modIdx >= 0) cfg.shortcutMod = modIdx;

    HWND hCmbKey = GetDlgItem(hDlg, IDC_CMB_SHORTKEY);
    int keyIdx = (int)SendMessageW(hCmbKey, CB_GETCURSEL, 0, 0);
    if (keyIdx >= 0 && keyIdx < 26) cfg.shortcutKey = (wchar_t)(L'A' + keyIdx);

    cfg.autoOptEnabled = (IsDlgButtonChecked(hDlg, IDC_CHK_AUTOOPT) == BST_CHECKED);
    cfg.autoOptType    = (IsDlgButtonChecked(hDlg, IDC_RAD_INTERVAL) == BST_CHECKED) ? 1 : 0;

    HWND hSlider = GetDlgItem(hDlg, IDC_SLD_THRESHOLD);
    cfg.autoOptThreshold = (int)SendMessageW(hSlider, TBM_GETPOS, 0, 0);

    cfg.autoOptCpuIdle = (IsDlgButtonChecked(hDlg, IDC_CHK_CPUIDLE) == BST_CHECKED);

    HWND hCmbInt = GetDlgItem(hDlg, IDC_CMB_INTERVAL);
    int intIdx = (int)SendMessageW(hCmbInt, CB_GETCURSEL, 0, 0);
    int intervals[] = { 5, 10, 15, 30, 45, 60, 120 };
    if (intIdx >= 0 && intIdx < 7) cfg.autoOptMinutes = intervals[intIdx];

    HWND hCmbLang = GetDlgItem(hDlg, IDC_CMB_LANG);
    int langIdx = (int)SendMessageW(hCmbLang, CB_GETCURSEL, 0, 0);
    if (langIdx >= 0) cfg.language = langIdx;

    cfg.Save();
}

LRESULT CALLBACK SettingsDialog::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hDlg, &rc);
        FillRect(hdc, &rc, s_hBgBrush);
        return 1;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hDlg, &ps);
        RECT rc;
        GetClientRect(hDlg, &rc);
        FillRect(hdc, &rc, s_hBgBrush);

        bool isDark = (IsDlgButtonChecked(hDlg, IDC_CHK_DARKMODE) == BST_CHECKED);
        COLORREF divCol = isDark ? RGB(65, 65, 65) : RGB(225, 225, 225);
        HPEN hPen = CreatePen(PS_SOLID, 1, divCol);
        HPEN oldPen = (HPEN)SelectObject(hdc, hPen);

        if (s_div1Y > 0) {
            MoveToEx(hdc, 26, s_div1Y, NULL);
            LineTo(hdc, 366, s_div1Y);
        }
        if (s_div2Y > 0) {
            MoveToEx(hdc, 26, s_div2Y, NULL);
            LineTo(hdc, 366, s_div2Y);
        }
        if (s_div3Y > 0) {
            MoveToEx(hdc, 0, s_div3Y, NULL);
            LineTo(hdc, rc.right, s_div3Y);
        }

        SelectObject(hdc, oldPen);
        DeleteObject(hPen);
        EndPaint(hDlg, &ps);
        return 0;
    }

    case WM_CREATE:
        if (s_pConfig) InitControls(hDlg, *s_pConfig);
        return 0;

    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, s_textColor);
        return (LRESULT)s_hBgBrush;
    }

    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, s_bgColor);
        SetTextColor(hdc, s_textColor);
        return (LRESULT)s_hBgBrush;
    }

    case WM_HSCROLL: {
        HWND hSlider = GetDlgItem(hDlg, IDC_SLD_THRESHOLD);
        if ((HWND)lParam == hSlider) {
            int pos = (int)SendMessageW(hSlider, TBM_GETPOS, 0, 0);
            std::wstring tStr = std::to_wstring(pos) + L" %";
            SetWindowTextW(GetDlgItem(hDlg, IDC_LBL_THRESHVAL), tStr.c_str());
        }
        return 0;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);

        if (id == IDC_CHK_AUTOOPT || id == IDC_RAD_THRESHOLD || id == IDC_RAD_INTERVAL || id == IDC_CHK_SHORTCUT) {
            UpdateAutoOptStates(hDlg);
        }

        if (id == IDC_CHK_DARKMODE) {
            BOOL isDark = (IsDlgButtonChecked(hDlg, IDC_CHK_DARKMODE) == BST_CHECKED);
            if (s_pConfig) s_pConfig->darkMode = (isDark != FALSE);
            if (isDark) {
                s_bgColor = RGB(35, 35, 35);
                s_textColor = RGB(230, 230, 230);
            } else {
                s_bgColor = RGB(255, 255, 255);
                s_textColor = RGB(35, 35, 35);
            }
            if (s_hBgBrush) DeleteObject(s_hBgBrush);
            s_hBgBrush = CreateSolidBrush(s_bgColor);

            DwmSetWindowAttribute(hDlg, 20, &isDark, sizeof(isDark));
            DwmSetWindowAttribute(hDlg, 19, &isDark, sizeof(isDark));
            RedrawWindow(hDlg, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
        }

        if (id == IDC_CMB_LANG && code == CBN_SELCHANGE) {
            int newLang = (int)SendMessageW(GetDlgItem(hDlg, IDC_CMB_LANG), CB_GETCURSEL, 0, 0);
            if (newLang >= 0 && newLang != s_currentDlgLang) {
                s_currentDlgLang = newLang;
                AppStrings str = Localization::GetStrings(s_currentDlgLang);
                SetWindowTextW(hDlg, str.settingsTitle.c_str());
                SetWindowTextW(GetDlgItem(hDlg, IDC_CHK_MIN_TRAY), str.minToTray.c_str());
                SetWindowTextW(GetDlgItem(hDlg, IDC_CHK_STARTUP), str.runAtStartup.c_str());
                SetWindowTextW(GetDlgItem(hDlg, IDC_CHK_CLIPBOARD), str.clearClipboard.c_str());
                SetWindowTextW(GetDlgItem(hDlg, IDC_CHK_UPDATE), str.autoCheckUpdate.c_str());
                SetWindowTextW(GetDlgItem(hDlg, IDC_CHK_NOTIFY), str.showNotice.c_str());
                SetWindowTextW(GetDlgItem(hDlg, IDC_CHK_DARKMODE), str.darkMode.c_str());
                SetWindowTextW(GetDlgItem(hDlg, IDC_CHK_TRAYUSAGE), str.showUsageInTray.c_str());
                SetWindowTextW(GetDlgItem(hDlg, IDC_CHK_SHORTCUT), str.shortcutKey.c_str());
                SetWindowTextW(GetDlgItem(hDlg, IDC_CHK_AUTOOPT), str.autoOpt.c_str());
                SetWindowTextW(GetDlgItem(hDlg, IDC_RAD_THRESHOLD), str.whenAvailLess.c_str());
                SetWindowTextW(GetDlgItem(hDlg, IDC_CHK_CPUIDLE), str.cpuIdle.c_str());
                SetWindowTextW(GetDlgItem(hDlg, IDC_RAD_INTERVAL), str.optEvery.c_str());
                SetWindowTextW(GetDlgItem(hDlg, IDC_LBL_LANG), str.language.c_str());
                SetWindowTextW(GetDlgItem(hDlg, IDC_BTN_OK), str.btnOK.c_str());

                HWND hCmbInt = GetDlgItem(hDlg, IDC_CMB_INTERVAL);
                int curSel = (int)SendMessageW(hCmbInt, CB_GETCURSEL, 0, 0);
                SendMessageW(hCmbInt, CB_RESETCONTENT, 0, 0);
                int intervals[] = { 5, 10, 15, 30, 45, 60, 120 };
                for (int i = 0; i < 7; ++i) {
                    std::wstring s = std::to_wstring(intervals[i]) + L" " + str.minuteUnit;
                    SendMessageW(hCmbInt, CB_ADDSTRING, 0, (LPARAM)s.c_str());
                }
                SendMessageW(hCmbInt, CB_SETCURSEL, (curSel >= 0 && curSel < 7) ? curSel : 4, 0);
                InvalidateRect(hDlg, NULL, TRUE);
            }
        }

        if (id == IDC_BTN_OK) {
            if (s_pConfig) SaveSettings(hDlg, *s_pConfig);
            DestroyWindow(hDlg);
            return 0;
        } else if (id == IDCANCEL) {
            DestroyWindow(hDlg);
            return 0;
        }
        return 0;
    }

    case WM_CLOSE:
        DestroyWindow(hDlg);
        return 0;

    case WM_DESTROY:
        if (s_hFont) { DeleteObject(s_hFont); s_hFont = NULL; }
        if (s_hFontBold) { DeleteObject(s_hFontBold); s_hFontBold = NULL; }
        if (s_hBgBrush) { DeleteObject(s_hBgBrush); s_hBgBrush = NULL; }
        return 0;
    }

    return DefWindowProcW(hDlg, uMsg, wParam, lParam);
}
