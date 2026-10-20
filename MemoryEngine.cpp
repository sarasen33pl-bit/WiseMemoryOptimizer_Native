#include "MemoryEngine.h"
#include <stdio.h>

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "advapi32.lib")

// NT API Typedefs
typedef enum _SYSTEM_MEMORY_LIST_COMMAND {
    MemoryCaptureAccessedBits,
    MemoryCaptureAndResetMySQLWorkingSet,
    MemoryEmptyWorkingSets,
    MemoryFlushModifiedList,
    MemoryPurgeStandbyList,
    MemoryPurgeLowPriorityStandbyList,
    MemoryCommandMax
} SYSTEM_MEMORY_LIST_COMMAND;

typedef NTSTATUS (NTAPI *pfnNtSetSystemInformation)(
    ULONG SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength
);

#define SystemMemoryListInformation 80

bool MemoryEngine::s_hasPrivileges = false;
ULONGLONG MemoryEngine::s_prevIdleTime = 0;
ULONGLONG MemoryEngine::s_prevKernelTime = 0;
ULONGLONG MemoryEngine::s_prevUserTime = 0;

static ULONGLONG FileTimeToUInt64(const FILETIME& ft) {
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return uli.QuadPart;
}

bool MemoryEngine::Initialize() {
    bool ok1 = EnablePrivilege(L"SeProfileSingleProcessPrivilege");
    bool ok2 = EnablePrivilege(L"SeIncreaseQuotaPrivilege");
    bool ok3 = EnablePrivilege(L"SeDebugPrivilege");
    s_hasPrivileges = ok1 || ok2 || ok3;

    // Seed CPU measurement
    FILETIME idle, kernel, user;
    if (GetSystemTimes(&idle, &kernel, &user)) {
        s_prevIdleTime = FileTimeToUInt64(idle);
        s_prevKernelTime = FileTimeToUInt64(kernel);
        s_prevUserTime = FileTimeToUInt64(user);
    }
    return true;
}

bool MemoryEngine::EnablePrivilege(LPCWSTR privilegeName) {
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return false;
    }

    LUID luid;
    if (!LookupPrivilegeValueW(NULL, privilegeName, &luid)) {
        CloseHandle(hToken);
        return false;
    }

    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    BOOL ok = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
    DWORD err = GetLastError();
    CloseHandle(hToken);

    return (ok && err == ERROR_SUCCESS);
}

MemoryMetrics MemoryEngine::QueryMetrics() {
    MemoryMetrics m = {};

    MEMORYSTATUSEX ms = { sizeof(ms) };
    if (GlobalMemoryStatusEx(&ms)) {
        m.totalPhysBytes = ms.ullTotalPhys;
        m.availPhysBytes = ms.ullAvailPhys;
        m.usedPhysBytes = (ms.ullTotalPhys > ms.ullAvailPhys) ? (ms.ullTotalPhys - ms.ullAvailPhys) : 0;
        m.loadPercentage = ms.dwMemoryLoad;

        m.totalPageBytes = ms.ullTotalPageFile;
        m.availPageBytes = ms.ullAvailPageFile;
        m.usedPageBytes = (ms.ullTotalPageFile > ms.ullAvailPageFile) ? (ms.ullTotalPageFile - ms.ullAvailPageFile) : 0;
    }

    PERFORMANCE_INFORMATION pi = { sizeof(pi) };
    if (GetPerformanceInfo(&pi, sizeof(pi))) {
        m.pageSize = pi.PageSize;
        m.commitTotalBytes = (ULONGLONG)pi.CommitTotal * pi.PageSize;
        m.commitLimitBytes = (ULONGLONG)pi.CommitLimit * pi.PageSize;
        m.kernelTotalBytes = (ULONGLONG)pi.KernelTotal * pi.PageSize;
        m.kernelPagedBytes = (ULONGLONG)pi.KernelPaged * pi.PageSize;
        m.kernelNonpagedBytes = (ULONGLONG)pi.KernelNonpaged * pi.PageSize;
        m.systemCacheBytes = (ULONGLONG)pi.SystemCache * pi.PageSize;

        // Estimated breakdowns
        ULONGLONG physicalTotal = (ULONGLONG)pi.PhysicalTotal * pi.PageSize;
        ULONGLONG physicalAvail = (ULONGLONG)pi.PhysicalAvailable * pi.PageSize;

        // Standby roughly approximates systemCache when available
        m.standbyEstimateBytes = (m.systemCacheBytes < physicalAvail) ? m.systemCacheBytes : (physicalAvail / 2);
        m.freeEstimateBytes = (physicalAvail > m.standbyEstimateBytes) ? (physicalAvail - m.standbyEstimateBytes) : (physicalAvail / 2);
        m.modifiedEstimateBytes = (m.usedPhysBytes > m.systemCacheBytes) ? (m.usedPhysBytes / 16) : 0;
    }

    return m;
}

