#include "NoticeWindow.h"
#include "Localization.h"
#include <gdiplus.h>
#include <math.h>

using namespace Gdiplus;

#define TIMER_SLIDE_ID   501
#define TIMER_CLOSE_ID   502

static HWND s_hNoticeWnd = NULL;
static std::wstring s_noticeFreedText;
static int s_noticeLoad = 0;
static int s_noticeLang = 0;
static bool s_noticeDarkMode = false;
static bool* s_pNoticeNotShowAgain = nullptr;
static bool s_localNotShow = false;
static int s_noticeCurY = 0;
static int s_noticeTargetY = 0;
static bool s_isSlidingIn = true;
static bool s_isSlidingOut = false;
static bool s_closeHover = false;
static bool s_chkHover = false;
static bool s_noticeTrackingMouse = false;

static const int NOTICE_W = 320;
static const int NOTICE_H = 132;

static LRESULT CALLBACK NoticeWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBM = CreateCompatibleBitmap(hdc, w, h);
        HBITMAP oldBM = (HBITMAP)SelectObject(memDC, memBM);

        {
            Graphics g(memDC);
            g.SetSmoothingMode(SmoothingModeAntiAlias);
            g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

            AppStrings strings = Localization::GetStrings(s_noticeLang);
            bool dark = s_noticeDarkMode;

            // Background
            Color bgCol = dark ? Color(255, 30, 30, 30) : Color(255, 255, 255, 255);
            SolidBrush bgBrush(bgCol);
            g.FillRectangle(&bgBrush, 0, 0, w, h);

            // Outer border
            Color borderCol = dark ? Color(255, 65, 65, 65) : Color(255, 215, 220, 228);
            Pen borderPen(borderCol, 1);
            g.DrawRectangle(&borderPen, 0, 0, w - 1, h - 1);

            FontFamily ff(L"Tahoma");
            Font fontTitle(&ff, 8.5f, FontStyleBold, UnitPoint);
            Font fontRegular(&ff, 8.5f, FontStyleRegular, UnitPoint);
            Font fontSmall(&ff, 8.0f, FontStyleRegular, UnitPoint);

            // 1. Title bar
            SolidBrush greenChip(Color(255, 69, 186, 31));
            g.FillRectangle(&greenChip, 10, 8, 16, 12);
            Pen penPin(Color(255, 200, 200, 200), 1);
            for (int px = 12; px < 24; px += 3) {
                g.DrawLine(&penPin, px, 6, px, 8);
                g.DrawLine(&penPin, px, 20, px, 22);
            }

            Color titleCol = dark ? Color(255, 230, 230, 230) : Color(255, 45, 55, 72);
            SolidBrush titleBrush(titleCol);
            g.DrawString(strings.appTitle.c_str(), -1, &fontTitle, PointF(30.0f, 7.0f), &titleBrush);

            // Close button [✕] at (w - 26, 6, 20, 18)
            int cx = w - 26, cy = 6, cw = 20, ch = 18;
            if (s_closeHover) {
                SolidBrush redHov(Color(255, 232, 17, 35));
                g.FillRectangle(&redHov, cx, cy, cw, ch);
            }
            Pen closePen(s_closeHover ? Color(255, 255, 255, 255) : (dark ? Color(255, 180, 185, 195) : Color(255, 130, 135, 145)), 1.3f);
            int midX = cx + cw / 2, midY = cy + ch / 2;
            g.DrawLine(&closePen, midX - 4, midY - 4, midX + 4, midY + 4);
            g.DrawLine(&closePen, midX + 4, midY - 4, midX - 4, midY + 4);

            // 2. Green Check Circle Icon at (16, 38, 30, 30)
            int icX = 16, icY = 38, icD = 30;
            SolidBrush circleGreen(Color(255, 69, 186, 31));
            g.FillEllipse(&circleGreen, icX, icY, icD, icD);

            // White Checkmark
            Pen checkPen(Color(255, 255, 255, 255), 2.2f);
            checkPen.SetStartCap(LineCapRound);
            checkPen.SetEndCap(LineCapRound);
            g.DrawLine(&checkPen, (REAL)icX + 8, (REAL)icY + 15, (REAL)icX + 13, (REAL)icY + 20);
            g.DrawLine(&checkPen, (REAL)icX + 13, (REAL)icY + 20, (REAL)icX + 22, (REAL)icY + 10);

            // 3. Message texts
            SolidBrush textHead(dark ? Color(255, 245, 245, 245) : Color(255, 20, 20, 20));
            SolidBrush textSub(dark ? Color(255, 209, 213, 219) : Color(255, 75, 85, 99));
            SolidBrush textMuted(dark ? Color(255, 156, 163, 175) : Color(255, 130, 140, 150));

            std::wstring head = strings.noticeHead;
            wchar_t l1Buf[128];
            swprintf_s(l1Buf, strings.noticeLine1.c_str(), s_noticeFreedText.c_str());
            std::wstring line1 = l1Buf;

            wchar_t l2Buf[128];
            swprintf_s(l2Buf, strings.noticeLine2.c_str(), s_noticeLoad);
            std::wstring line2 = l2Buf;

            g.DrawString(head.c_str(), -1, &fontTitle, PointF(56.0f, 35.0f), &textHead);
            g.DrawString(line1.c_str(), -1, &fontRegular, PointF(56.0f, 52.0f), &textSub);
            g.DrawString(line2.c_str(), -1, &fontSmall, PointF(56.0f, 69.0f), &textMuted);

            // 4. Subtle line
            Pen divPen(dark ? Color(255, 55, 65, 81) : Color(255, 240, 242, 246), 1);
            g.DrawLine(&divPen, 14, 94, w - 14, 94);

            // 5. Checkbox: "Do not show this notification again"
            int boxX = 16, boxY = 104, boxS = 13;
            Color chkBorder = s_chkHover ? Color(255, 69, 186, 31) : (dark ? Color(255, 75, 85, 99) : Color(255, 180, 185, 195));
            Pen boxPen(chkBorder, 1.2f);
            g.DrawRectangle(&boxPen, boxX, boxY, boxS, boxS);
            if (s_localNotShow) {
                SolidBrush chkFill(Color(255, 69, 186, 31));
                g.FillRectangle(&chkFill, boxX + 2, boxY + 2, boxS - 3, boxS - 3);
                Pen tickPen(Color(255, 255, 255, 255), 1.6f);
                g.DrawLine(&tickPen, boxX + 3, boxY + 6, boxX + 6, boxY + 9);
                g.DrawLine(&tickPen, boxX + 6, boxY + 9, boxX + 10, boxY + 3);
            }
            g.DrawString(strings.noticeNotAgain.c_str(), -1, &fontSmall, PointF((REAL)boxX + 18, (REAL)boxY - 1), &textSub);
        }

        BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);
        SelectObject(memDC, oldBM);
        DeleteObject(memBM);
        DeleteDC(memDC);

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_TIMER:
        if (wParam == TIMER_SLIDE_ID) {
            if (s_isSlidingIn) {
                int diff = s_noticeTargetY - s_noticeCurY;
                if (abs(diff) > 2) {
                    s_noticeCurY += (int)(diff * 0.35f);
                    RECT rc;
                    GetWindowRect(hWnd, &rc);
                    SetWindowPos(hWnd, NULL, rc.left, s_noticeCurY, NOTICE_W, NOTICE_H, SWP_NOZORDER);
                } else {
                    s_noticeCurY = s_noticeTargetY;
                    s_isSlidingIn = false;
                    KillTimer(hWnd, TIMER_SLIDE_ID);
                    RECT rc;
                    GetWindowRect(hWnd, &rc);
                    SetWindowPos(hWnd, NULL, rc.left, s_noticeCurY, NOTICE_W, NOTICE_H, SWP_NOZORDER);
                    SetTimer(hWnd, TIMER_CLOSE_ID, 4000, NULL);
                }
            } else if (s_isSlidingOut) {
                RECT rcWork;
                SystemParametersInfoW(SPI_GETWORKAREA, 0, &rcWork, 0);
                int offscreenY = rcWork.bottom + 10;
                int diff = offscreenY - s_noticeCurY;
                if (abs(diff) > 2) {
                    s_noticeCurY += (int)(diff * 0.35f);
                    RECT rc;
                    GetWindowRect(hWnd, &rc);
                    SetWindowPos(hWnd, NULL, rc.left, s_noticeCurY, NOTICE_W, NOTICE_H, SWP_NOZORDER);
                } else {
                    KillTimer(hWnd, TIMER_SLIDE_ID);
                    DestroyWindow(hWnd);
                }
            }
        } else if (wParam == TIMER_CLOSE_ID) {
            KillTimer(hWnd, TIMER_CLOSE_ID);
            s_isSlidingOut = true;
            SetTimer(hWnd, TIMER_SLIDE_ID, 16, NULL);
        }
        return 0;

    case WM_MOUSEMOVE: {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        if (!s_noticeTrackingMouse) {
            TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT) };
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hWnd;
            TrackMouseEvent(&tme);
            s_noticeTrackingMouse = true;
            KillTimer(hWnd, TIMER_CLOSE_ID);
        }

        RECT rc;
        GetClientRect(hWnd, &rc);
        int w = rc.right - rc.left;

        bool oldClose = s_closeHover;
        bool oldChk = s_chkHover;

        s_closeHover = (x >= w - 26 && x <= w - 6 && y >= 6 && y <= 24);
        s_chkHover = (x >= 14 && x <= 260 && y >= 100 && y <= 122);

        if (oldClose != s_closeHover || oldChk != s_chkHover) {
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_MOUSELEAVE: {
        s_noticeTrackingMouse = false;
        if (s_closeHover || s_chkHover) {
            s_closeHover = false;
            s_chkHover = false;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        if (!s_isSlidingIn && !s_isSlidingOut) {
            SetTimer(hWnd, TIMER_CLOSE_ID, 3500, NULL);
        }
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        RECT rc;
        GetClientRect(hWnd, &rc);
        int w = rc.right - rc.left;

        if (x >= w - 26 && x <= w - 6 && y >= 6 && y <= 24) {
            if (s_pNoticeNotShowAgain) *s_pNoticeNotShowAgain = s_localNotShow;
            DestroyWindow(hWnd);
            return 0;
        }

        if (x >= 14 && x <= 260 && y >= 100 && y <= 122) {
            s_localNotShow = !s_localNotShow;
            if (s_pNoticeNotShowAgain) *s_pNoticeNotShowAgain = s_localNotShow;
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }
        return 0;
    }

    case WM_DESTROY:
        if (s_pNoticeNotShowAgain) *s_pNoticeNotShowAgain = s_localNotShow;
        s_hNoticeWnd = NULL;
        return 0;
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

void NoticeWindow::Show(HWND hOwner, const std::wstring& freedText, int currentLoad, bool* pNotShowAgain, int language, bool darkMode) {
    if (s_hNoticeWnd && IsWindow(s_hNoticeWnd)) {
        DestroyWindow(s_hNoticeWnd);
        s_hNoticeWnd = NULL;
    }

    s_noticeFreedText = freedText;
    s_noticeLoad = currentLoad;
    s_noticeLang = language;
    s_noticeDarkMode = darkMode;
    s_pNoticeNotShowAgain = pNotShowAgain;
    s_localNotShow = (pNotShowAgain && *pNotShowAgain);

    HINSTANCE hInst = GetModuleHandleW(NULL);
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = NoticeWndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"WiseNoticeClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.style = CS_DROPSHADOW;
    RegisterClassExW(&wc);

    RECT rcWork;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &rcWork, 0);

    int startX = rcWork.right - NOTICE_W - 12;
    s_noticeTargetY = rcWork.bottom - NOTICE_H - 12;
    s_noticeCurY = rcWork.bottom + 10;
    s_isSlidingIn = true;
    s_isSlidingOut = false;
    s_closeHover = false;
    s_chkHover = false;

    s_hNoticeWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"WiseNoticeClass",
        L"Wise Memory Optimizer Notification",
        WS_POPUP,
        startX, s_noticeCurY, NOTICE_W, NOTICE_H,
        NULL, NULL, hInst, NULL
    );

    if (s_hNoticeWnd) {
        ShowWindow(s_hNoticeWnd, SW_SHOWNOACTIVATE);
        UpdateWindow(s_hNoticeWnd);
        SetTimer(s_hNoticeWnd, TIMER_SLIDE_ID, 16, NULL);
    }
}
