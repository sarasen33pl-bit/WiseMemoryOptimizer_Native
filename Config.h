#pragma once
#include <windows.h>
#include <string>

struct AppConfig {
    bool minimizeToTrayOnClose = true;
    bool runAtStartup = true;
    bool clearClipboardOnOpt = true;
    bool clearMem = true;
    bool clearStandby = true;
    bool defragMem = false;
    bool autoCheckUpdate = false;
    bool showNotificationOnOpt = true;
    bool notShowNoticeAgain = false;
    bool darkMode = false;
    bool showUsageInTray = false;
    bool shortcutEnabled = true;
    int  shortcutMod = 0; // 0: CTRL+ALT, 1: ALT+SHIFT, 2: CTRL+SHIFT
    wchar_t shortcutKey = L'A';

    bool autoOptEnabled = true;
    int  autoOptType = 1; // 0: Threshold, 1: Interval
    int  autoOptThreshold = 30; // %
    bool autoOptCpuIdle = false;
    int  autoOptMinutes = 45; // Minutes

    int  language = 0; // 0: English, 1: Arabic
    int  windowLeft = -1;
    int  windowTop = -1;

    static std::wstring GetConfigPath() {
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(NULL, path, MAX_PATH);
        std::wstring s(path);
        size_t pos = s.find_last_of(L"\\/");
        if (pos != std::wstring::npos) {
            s = s.substr(0, pos + 1);
        }
        s += L"config.ini";
        return s;
    }

    void Load() {
        std::wstring ini = GetConfigPath();
        minimizeToTrayOnClose = GetPrivateProfileIntW(L"Settings", L"MinimizeToTray", 1, ini.c_str()) != 0;
        runAtStartup          = GetPrivateProfileIntW(L"Settings", L"RunAtStartup", 1, ini.c_str()) != 0;
        clearClipboardOnOpt   = GetPrivateProfileIntW(L"Settings", L"ClearClipboard", 1, ini.c_str()) != 0;
        clearMem              = GetPrivateProfileIntW(L"Settings", L"ClearMem", 1, ini.c_str()) != 0;
        clearStandby          = GetPrivateProfileIntW(L"Settings", L"ClearStandby", 1, ini.c_str()) != 0;
        defragMem             = GetPrivateProfileIntW(L"Settings", L"DefragMem", 0, ini.c_str()) != 0;
        autoCheckUpdate       = GetPrivateProfileIntW(L"Settings", L"AutoCheckUpdate", 0, ini.c_str()) != 0;
        showNotificationOnOpt = GetPrivateProfileIntW(L"Settings", L"ShowNotification", 1, ini.c_str()) != 0;
        notShowNoticeAgain    = GetPrivateProfileIntW(L"Settings", L"NotShowNoticeAgain", 0, ini.c_str()) != 0;
        darkMode              = GetPrivateProfileIntW(L"Settings", L"DarkMode", 0, ini.c_str()) != 0;
        showUsageInTray       = GetPrivateProfileIntW(L"Settings", L"ShowUsageInTray", 0, ini.c_str()) != 0;
        shortcutEnabled       = GetPrivateProfileIntW(L"Settings", L"ShortcutEnabled", 1, ini.c_str()) != 0;
        shortcutMod           = GetPrivateProfileIntW(L"Settings", L"ShortcutMod", 0, ini.c_str());
        
        wchar_t keyBuf[4] = L"A";
        GetPrivateProfileStringW(L"Settings", L"ShortcutKey", L"A", keyBuf, 4, ini.c_str());
        shortcutKey = keyBuf[0];

        autoOptEnabled        = GetPrivateProfileIntW(L"AutoOpt", L"Enabled", 1, ini.c_str()) != 0;
        autoOptType           = GetPrivateProfileIntW(L"AutoOpt", L"Type", 1, ini.c_str());
        autoOptThreshold      = GetPrivateProfileIntW(L"AutoOpt", L"Threshold", 30, ini.c_str());
        autoOptCpuIdle        = GetPrivateProfileIntW(L"AutoOpt", L"CpuIdle", 0, ini.c_str()) != 0;
        autoOptMinutes        = GetPrivateProfileIntW(L"AutoOpt", L"Minutes", 45, ini.c_str());
        language              = GetPrivateProfileIntW(L"Settings", L"Language", 0, ini.c_str());
        windowLeft            = GetPrivateProfileIntW(L"GUI", L"left", -1, ini.c_str());
        windowTop             = GetPrivateProfileIntW(L"GUI", L"top", -1, ini.c_str());
    }