bool MemoryEngine::PurgeStandbyListInternal() {
    HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtDll) return false;

    pfnNtSetSystemInformation NtSetSysInfo = (pfnNtSetSystemInformation)GetProcAddress(hNtDll, "NtSetSystemInformation");
    if (!NtSetSysInfo) return false;

    SYSTEM_MEMORY_LIST_COMMAND cmdLow = MemoryPurgeLowPriorityStandbyList;
    NtSetSysInfo(SystemMemoryListInformation, &cmdLow, sizeof(cmdLow));

    SYSTEM_MEMORY_LIST_COMMAND cmd = MemoryPurgeStandbyList;
    NTSTATUS status = NtSetSysInfo(SystemMemoryListInformation, &cmd, sizeof(cmd));
    return (status >= 0);
}

bool MemoryEngine::EmptySystemWorkingSetsInternal() {
    HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtDll) return false;

    pfnNtSetSystemInformation NtSetSysInfo = (pfnNtSetSystemInformation)GetProcAddress(hNtDll, "NtSetSystemInformation");
    if (!NtSetSysInfo) return false;

    SYSTEM_MEMORY_LIST_COMMAND cmd = MemoryEmptyWorkingSets;
    NTSTATUS status = NtSetSysInfo(SystemMemoryListInformation, &cmd, sizeof(cmd));
    return (status >= 0);
}

bool MemoryEngine::FlushModifiedListInternal() {
    HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtDll) return false;

    pfnNtSetSystemInformation NtSetSysInfo = (pfnNtSetSystemInformation)GetProcAddress(hNtDll, "NtSetSystemInformation");
    if (!NtSetSysInfo) return false;

    SYSTEM_MEMORY_LIST_COMMAND cmd = MemoryFlushModifiedList;
    NTSTATUS status = NtSetSysInfo(SystemMemoryListInformation, &cmd, sizeof(cmd));
    return (status >= 0);
}

void MemoryEngine::FlushSystemFileCache() {
    SetSystemFileCacheSize((SIZE_T)-1, (SIZE_T)-1, 0);
}

void MemoryEngine::TrimProcessWorkingSets() {
    DWORD currentPid = GetCurrentProcessId();

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32W pe = { sizeof(pe) };
    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            if (pe.th32ProcessID == 0 || pe.th32ProcessID == 4 || pe.th32ProcessID == currentPid) {
                continue;
            }

            HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_SET_QUOTA, FALSE, pe.th32ProcessID);
            if (!hProcess) {
                hProcess = OpenProcess(PROCESS_SET_QUOTA, FALSE, pe.th32ProcessID);
            }
            if (hProcess) {
                K32EmptyWorkingSet(hProcess);
                CloseHandle(hProcess);
            }
        } while (Process32NextW(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);

    // Also trim own working set
    HANDLE hSelf = GetCurrentProcess();
    K32EmptyWorkingSet(hSelf);
}

void MemoryEngine::ClearClipboardData() {
    if (OpenClipboard(NULL)) {
        EmptyClipboard();
        CloseClipboard();
    }
}

double MemoryEngine::GetCpuUsage() {
    FILETIME idle, kernel, user;
    if (!GetSystemTimes(&idle, &kernel, &user)) return 0.0;

    ULONGLONG curIdle = FileTimeToUInt64(idle);
    ULONGLONG curKernel = FileTimeToUInt64(kernel);
    ULONGLONG curUser = FileTimeToUInt64(user);

    ULONGLONG idleDiff = curIdle - s_prevIdleTime;
    ULONGLONG kernelDiff = curKernel - s_prevKernelTime;
    ULONGLONG userDiff = curUser - s_prevUserTime;

    s_prevIdleTime = curIdle;
    s_prevKernelTime = curKernel;
    s_prevUserTime = curUser;

    ULONGLONG total = kernelDiff + userDiff;
    if (total == 0) return 0.0;

    double cpu = (1.0 - ((double)idleDiff / (double)total)) * 100.0;
    if (cpu < 0.0) cpu = 0.0;
    if (cpu > 100.0) cpu = 100.0;
    return cpu;
}

