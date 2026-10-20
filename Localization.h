#pragma once
#include <windows.h>
#include <string>

struct AppStrings {
    // Main Window
    std::wstring appTitle;
    std::wstring inUse;
    std::wstring available;
    std::wstring freed;
    std::wstring atTime;
    std::wstring optimize;
    std::wstring optimizing;
    std::wstring advanced;
    std::wstring freeUpRAM;
    std::wstring clearStandbyRAM;
    std::wstring defragRAM;
    std::wstring warnNoOpt;

    // Right panel columns
    std::wstring colHardwareReserved;
    std::wstring colInUsed;
    std::wstring colModified;
    std::wstring colStandby;
    std::wstring colFree;
    std::wstring colCached;
    std::wstring colTotal;
    std::wstring colInstalled;

    // Tray Menu
    std::wstring trayOpen;
    std::wstring trayOptNow;
    std::wstring trayAutoOpt;
    std::wstring trayDisable;
    std::wstring trayLowMem;
    std::wstring tray5m;
    std::wstring tray10m;
    std::wstring tray30m;
    std::wstring tray60m;
    std::wstring traySettings;
    std::wstring trayAbout;
    std::wstring trayExit;

    // Settings Dialog
    std::wstring settingsTitle;
    std::wstring minToTray;
    std::wstring runAtStartup;
    std::wstring clearClipboard;
    std::wstring autoCheckUpdate;
    std::wstring showNotice;
    std::wstring darkMode;
    std::wstring showUsageInTray;
    std::wstring shortcutKey;
    std::wstring autoOpt;
    std::wstring whenAvailLess;
    std::wstring cpuIdle;
    std::wstring optEvery;
    std::wstring minuteUnit;
    std::wstring language;
    std::wstring btnOK;

    // Notice Window
    std::wstring noticeHead;
    std::wstring noticeLine1;
    std::wstring noticeLine2;
    std::wstring noticeNotAgain;

    // Feedback Dialog
    std::wstring feedbackTitle;
    std::wstring fbRate;
    std::wstring fbEmail;
    std::wstring fbBest;
    std::wstring fbBad;
    std::wstring fbWant;
    std::wstring fbNotInterested;
    std::wstring fbSubmit;
    std::wstring fbThanks;

    // About Dialog
    std::wstring aboutTitle;
    std::wstring aboutVersion;
    std::wstring aboutEmail;
    std::wstring aboutHome;
    std::wstring aboutPolicy;
    std::wstring aboutUninstall;
    std::wstring aboutEula;
    std::wstring aboutThanks;
    std::wstring aboutCopy;
};

