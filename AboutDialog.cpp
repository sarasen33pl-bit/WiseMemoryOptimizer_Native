#include "AboutDialog.h"
#include "Localization.h"
#include <gdiplus.h>
#include <shellapi.h>
#include <dwmapi.h>
#include <string>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "dwmapi.lib")

using namespace Gdiplus;

#define IDC_BTN_OK          5001
#define IDC_LNK_EMAIL       5002
#define IDC_LNK_HOME        5003
#define IDC_LNK_POLICY      5004
#define IDC_LNK_UNINSTALL   5005
#define IDC_LNK_EULA        5006
#define IDC_LBL_VER         5007

static HFONT s_aboutFont = NULL;
static HFONT s_aboutFontBold = NULL;
static HFONT s_aboutFontGreen = NULL;
static HFONT s_aboutFontLink = NULL;
static HFONT s_aboutFontItalic = NULL;
static HBRUSH s_aboutBgBrush = NULL;
static COLORREF s_aboutBg = RGB(255, 255, 255);
static COLORREF s_aboutText = RGB(35, 35, 35);
static Image* s_pAboutLogo = NULL;
static int s_aboutLang = 0;
static bool s_aboutDark = false;

void AboutDialog::Show(HWND hParent, const AppConfig& cfg) {
    s_aboutLang = cfg.language;
    s_aboutDark = cfg.darkMode;
    AppStrings strings = Localization::GetStrings(s_aboutLang);

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = (WNDPROC)DialogProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = L"WiseAboutClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassExW(&wc);

    int w = 540;
    int h = 380;
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
        s_aboutBg = RGB(35, 35, 35);
        s_aboutText = RGB(225, 225, 225);
    } else {
        s_aboutBg = RGB(255, 255, 255);
        s_aboutText = RGB(35, 35, 35);
    }
    s_aboutBgBrush = CreateSolidBrush(s_aboutBg);

    // Load authentic RAM logo from assets or resource
    if (!s_pAboutLogo) {
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(NULL, exePath, MAX_PATH);
        std::wstring p(exePath);
        size_t pos = p.find_last_of(L"\\/");
        if (pos != std::wstring::npos) p = p.substr(0, pos + 1);
        std::wstring imgPath = p + L"assets\\about_logo.png";
        s_pAboutLogo = Image::FromFile(imgPath.c_str());
        if (!s_pAboutLogo || s_pAboutLogo->GetLastStatus() != Ok) {
            std::wstring fallback = p + L"assets\\ram_icon.png";
            s_pAboutLogo = Image::FromFile(fallback.c_str());
        }
    }

    HWND hWnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        L"WiseAboutClass",
        strings.aboutTitle.c_str(),
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

