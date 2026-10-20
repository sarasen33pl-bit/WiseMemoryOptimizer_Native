#pragma once
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <string>
#include <vector>

// Structure holding comprehensive physical & virtual memory metrics
struct MemoryMetrics {
    ULONGLONG totalPhysBytes;
    ULONGLONG availPhysBytes;
    ULONGLONG usedPhysBytes;
    DWORD     loadPercentage;

    ULONGLONG totalPageBytes;
    ULONGLONG availPageBytes;
    ULONGLONG usedPageBytes;

    // Detailed metrics from GetPerformanceInfo
    ULONGLONG commitTotalBytes;
    ULONGLONG commitLimitBytes;
    ULONGLONG kernelTotalBytes;
    ULONGLONG kernelPagedBytes;
    ULONGLONG kernelNonpagedBytes;
    ULONGLONG systemCacheBytes;
    SIZE_T    pageSize;

    // Derived / estimated counters
    ULONGLONG standbyEstimateBytes;
    ULONGLONG modifiedEstimateBytes;
    ULONGLONG freeEstimateBytes;
};

// Optimization modes
enum class OptMethod {
    Full,          // Trims working sets + Standby list + system cache + clipboard
    TrimOnly,      // Trims process working sets
    StandbyOnly,   // Purges Standby List
    DefragOnly     // Trims working sets + Purges Standby list
};

class MemoryEngine {
public:
    static bool Initialize();
    static MemoryMetrics QueryMetrics();
    static ULONGLONG Optimize(OptMethod method, bool clearClipboard, std::wstring* pOutFreedStr = nullptr);
    static ULONGLONG OptimizeEx(bool clearMem, bool clearStandby, bool defragMem, bool clearClipboard, std::wstring* pOutFreedStr = nullptr);
    static void ClearClipboardData();
    static double GetCpuUsage();

private:
    static bool EnablePrivilege(LPCWSTR privilegeName);
    static bool PurgeStandbyListInternal();
    static bool EmptySystemWorkingSetsInternal();
    static bool FlushModifiedListInternal();
    static void TrimProcessWorkingSets();
    static void FlushSystemFileCache();

    static bool s_hasPrivileges;
    static ULONGLONG s_prevIdleTime;
    static ULONGLONG s_prevKernelTime;
    static ULONGLONG s_prevUserTime;
};
