#pragma once
#include <windows.h>
#include <string>

class NoticeWindow {
public:
    static void Show(HWND hOwner, const std::wstring& freedText, int currentLoad, bool* pNotShowAgain, int language = 0, bool darkMode = false);
};
