#include "AppShared.h"

HWND g_window = nullptr;
HWND g_taskbar = nullptr;
HWND g_tray = nullptr;
HHOOK g_keyboardHook = nullptr;
HHOOK g_mouseHook = nullptr;
HWINEVENTHOOK g_taskbarEventHook = nullptr;
IServiceProvider* g_shell = nullptr;
IVirtualDesktopManagerInternal* g_desktopManager = nullptr;
IVirtualDesktopManager* g_publicDesktopManager = nullptr;
IApplicationViewCollection* g_applicationViews = nullptr;
IVirtualDesktopNotificationService* g_notificationService = nullptr;
DWORD g_notificationCookie = 0;
DesktopState g_state;
DWORD g_ignoreSwitchEventsUntil = 0;
DWORD g_ignoreKeyboardCaptureUntil = 0;
int g_hoverIndex = -1;
int g_taskbarRefreshBurstsRemaining = 0;
bool g_trackingMouse = false;
bool g_trayIconAdded = false;
HWND g_dragCandidateWindow = nullptr;
POINT g_dragStartPoint{};
POINT g_dragDropPoint{};
int g_dragDropTarget = -1;
DWORD g_suppressPagerClickUntil = 0;
int g_overlayManualOffsetX = 0;
bool g_overlayRightButtonDown = false;
bool g_overlayPositionDragActive = false;
POINT g_overlayPositionDragStart{};
int g_overlayPositionDragStartX = 0;
DWORD g_overlayRightButtonDownTick = 0;

GUID g_pendingDesktopId{};
bool g_hasPendingDesktopSwitch = false;

void Log(const char* format, ...)
{
    FILE* log = nullptr;
    fopen_s(&log, "WorkspacePagerMvp.log", "a");
    if (log == nullptr)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    vfprintf(log, format, args);
    fprintf(log, "\n");
    va_end(args);
    fclose(log);
}

LONG WINAPI UnhandledExceptionHandler(EXCEPTION_POINTERS* exceptionInfo)
{
    if (exceptionInfo != nullptr && exceptionInfo->ExceptionRecord != nullptr)
    {
        Log(
            "Unhandled exception code=0x%08X address=0x%p",
            static_cast<unsigned>(exceptionInfo->ExceptionRecord->ExceptionCode),
            exceptionInfo->ExceptionRecord->ExceptionAddress);
    }
    else
    {
        Log("Unhandled exception without exception record");
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

void ConfigureDpiAwareness()
{
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32 == nullptr)
    {
        return;
    }

    typedef BOOL(WINAPI* SetProcessDpiAwarenessContextProc)(DPI_AWARENESS_CONTEXT);
    SetProcessDpiAwarenessContextProc setProcessDpiAwarenessContext =
        reinterpret_cast<SetProcessDpiAwarenessContextProc>(GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
    if (setProcessDpiAwarenessContext != nullptr)
    {
        BOOL ok = setProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        Log("SetProcessDpiAwarenessContext ok=%d err=%lu", ok ? 1 : 0, GetLastError());
    }
}

void ReleaseUnknown(IUnknown* object)
{
    if (object != nullptr)
    {
        object->Release();
    }
}
