#pragma once
#include <windows.h>
#include "Config.h"

class FeedbackDialog {
public:
    static void Show(HWND hParent, const AppConfig& config);
private:
    static LRESULT CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
