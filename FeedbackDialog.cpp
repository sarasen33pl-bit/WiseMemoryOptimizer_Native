#include "FeedbackDialog.h"
#include "Localization.h"
#include <commctrl.h>
#include <dwmapi.h>
#include <string>

#pragma comment(lib, "dwmapi.lib")

#define IDC_EDT_EMAIL    4001
#define IDC_EDT_BEST     4002
#define IDC_EDT_BAD      4003
#define IDC_EDT_WANT     4004
#define IDC_BTN_SUBMIT   4005
#define IDC_BTN_CANCEL   4006
#define IDC_STAR_1       4011
#define IDC_STAR_2       4012
#define IDC_STAR_3       4013
#define IDC_STAR_4       4014
#define IDC_STAR_5       4015

static HFONT s_fbFont = NULL;
static HFONT s_fbFontBold = NULL;
static HFONT s_fbFontStars = NULL;
static HBRUSH s_fbBrush = NULL;
static HBRUSH s_fbEditBrush = NULL;
static COLORREF s_fbBg = RGB(255, 255, 255);
static COLORREF s_fbText = RGB(35, 35, 35);
static int s_rating = 5;
static int s_fbLang = 0;
static bool s_fbDark = false;

void FeedbackDialog::Show(HWND hParent, const AppConfig& cfg) {
    s_fbLang = cfg.language;
    s_fbDark = cfg.darkMode;
    AppStrings strings = Localization::GetStrings(s_fbLang);

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = (WNDPROC)DialogProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = L"WiseFeedbackClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassExW(&wc);

    int w = 376;
    int h = 420;
    int scrW = GetSystemMetrics(SM_CXSCREEN);
    int scrH = GetSystemMetrics(SM_CYSCREEN);
    int x = (scrW - w) / 2;
    int y = (scrH - h) / 2;
    if (hParent && IsWindow(hParent)) {
        RECT rcParent;
        GetWindowRect(hParent, &rcParent);
        x = rcParent.left + (rcParent.right - rcParent.left - w) / 2;
        y = rcParent.top + (rcParent.bottom - rcParent.top - h) / 2;
    }

    if (cfg.darkMode) {
        s_fbBg = RGB(35, 35, 35);
        s_fbText = RGB(230, 230, 230);
    } else {
        s_fbBg = RGB(255, 255, 255);
        s_fbText = RGB(35, 35, 35);
    }
    s_fbBrush = CreateSolidBrush(s_fbBg);
    s_fbEditBrush = CreateSolidBrush(cfg.darkMode ? RGB(48, 48, 48) : RGB(255, 255, 255));

    HWND hWnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        L"WiseFeedbackClass",
        strings.feedbackTitle.c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        x, y, w, h,
        hParent, NULL, GetModuleHandleW(NULL), NULL
    );

    if (!hWnd) return;

    BOOL useDark = cfg.darkMode ? TRUE : FALSE;
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
}