class Localization {
public:
    static AppStrings GetStrings(int langId) {
        AppStrings s;
        if (langId == 1) { // Arabic
            s.appTitle = L"Wise Memory Optimizer";
            s.inUse = L"الذاكرة المستخدمة: ";
            s.available = L"الذاكرة الحرة: ";
            s.freed = L"المحرر: ";
            s.atTime = L" في ";
            s.optimize = L"تحسين الآن";
            s.optimizing = L"جاري التحسين...";
            s.advanced = L"متقدم";
            s.freeUpRAM = L"تفريغ الذاكرة (Free up RAM)";
            s.clearStandbyRAM = L"مسح قائمة الانتظار (Clear Standby RAM)";
            s.defragRAM = L"إلغاء تجزئة الذاكرة (Defrag RAM)";
            s.warnNoOpt = L"يجب عليك اختيار طريقة تحسين واحدة على الأقل.";

            s.colHardwareReserved = L"محجوز\nللأجهزة";
            s.colInUsed = L"مستخدمة";
            s.colModified = L"معدل";
            s.colStandby = L"في الانتظار";
            s.colFree = L"حرة";
            s.colCached = L"مخبأة: ";
            s.colTotal = L"الإجمالي: ";
            s.colInstalled = L"المثبتة: ";

            s.trayOpen = L"إظهار نافذة البرنامج";
            s.trayOptNow = L"تحسين الآن";
            s.trayAutoOpt = L"تحسين تلقائي";
            s.trayDisable = L"تعطيل";
            s.trayLowMem = L"إذا كانت الذاكرة منخفضة";
            s.tray5m = L"كل 5 دقائق";
            s.tray10m = L"كل 10 دقائق";
            s.tray30m = L"كل 30 دقيقة";
            s.tray60m = L"كل 60 دقيقة";
            s.traySettings = L"الإعدادات";
            s.trayAbout = L"حول البرنامج";
            s.trayExit = L"خروج";

            s.settingsTitle = L"الإعدادات - Wise Memory Optimizer";
            s.minToTray = L"تصغير إلى شريط المهام عند الإغلاق";
            s.runAtStartup = L"التشغيل عند بدء تشغيل Windows";
            s.clearClipboard = L"مسح الحافظة أثناء التحسين";
            s.autoCheckUpdate = L"التحقق التلقائي من التحديثات";
            s.showNotice = L"إظهار إشعار بعد التحسين";
            s.darkMode = L"الوضع الداكن";
            s.showUsageInTray = L"إظهار نسبة الاستخدام في أيقونة شريط المهام";
            s.shortcutKey = L"مفتاح الاختصار";
            s.autoOpt = L"التحسين التلقائي";
            s.whenAvailLess = L"عندما تقل الذاكرة المتاحة عن:";
            s.cpuIdle = L"تشغيل عند خمول المعالج (مستحسن)";
            s.optEvery = L"تحسين الذاكرة كل";
            s.minuteUnit = L"دقيقة";
            s.language = L"اللغة:";
            s.btnOK = L"موافق";

            s.noticeHead = L"تم تحسين الذاكرة بنجاح!";
            s.noticeLine1 = L"تم تحرير %s من الذاكرة هذه المرة.";
            s.noticeLine2 = L"استخدام الذاكرة الحالي: %d%%";
            s.noticeNotAgain = L"عدم إظهار هذا الإشعار مرة أخرى";

            s.feedbackTitle = L"الملاحظات والتقييم - Wise Memory Optimizer";
            s.fbRate = L"كيف تقيم Wise Memory Optimizer؟";
            s.fbEmail = L"البريد الإلكتروني:";
            s.fbBest = L"ما هي أفضل ميزة تعجبك؟";
            s.fbBad = L"ما هي الميزة التي لا تعجبك؟";
            s.fbWant = L"ما هي الميزة الجديدة التي ترغب بها؟";
            s.fbNotInterested = L"غير مهتم";
            s.fbSubmit = L"إرسال";
            s.fbThanks = L"شكراً جزيلاً لملاحظاتك القيّمة!";

            s.aboutTitle = L"حول البرنامج - Wise Memory Optimizer";
            s.aboutVersion = L"الإصدار الحالي:  4.2.4.132";
            s.aboutEmail = L"البريد الإلكتروني للدعم:";
            s.aboutHome = L"الموقع الرسمي:";
            s.aboutPolicy = L"سياسة الخصوصية";
            s.aboutUninstall = L"تعليمات إلغاء التثبيت";
            s.aboutEula = L"اتفاقية الترخيص";
            s.aboutThanks = L"شكر خاص للمساهمين في الترجمة:";
            s.aboutCopy = L"حقوق النشر © 2006-2025 WiseCleaner. جميع الحقوق محفوظة.";
        } else { // English
            s.appTitle = L"Wise Memory Optimizer";
            s.inUse = L"In Use: ";
            s.available = L"Available: ";
            s.freed = L"Freed: ";
            s.atTime = L" At ";
            s.optimize = L"Optimize";
            s.optimizing = L"Optimizing...";
            s.advanced = L"Advanced";
            s.freeUpRAM = L"Free up RAM";
            s.clearStandbyRAM = L"Clear Standby RAM";
            s.defragRAM = L"Defrag RAM";
            s.warnNoOpt = L"You need to select at least one optimization method.";

            s.colHardwareReserved = L"Hardware\nReserved";
            s.colInUsed = L"In Used";
            s.colModified = L"Modified";
            s.colStandby = L"Standby";
            s.colFree = L"Free";
            s.colCached = L"Cached: ";
            s.colTotal = L"Total: ";
            s.colInstalled = L"Installed: ";

            s.trayOpen = L"Open";
            s.trayOptNow = L"Optimize Now";
            s.trayAutoOpt = L"Auto Optimize";
            s.trayDisable = L"Disable";
            s.trayLowMem = L"If free memory is low";
            s.tray5m = L"Every 5 minutes";
            s.tray10m = L"Every 10 minutes";
            s.tray30m = L"Every 30 minutes";
            s.tray60m = L"Every 60 minutes";
            s.traySettings = L"Settings";
            s.trayAbout = L"About";
            s.trayExit = L"Exit";

            s.settingsTitle = L"Settings - Wise Memory Optimizer";
            s.minToTray = L"Minimize to system tray when closed";
            s.runAtStartup = L"Run at Windows startup";
            s.clearClipboard = L"Clear clipboard while optimizing";
            s.autoCheckUpdate = L"Auto check for new version";
            s.showNotice = L"Show notification after optimizing";
            s.darkMode = L"Dark Mode";
            s.showUsageInTray = L"Show memory usage in tray icon";
            s.shortcutKey = L"Shortcut key";
            s.autoOpt = L"Auto Optimization";
            s.whenAvailLess = L"When available memory is less than:";
            s.cpuIdle = L"Run when CPU is idle (Recommended)";
            s.optEvery = L"Optimize memory every";
            s.minuteUnit = L"minute(s)";
            s.language = L"Language:";
            s.btnOK = L"OK";

            s.noticeHead = L"Optimization finished!";
            s.noticeLine1 = L"Released %s memory this time.";
            s.noticeLine2 = L"Current memory: %d%%";
            s.noticeNotAgain = L"Do not show this notification again";

            s.feedbackTitle = L"Feedback - Wise Memory Optimizer";
            s.fbRate = L"How would you rate Wise Memory Optimizer?";
            s.fbEmail = L"Email address:";
            s.fbBest = L"Which feature do you like best?";
            s.fbBad = L"Which feature do you dislike the most?";
            s.fbWant = L"What new feature would you like most?";
            s.fbNotInterested = L"Not interested";
            s.fbSubmit = L"Submit";
            s.fbThanks = L"Thanks for your feedback!";

            s.aboutTitle = L"About - Wise Memory Optimizer";
            s.aboutVersion = L"Current Version:  4.2.4.132";
            s.aboutEmail = L"Support Email:";
            s.aboutHome = L"Homepage:";
            s.aboutPolicy = L"Privacy Policy";
            s.aboutUninstall = L"Uninstall Instruction";
            s.aboutEula = L"EULA";
            s.aboutThanks = L"Thanks to the following people for their contributions:";
            s.aboutCopy = L"Copyright © 2006-2025 WiseCleaner. All rights reserved.";
        }
        return s;
    }
};