LRESULT CALLBACK AboutDialog::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_ERASEBKGND:
        return 1;

    case WM_CREATE: {
        HINSTANCE hInst = GetModuleHandleW(NULL);
        AppStrings strings = Localization::GetStrings(s_aboutLang);

        s_aboutFont = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        s_aboutFontBold = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        s_aboutFontGreen = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        s_aboutFontLink = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, TRUE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        s_aboutFontItalic = CreateFontW(12, 0, 0, 0, FW_NORMAL, TRUE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

        int curX = 24;
        int curY = 16;

        // Current Version
        HWND hVerLbl = CreateWindowExW(0, L"STATIC", strings.aboutVersion.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT, curX, curY, 320, 20, hDlg, (HMENU)IDC_LBL_VER, hInst, NULL);
        SendMessageW(hVerLbl, WM_SETFONT, (WPARAM)s_aboutFontGreen, TRUE);
        curY += 24;

        // Support Email
        HWND hEmailLbl = CreateWindowExW(0, L"STATIC", strings.aboutEmail.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT, curX, curY, 130, 18, hDlg, NULL, hInst, NULL);
        SendMessageW(hEmailLbl, WM_SETFONT, (WPARAM)s_aboutFont, TRUE);

        HWND hEmailLink = CreateWindowExW(0, L"STATIC", L"support@wisecleaner.com",
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOTIFY, curX + 134, curY, 200, 18, hDlg, (HMENU)IDC_LNK_EMAIL, hInst, NULL);
        SendMessageW(hEmailLink, WM_SETFONT, (WPARAM)s_aboutFontLink, TRUE);
        curY += 22;

        // Homepage
        HWND hHomeLbl = CreateWindowExW(0, L"STATIC", strings.aboutHome.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT, curX, curY, 130, 18, hDlg, NULL, hInst, NULL);
        SendMessageW(hHomeLbl, WM_SETFONT, (WPARAM)s_aboutFont, TRUE);

        HWND hHomeLink = CreateWindowExW(0, L"STATIC", L"www.wisecleaner.com",
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOTIFY, curX + 134, curY, 200, 18, hDlg, (HMENU)IDC_LNK_HOME, hInst, NULL);
        SendMessageW(hHomeLink, WM_SETFONT, (WPARAM)s_aboutFontLink, TRUE);
        curY += 28;

        // Links: Privacy Policy | Uninstall Instruction | EULA
        HWND hLnkPolicy = CreateWindowExW(0, L"STATIC", strings.aboutPolicy.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOTIFY, curX, curY, 100, 18, hDlg, (HMENU)IDC_LNK_POLICY, hInst, NULL);
        SendMessageW(hLnkPolicy, WM_SETFONT, (WPARAM)s_aboutFontLink, TRUE);

        HWND hLnkUninstall = CreateWindowExW(0, L"STATIC", strings.aboutUninstall.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOTIFY, curX + 115, curY, 140, 18, hDlg, (HMENU)IDC_LNK_UNINSTALL, hInst, NULL);
        SendMessageW(hLnkUninstall, WM_SETFONT, (WPARAM)s_aboutFontLink, TRUE);

        HWND hLnkEula = CreateWindowExW(0, L"STATIC", strings.aboutEula.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOTIFY, curX + 270, curY, 90, 18, hDlg, (HMENU)IDC_LNK_EULA, hInst, NULL);
        SendMessageW(hLnkEula, WM_SETFONT, (WPARAM)s_aboutFontLink, TRUE);
        curY += 28;

        // Thanks to contributors
        HWND hThanksLbl = CreateWindowExW(0, L"STATIC", strings.aboutThanks.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT, curX, curY, 470, 18, hDlg, NULL, hInst, NULL);
        SendMessageW(hThanksLbl, WM_SETFONT, (WPARAM)s_aboutFontBold, TRUE);
        curY += 22;

        const wchar_t* contributors = 
            L"Aleksandr, Awadh, Beem, Boris, Cannie, Chirantan, cienislaw, Damned, Danfong, "
            L"DarkPower202, Emily, Fero Fico, geogeo, Giran, Gorran, Hartmut, htooaung, JaeHyung, "
            L"Joan, JP, juupii, Kalju, Kolbjorn, Luis Neves, MiHLitvin, Misevicius, Mohammad, "
            L"Mohsen, Mondie, Neamtu, Ozer, penaV, Pierre, r3Qui3m, Shady, Sunil, sword, Terry, "
            L"thaiware, Tilt, UGLT, Umar, Vinay, Vugar, Wim, xilolee, zetatalk, ZityiSoft";

        HWND hContrib = CreateWindowExW(0, L"STATIC", contributors,
            WS_CHILD | WS_VISIBLE | SS_LEFT, curX, curY, 476, 75, hDlg, NULL, hInst, NULL);
        SendMessageW(hContrib, WM_SETFONT, (WPARAM)s_aboutFont, TRUE);
        curY += 80;

        // Copyright
        HWND hCopy = CreateWindowExW(0, L"STATIC", strings.aboutCopy.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT, curX, curY + 6, 380, 18, hDlg, NULL, hInst, NULL);
        SendMessageW(hCopy, WM_SETFONT, (WPARAM)s_aboutFontItalic, TRUE);

        // OK Button
        HWND hBtnOk = CreateWindowExW(0, L"BUTTON", strings.btnOK.c_str(),
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 420, curY, 80, 28, hDlg, (HMENU)IDC_BTN_OK, hInst, NULL);
        SendMessageW(hBtnOk, WM_SETFONT, (WPARAM)s_aboutFontBold, TRUE);
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hDlg, &ps);
        RECT rc;
        GetClientRect(hDlg, &rc);
        FillRect(hdc, &rc, s_aboutBgBrush);

        if (s_pAboutLogo && s_pAboutLogo->GetLastStatus() == Ok) {
            Graphics g(hdc);
            g.SetSmoothingMode(SmoothingModeAntiAlias);
            g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
            g.DrawImage(s_pAboutLogo, 390, 16, 110, 80);
        }
        EndPaint(hDlg, &ps);
        return 0;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        HWND hCtrl = (HWND)lParam;
        int id = GetDlgCtrlID(hCtrl);
        SetBkColor(hdc, s_aboutBg);
        if (id == IDC_LNK_EMAIL || id == IDC_LNK_HOME || id == IDC_LNK_POLICY ||
            id == IDC_LNK_UNINSTALL || id == IDC_LNK_EULA) {
            SetTextColor(hdc, s_aboutDark ? RGB(96, 165, 250) : RGB(37, 99, 235));
        } else if (id == IDC_LBL_VER) {
            SetTextColor(hdc, s_aboutDark ? RGB(74, 222, 128) : RGB(22, 163, 74));
        } else {
            SetTextColor(hdc, s_aboutText);
        }
        return (LRESULT)s_aboutBgBrush;
    }

    case WM_SETCURSOR: {
        HWND hCtrl = (HWND)wParam;
        int id = GetDlgCtrlID(hCtrl);
        if (id == IDC_LNK_EMAIL || id == IDC_LNK_HOME || id == IDC_LNK_POLICY ||
            id == IDC_LNK_UNINSTALL || id == IDC_LNK_EULA) {
            SetCursor(LoadCursor(NULL, IDC_HAND));
            return TRUE;
        }
        break;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == IDC_BTN_OK || id == IDCANCEL) {
            DestroyWindow(hDlg);
            return 0;
        } else if (id == IDC_LNK_EMAIL) {
            ShellExecuteW(NULL, L"open", L"mailto:support@wisecleaner.com", NULL, NULL, SW_SHOWNORMAL);
            return 0;
        } else if (id == IDC_LNK_HOME) {
            ShellExecuteW(NULL, L"open", L"https://www.wisecleaner.com", NULL, NULL, SW_SHOWNORMAL);
            return 0;
        } else if (id == IDC_LNK_POLICY) {
            ShellExecuteW(NULL, L"open", L"https://www.wisecleaner.com/privacy-policy.html", NULL, NULL, SW_SHOWNORMAL);
            return 0;
        } else if (id == IDC_LNK_UNINSTALL) {
            ShellExecuteW(NULL, L"open", L"https://www.wisecleaner.com/uninstall.html", NULL, NULL, SW_SHOWNORMAL);
            return 0;
        } else if (id == IDC_LNK_EULA) {
            ShellExecuteW(NULL, L"open", L"https://www.wisecleaner.com/eula.html", NULL, NULL, SW_SHOWNORMAL);
            return 0;
        }
        break;
    }

    case WM_CLOSE:
        DestroyWindow(hDlg);
        return 0;

    case WM_DESTROY:
        if (s_aboutFont) { DeleteObject(s_aboutFont); s_aboutFont = NULL; }
        if (s_aboutFontBold) { DeleteObject(s_aboutFontBold); s_aboutFontBold = NULL; }
        if (s_aboutFontGreen) { DeleteObject(s_aboutFontGreen); s_aboutFontGreen = NULL; }
        if (s_aboutFontLink) { DeleteObject(s_aboutFontLink); s_aboutFontLink = NULL; }
        if (s_aboutFontItalic) { DeleteObject(s_aboutFontItalic); s_aboutFontItalic = NULL; }
        if (s_aboutBgBrush) { DeleteObject(s_aboutBgBrush); s_aboutBgBrush = NULL; }
        return 0;
    }

    return DefWindowProcW(hDlg, uMsg, wParam, lParam);
}
