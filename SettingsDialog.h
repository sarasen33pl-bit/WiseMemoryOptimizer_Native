#pragma once
#include <windows.h>
#include <commctrl.h>
#include "Config.h"

class SettingsDialog {
public:
    static bool Show(HWND hParent, AppConfig& config);

private:
    static LRESULT CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static void InitControls(HWND hDlg, const AppConfig& config);
    static void SaveSettings(HWND hDlg, AppConfig& config);
    static void UpdateAutoOptStates(HWND hDlg);

    static AppConfig* s_pConfig;
};