LRESULT CALLBACK FeedbackDialog::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hDlg, &rc);
        FillRect(hdc, &rc, s_fbBrush);
        return 1;
    }

    case WM_CREATE: {
        HINSTANCE hInst = GetModuleHandleW(NULL);
        AppStrings strings = Localization::GetStrings(s_fbLang);

        s_fbFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        s_fbFontBold = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        s_fbFontStars = CreateFontW(22, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

        int curX = 24;
        int inputW = 312;

        HWND hLblRate = CreateWindowExW(0, L"STATIC", strings.fbRate.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT, curX, 14, inputW, 18, hDlg, NULL, hInst, NULL);
        SendMessageW(hLblRate, WM_SETFONT, (WPARAM)s_fbFontBold, TRUE);

        // 5 Star Buttons
        for (int i = 0; i < 5; ++i) {
            HWND hStar = CreateWindowExW(0, L"BUTTON", (i < s_rating) ? L"★" : L"☆",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, curX + i * 32, 36, 28, 28,
                hDlg, (HMENU)(UINT_PTR)(IDC_STAR_1 + i), hInst, NULL);
            SendMessageW(hStar, WM_SETFONT, (WPARAM)s_fbFontStars, TRUE);
        }

        HWND hLblEmail = CreateWindowExW(0, L"STATIC", strings.fbEmail.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT, curX, 70, inputW, 18, hDlg, NULL, hInst, NULL);
        SendMessageW(hLblEmail, WM_SETFONT, (WPARAM)s_fbFont, TRUE);

        HWND hEdtEmail = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, curX, 88, inputW, 22, hDlg, (HMENU)IDC_EDT_EMAIL, hInst, NULL);
        SendMessageW(hEdtEmail, WM_SETFONT, (WPARAM)s_fbFont, TRUE);

        HWND hLblBest = CreateWindowExW(0, L"STATIC", strings.fbBest.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT, curX, 116, inputW, 18, hDlg, NULL, hInst, NULL);
        SendMessageW(hLblBest, WM_SETFONT, (WPARAM)s_fbFont, TRUE);

        HWND hEdtBest = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, curX, 134, inputW, 22, hDlg, (HMENU)IDC_EDT_BEST, hInst, NULL);
        SendMessageW(hEdtBest, WM_SETFONT, (WPARAM)s_fbFont, TRUE);

        HWND hLblBad = CreateWindowExW(0, L"STATIC", strings.fbBad.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT, curX, 162, inputW, 18, hDlg, NULL, hInst, NULL);
        SendMessageW(hLblBad, WM_SETFONT, (WPARAM)s_fbFont, TRUE);

        HWND hEdtBad = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, curX, 180, inputW, 22, hDlg, (HMENU)IDC_EDT_BAD, hInst, NULL);
        SendMessageW(hEdtBad, WM_SETFONT, (WPARAM)s_fbFont, TRUE);

        HWND hLblWant = CreateWindowExW(0, L"STATIC", strings.fbWant.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT, curX, 208, inputW, 18, hDlg, NULL, hInst, NULL);
        SendMessageW(hLblWant, WM_SETFONT, (WPARAM)s_fbFont, TRUE);

        HWND hEdtWant = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_MULTILINE | ES_WANTRETURN, curX, 226, inputW, 44, hDlg, (HMENU)IDC_EDT_WANT, hInst, NULL);
        SendMessageW(hEdtWant, WM_SETFONT, (WPARAM)s_fbFont, TRUE);

        HWND hBtnCancel = CreateWindowExW(0, L"BUTTON", strings.fbNotInterested.c_str(),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, curX, 284, 150, 30, hDlg, (HMENU)IDC_BTN_CANCEL, hInst, NULL);
        SendMessageW(hBtnCancel, WM_SETFONT, (WPARAM)s_fbFont, TRUE);

        HWND hBtnSubmit = CreateWindowExW(0, L"BUTTON", strings.fbSubmit.c_str(),
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, curX + 162, 284, 150, 30, hDlg, (HMENU)IDC_BTN_SUBMIT, hInst, NULL);
        SendMessageW(hBtnSubmit, WM_SETFONT, (WPARAM)s_fbFontBold, TRUE);
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hDlg, &ps);
        RECT rc;
        GetClientRect(hDlg, &rc);
        FillRect(hdc, &rc, s_fbBrush);
        EndPaint(hDlg, &ps);
        return 0;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id >= IDC_STAR_1 && id <= IDC_STAR_5) {
            s_rating = id - IDC_STAR_1 + 1;
            for (int i = 0; i < 5; ++i) {
                SetDlgItemTextW(hDlg, IDC_STAR_1 + i, (i < s_rating) ? L"★" : L"☆");
            }
            return TRUE;
        } else if (id == IDC_BTN_SUBMIT) {
            AppStrings strings = Localization::GetStrings(s_fbLang);
            MessageBoxW(hDlg, strings.fbThanks.c_str(), strings.appTitle.c_str(), MB_OK | MB_ICONINFORMATION);
            DestroyWindow(hDlg);
            return TRUE;
        } else if (id == IDC_BTN_CANCEL) {
            DestroyWindow(hDlg);
            return TRUE;
        }
        break;
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, s_fbBg);
        SetTextColor(hdc, s_fbText);
        return (LRESULT)s_fbBrush;
    }

    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, s_fbDark ? RGB(48, 48, 48) : RGB(255, 255, 255));
        SetTextColor(hdc, s_fbDark ? RGB(240, 240, 240) : RGB(30, 30, 30));
        return (LRESULT)s_fbEditBrush;
    }

    case WM_CLOSE:
        DestroyWindow(hDlg);
        return TRUE;

    case WM_DESTROY:
        if (s_fbFont) { DeleteObject(s_fbFont); s_fbFont = NULL; }
        if (s_fbFontBold) { DeleteObject(s_fbFontBold); s_fbFontBold = NULL; }
        if (s_fbFontStars) { DeleteObject(s_fbFontStars); s_fbFontStars = NULL; }
        if (s_fbBrush) { DeleteObject(s_fbBrush); s_fbBrush = NULL; }
        if (s_fbEditBrush) { DeleteObject(s_fbEditBrush); s_fbEditBrush = NULL; }
        return 0;
    }
    return DefWindowProcW(hDlg, uMsg, wParam, lParam);
}
