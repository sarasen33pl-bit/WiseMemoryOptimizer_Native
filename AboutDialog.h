#pragma once
#include <windows.h>
#include "Config.h"

class AboutDialog {
public:
    static void Show(HWND hParent, const AppConfig& cfg);

private:
    static LRESULT CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