    void Save() const {
        std::wstring ini = GetConfigPath();
        WritePrivateProfileStringW(L"Settings", L"MinimizeToTray", minimizeToTrayOnClose ? L"1" : L"0", ini.c_str());
        WritePrivateProfileStringW(L"Settings", L"RunAtStartup", runAtStartup ? L"1" : L"0", ini.c_str());
        WritePrivateProfileStringW(L"Settings", L"ClearClipboard", clearClipboardOnOpt ? L"1" : L"0", ini.c_str());
        WritePrivateProfileStringW(L"Settings", L"ClearMem", clearMem ? L"1" : L"0", ini.c_str());
        WritePrivateProfileStringW(L"Settings", L"ClearStandby", clearStandby ? L"1" : L"0", ini.c_str());
        WritePrivateProfileStringW(L"Settings", L"DefragMem", defragMem ? L"1" : L"0", ini.c_str());
        WritePrivateProfileStringW(L"Settings", L"AutoCheckUpdate", autoCheckUpdate ? L"1" : L"0", ini.c_str());
        WritePrivateProfileStringW(L"Settings", L"ShowNotification", showNotificationOnOpt ? L"1" : L"0", ini.c_str());
        WritePrivateProfileStringW(L"Settings", L"NotShowNoticeAgain", notShowNoticeAgain ? L"1" : L"0", ini.c_str());
        WritePrivateProfileStringW(L"Settings", L"DarkMode", darkMode ? L"1" : L"0", ini.c_str());
        WritePrivateProfileStringW(L"Settings", L"ShowUsageInTray", showUsageInTray ? L"1" : L"0", ini.c_str());
        WritePrivateProfileStringW(L"Settings", L"ShortcutEnabled", shortcutEnabled ? L"1" : L"0", ini.c_str());
        WritePrivateProfileStringW(L"Settings", L"ShortcutMod", std::to_wstring(shortcutMod).c_str(), ini.c_str());

        wchar_t keyBuf[2] = { shortcutKey, 0 };
        WritePrivateProfileStringW(L"Settings", L"ShortcutKey", keyBuf, ini.c_str());

        WritePrivateProfileStringW(L"AutoOpt", L"Enabled", autoOptEnabled ? L"1" : L"0", ini.c_str());
        WritePrivateProfileStringW(L"AutoOpt", L"Type", std::to_wstring(autoOptType).c_str(), ini.c_str());
        WritePrivateProfileStringW(L"AutoOpt", L"Threshold", std::to_wstring(autoOptThreshold).c_str(), ini.c_str());
        WritePrivateProfileStringW(L"AutoOpt", L"CpuIdle", autoOptCpuIdle ? L"1" : L"0", ini.c_str());
        WritePrivateProfileStringW(L"AutoOpt", L"Minutes", std::to_wstring(autoOptMinutes).c_str(), ini.c_str());
        WritePrivateProfileStringW(L"Settings", L"Language", std::to_wstring(language).c_str(), ini.c_str());

        if (windowLeft >= 0 && windowTop >= 0) {
            WritePrivateProfileStringW(L"GUI", L"left", std::to_wstring(windowLeft).c_str(), ini.c_str());
            WritePrivateProfileStringW(L"GUI", L"top", std::to_wstring(windowTop).c_str(), ini.c_str());
        }

        ApplyStartup(runAtStartup);
    }

    static void ApplyStartup(bool enable) {
        HKEY hKey;
        LPCWSTR subkey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
        if (RegOpenKeyExW(HKEY_CURRENT_USER, subkey, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            LPCWSTR appName = L"WiseMemoryOptimizerNative";
            if (enable) {
                wchar_t exePath[MAX_PATH];
                GetModuleFileNameW(NULL, exePath, MAX_PATH);
                std::wstring cmd = L"\"" + std::wstring(exePath) + L"\" -tray";
                RegSetValueExW(hKey, appName, 0, REG_SZ, (const BYTE*)cmd.c_str(), (DWORD)((cmd.length() + 1) * sizeof(wchar_t)));
            } else {
                RegDeleteValueW(hKey, appName);
            }
            RegCloseKey(hKey);
        }
    }
};