ULONGLONG MemoryEngine::Optimize(OptMethod method, bool clearClipboard, std::wstring* pOutFreedStr) {
    MemoryMetrics before = QueryMetrics();
    ULONGLONG tStart = GetTickCount64();

    switch (method) {
    case OptMethod::Full:
        FlushModifiedListInternal();
        TrimProcessWorkingSets();
        PurgeStandbyListInternal();
        EmptySystemWorkingSetsInternal();
        FlushSystemFileCache();
        HeapCompact(GetProcessHeap(), 0);
        if (clearClipboard) ClearClipboardData();
        break;

    case OptMethod::TrimOnly:
        TrimProcessWorkingSets();
        EmptySystemWorkingSetsInternal();
        if (clearClipboard) ClearClipboardData();
        break;

    case OptMethod::StandbyOnly:
        FlushModifiedListInternal();
        PurgeStandbyListInternal();
        FlushSystemFileCache();
        break;

    case OptMethod::DefragOnly:
        FlushModifiedListInternal();
        TrimProcessWorkingSets();
        PurgeStandbyListInternal();
        EmptySystemWorkingSetsInternal();
        FlushSystemFileCache();
        HeapCompact(GetProcessHeap(), 0);
        if (clearClipboard) ClearClipboardData();
        break;
    }

    // Ensure fluid animation spins smoothly for user feedback
    ULONGLONG elapsed = GetTickCount64() - tStart;
    if (elapsed < 1100) {
        Sleep((DWORD)(1100 - elapsed));
    }

    MemoryMetrics after = QueryMetrics();
    ULONGLONG freedBytes = 0;
    if (after.availPhysBytes > before.availPhysBytes) {
        freedBytes = after.availPhysBytes - before.availPhysBytes;
    }

    if (pOutFreedStr) {
        wchar_t buf[64];
        if (freedBytes >= 1024ULL * 1024ULL * 1024ULL) {
            double gb = (double)freedBytes / (1024.0 * 1024.0 * 1024.0);
            swprintf_s(buf, L"%.2f GB", gb);
        } else {
            double mb = (double)freedBytes / (1024.0 * 1024.0);
            swprintf_s(buf, L"%.2f MB", mb);
        }
        *pOutFreedStr = buf;
    }

    return freedBytes;
}

ULONGLONG MemoryEngine::OptimizeEx(bool clearMem, bool clearStandby, bool defragMem, bool clearClipboard, std::wstring* pOutFreedStr) {
    MemoryMetrics before = QueryMetrics();
    ULONGLONG tStart = GetTickCount64();

    if (defragMem) {
        FlushModifiedListInternal();
        TrimProcessWorkingSets();
        PurgeStandbyListInternal();
        EmptySystemWorkingSetsInternal();
        FlushSystemFileCache();
        HeapCompact(GetProcessHeap(), 0);
    } else {
        if (clearMem) {
            EmptySystemWorkingSetsInternal();
            TrimProcessWorkingSets();
        }
        if (clearStandby) {
            FlushModifiedListInternal();
            PurgeStandbyListInternal();
            FlushSystemFileCache();
        }
    }

    if (clearClipboard) {
        ClearClipboardData();
    }

    // Ensure fluid animation spins smoothly for user feedback
    ULONGLONG elapsed = GetTickCount64() - tStart;
    if (elapsed < 1100) {
        Sleep((DWORD)(1100 - elapsed));
    }

    MemoryMetrics after = QueryMetrics();
    ULONGLONG freedBytes = 0;
    if (after.availPhysBytes > before.availPhysBytes) {
        freedBytes = after.availPhysBytes - before.availPhysBytes;
    }

    if (pOutFreedStr) {
        wchar_t buf[64];
        if (freedBytes >= 1024ULL * 1024ULL * 1024ULL) {
            double gb = (double)freedBytes / (1024.0 * 1024.0 * 1024.0);
            swprintf_s(buf, L"%.2f GB", gb);
        } else {
            double mb = (double)freedBytes / (1024.0 * 1024.0);
            swprintf_s(buf, L"%.2f MB", mb);
        }
        *pOutFreedStr = buf;
    }

    return freedBytes;
}
