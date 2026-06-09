#include <windows.h>
#include <windowsx.h>
#include <ole2.h>
#include <shellapi.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <vector>

static const GUID CLSID_ImmersiveShell = {0xC2F03A33, 0x21F5, 0x47FA, {0xB4, 0xBB, 0x15, 0x63, 0x62, 0xA2, 0xF2, 0x39}};
static const GUID CLSID_VirtualDesktopManagerInternal = {0xC5E0CDCA, 0x7B6E, 0x41B2, {0x9F, 0xC4, 0xD9, 0x39, 0x75, 0xCC, 0x46, 0x7B}};
static const GUID CLSID_VirtualDesktopManager = {0xAA509086, 0x5CA9, 0x4C25, {0x8F, 0x95, 0x58, 0x9D, 0x3C, 0x07, 0xB4, 0x8A}};
static const GUID CLSID_VirtualNotificationService = {0xA501FDEC, 0x4A09, 0x464C, {0xAE, 0x4E, 0x1B, 0x9C, 0x21, 0xB8, 0x49, 0x18}};
static const GUID IID_IVirtualDesktopManagerInternal = {0x53F5CA0B, 0x158F, 0x4124, {0x90, 0x0C, 0x05, 0x71, 0x58, 0x06, 0x0B, 0x27}};
static const GUID IID_IVirtualDesktopManager = {0xA5CD92FF, 0x29BE, 0x454C, {0x8D, 0x04, 0xD8, 0x28, 0x79, 0xFB, 0x3F, 0x1B}};
static const GUID IID_IVirtualDesktopNotificationService = {0x0CD45E71, 0xD927, 0x4F15, {0x8B, 0x0A, 0x8F, 0xEF, 0x52, 0x53, 0x37, 0xBF}};
static const GUID IID_IVirtualDesktopNotification = {0xB9E5E94D, 0x233E, 0x49AB, {0xAF, 0x5C, 0x2B, 0x45, 0x41, 0xC3, 0xAA, 0xDE}};
static const GUID IID_IVirtualDesktop = {0x3F07F4BE, 0xB107, 0x441A, {0xAF, 0x0F, 0x39, 0xD8, 0x25, 0x29, 0x07, 0x2C}};
static const GUID IID_IApplicationViewCollection = {0x1841C6D7, 0x4F9D, 0x42C0, {0xAF, 0x41, 0x87, 0x47, 0x53, 0x8F, 0x10, 0xE5}};

static const wchar_t* WindowClassName = L"WorkspacePagerMvpWindow";
static const UINT WM_VD_CHANGED = WM_APP + 1;
static const UINT WM_TRAYICON = WM_APP + 2;
static const UINT WM_TASKBAR_REFRESH = WM_APP + 3;
static const UINT WM_DRAGGED_WINDOW_DROPPED = WM_APP + 4;
static const WPARAM VdChangedRefresh = 0;
static const WPARAM VdChangedSwitch = 1;
static const UINT_PTR RepositionTimerId = 1;
static const UINT_PTR DesktopRefreshTimerId = 2;
static const UINT_PTR ForegroundRefreshTimerId = 3;
static const UINT_PTR SwitchRefreshTimerId = 4;
static const UINT_PTR TaskbarRefreshTimerId = 5;
static const UINT DesktopStructureRefreshDelayMs = 1500;
static const UINT TaskbarRefreshDelayMs = 25;
static const UINT TaskbarRefreshBurstDelayMs = 16;
static const UINT TrayIconId = 1;
static const UINT TrayMenuSettingsCommand = 1001;
static const UINT TrayMenuExitCommand = 1002;

struct IVirtualDesktop : IUnknown
{
    virtual BOOL STDMETHODCALLTYPE IsViewVisible(IUnknown* view) = 0;
    virtual GUID STDMETHODCALLTYPE GetId() = 0;
    virtual void* STDMETHODCALLTYPE GetName() = 0;
    virtual void* STDMETHODCALLTYPE GetWallpaperPath() = 0;
    virtual BOOL STDMETHODCALLTYPE IsRemote() = 0;
};

struct IObjectArrayProbe : IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetCount(UINT* count) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAt(UINT index, REFIID iid, void** object) = 0;
};

struct IVirtualDesktopManagerInternal : IUnknown
{
    virtual int STDMETHODCALLTYPE GetCount() = 0;
    virtual HRESULT STDMETHODCALLTYPE MoveViewToDesktop(IUnknown* view, IVirtualDesktop* desktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE CanViewMoveDesktops(IUnknown* view, BOOL* canMove) = 0;
    virtual IVirtualDesktop* STDMETHODCALLTYPE GetCurrentDesktop() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDesktops(IObjectArrayProbe** desktops) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAdjacentDesktop(IVirtualDesktop* from, int direction, IVirtualDesktop** desktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE SwitchDesktop(IVirtualDesktop* desktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE SwitchDesktopAndMoveForegroundView(IVirtualDesktop* desktop) = 0;
    virtual IVirtualDesktop* STDMETHODCALLTYPE CreateDesktop() = 0;
    virtual HRESULT STDMETHODCALLTYPE MoveDesktop(IVirtualDesktop* desktop, int index) = 0;
    virtual HRESULT STDMETHODCALLTYPE RemoveDesktop(IVirtualDesktop* desktop, IVirtualDesktop* fallback) = 0;
    virtual IVirtualDesktop* STDMETHODCALLTYPE FindDesktop(GUID* desktopId) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDesktopSwitchIncludeExcludeViews(IVirtualDesktop* desktop, IObjectArrayProbe** unknown1, IObjectArrayProbe** unknown2) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDesktopName(IVirtualDesktop* desktop, void* name) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDesktopWallpaper(IVirtualDesktop* desktop, void* path) = 0;
    virtual HRESULT STDMETHODCALLTYPE UpdateWallpaperPathForAllDesktops(void* path) = 0;
    virtual HRESULT STDMETHODCALLTYPE CopyDesktopState(IUnknown* view0, IUnknown* view1) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateRemoteDesktop(void* path, IVirtualDesktop** desktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE SwitchRemoteDesktop(IVirtualDesktop* desktop, void* switchType) = 0;
    virtual HRESULT STDMETHODCALLTYPE SwitchDesktopWithAnimation(IVirtualDesktop* desktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetLastActiveDesktop(IVirtualDesktop** desktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE WaitForAnimationToComplete() = 0;
};

struct IVirtualDesktopManager : IUnknown
{
    virtual BOOL STDMETHODCALLTYPE IsWindowOnCurrentVirtualDesktop(HWND topLevelWindow) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetWindowDesktopId(HWND topLevelWindow, GUID* desktopId) = 0;
    virtual HRESULT STDMETHODCALLTYPE MoveWindowToDesktop(HWND topLevelWindow, REFGUID desktopId) = 0;
};

struct IApplicationViewCollection : IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetViews(IObjectArrayProbe** views) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetViewsByZOrder(IObjectArrayProbe** views) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetViewsByAppUserModelId(const wchar_t* appUserModelId, IObjectArrayProbe** views) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetViewForHwnd(HWND window, IUnknown** view) = 0;
};

struct IVirtualDesktopNotification : IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE VirtualDesktopCreated(IVirtualDesktop* desktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE VirtualDesktopDestroyBegin(IVirtualDesktop* desktopDestroyed, IVirtualDesktop* desktopFallback) = 0;
    virtual HRESULT STDMETHODCALLTYPE VirtualDesktopDestroyFailed(IVirtualDesktop* desktopDestroyed, IVirtualDesktop* desktopFallback) = 0;
    virtual HRESULT STDMETHODCALLTYPE VirtualDesktopDestroyed(IVirtualDesktop* desktopDestroyed, IVirtualDesktop* desktopFallback) = 0;
    virtual HRESULT STDMETHODCALLTYPE VirtualDesktopMoved(IVirtualDesktop* desktop, INT64 oldIndex, INT64 newIndex) = 0;
    virtual HRESULT STDMETHODCALLTYPE VirtualDesktopNameChanged(IVirtualDesktop* desktop, void* name) = 0;
    virtual HRESULT STDMETHODCALLTYPE ViewVirtualDesktopChanged(IUnknown* view) = 0;
    virtual HRESULT STDMETHODCALLTYPE CurrentVirtualDesktopChanged(IVirtualDesktop* desktopOld, IVirtualDesktop* desktopNew) = 0;
    virtual HRESULT STDMETHODCALLTYPE VirtualDesktopWallpaperChanged(IVirtualDesktop* desktop, void* path) = 0;
    virtual HRESULT STDMETHODCALLTYPE VirtualDesktopSwitched(IVirtualDesktop* desktop, int type) = 0;
    virtual HRESULT STDMETHODCALLTYPE RemoteVirtualDesktopConnected(IVirtualDesktop* desktop) = 0;
};

struct IVirtualDesktopNotificationService : IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE Register(IVirtualDesktopNotification* notification, DWORD* cookie) = 0;
    virtual HRESULT STDMETHODCALLTYPE Unregister(DWORD cookie) = 0;
};

struct DesktopState
{
    std::vector<GUID> ids;
    std::vector<HWND> foregroundWindows;
    std::vector<HBITMAP> thumbnails;
    int currentIndex = 0;
};

static HWND g_window = nullptr;
static HWND g_taskbar = nullptr;
static HWND g_tray = nullptr;
static HHOOK g_keyboardHook = nullptr;
static HHOOK g_mouseHook = nullptr;
static HWINEVENTHOOK g_taskbarEventHook = nullptr;
static IServiceProvider* g_shell = nullptr;
static IVirtualDesktopManagerInternal* g_desktopManager = nullptr;
static IVirtualDesktopManager* g_publicDesktopManager = nullptr;
static IApplicationViewCollection* g_applicationViews = nullptr;
static IVirtualDesktopNotificationService* g_notificationService = nullptr;
static DWORD g_notificationCookie = 0;
static DesktopState g_state;
static DWORD g_ignoreSwitchEventsUntil = 0;
static DWORD g_ignoreKeyboardCaptureUntil = 0;
static int g_hoverIndex = -1;
static int g_taskbarRefreshBurstsRemaining = 0;
static bool g_trackingMouse = false;
static bool g_trayIconAdded = false;
static HWND g_dragCandidateWindow = nullptr;
static POINT g_dragStartPoint{};
static POINT g_dragDropPoint{};
static int g_dragDropTarget = -1;
static DWORD g_suppressPagerClickUntil = 0;

GUID g_pendingDesktopId{};
bool g_hasPendingDesktopSwitch = false;
static void Log(const char* format, ...)
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

static LONG WINAPI UnhandledExceptionHandler(EXCEPTION_POINTERS* exceptionInfo)
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

static void ConfigureDpiAwareness()
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

static void ReleaseUnknown(IUnknown* object)
{
    if (object != nullptr)
    {
        object->Release();
    }
}

static int DpiScale(HWND window, int value);
static int HoverPreviewContentWidth(HWND window);
static int HoverPreviewContentHeight(HWND window);
static RECT BlockRect(HWND window, int index);
static RECT HoverPreviewRect(HWND window, int index);
static void RenderPager(HWND window);

static IVirtualDesktop* GetDesktopAt(UINT index)
{
    if (g_desktopManager == nullptr)
    {
        return nullptr;
    }

    IObjectArrayProbe* desktops = nullptr;
    HRESULT hr = g_desktopManager->GetDesktops(&desktops);
    if (FAILED(hr) || desktops == nullptr)
    {
        return nullptr;
    }

    IVirtualDesktop* desktop = nullptr;
    hr = desktops->GetAt(index, IID_IVirtualDesktop, reinterpret_cast<void**>(&desktop));
    desktops->Release();
    return SUCCEEDED(hr) ? desktop : nullptr;
}

static int FindDesktopIndexById(const GUID& id)
{
    for (size_t i = 0; i < g_state.ids.size(); ++i)
    {
        if (IsEqualGUID(g_state.ids[i], id))
        {
            return static_cast<int>(i);
        }
    }

    return -1;
}

static int FindDesktopIndexIn(const std::vector<GUID>& ids, const GUID& id)
{
    for (size_t i = 0; i < ids.size(); ++i)
    {
        if (IsEqualGUID(ids[i], id))
        {
            return static_cast<int>(i);
        }
    }

    return -1;
}

static int GetCurrentDesktopIndexFromRegistry()
{
    BYTE data[sizeof(GUID)]{};
    DWORD dataSize = sizeof(data);
    LSTATUS status = RegGetValueW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VirtualDesktops",
        L"CurrentVirtualDesktop",
        RRF_RT_REG_BINARY,
        nullptr,
        data,
        &dataSize);
    if (status != ERROR_SUCCESS || dataSize < sizeof(GUID))
    {
        Log("Read CurrentVirtualDesktop failed status=%ld size=%lu", status, dataSize);
        return -1;
    }

    GUID currentId{};
    memcpy(&currentId, data, sizeof(GUID));
    return FindDesktopIndexById(currentId);
}

static void ClearThumbnails(std::vector<HBITMAP>& thumbnails)
{
    for (HBITMAP thumbnail : thumbnails)
    {
        if (thumbnail != nullptr)
        {
            DeleteObject(thumbnail);
        }
    }
    thumbnails.clear();
}

static void ClearThumbnailForDesktop(int index)
{
    if (index < 0 || static_cast<size_t>(index) >= g_state.thumbnails.size())
    {
        return;
    }

    if (g_state.thumbnails[index] != nullptr)
    {
        DeleteObject(g_state.thumbnails[index]);
        g_state.thumbnails[index] = nullptr;
    }
}

static void ClearCurrentDesktopThumbnail()
{
    ClearThumbnailForDesktop(g_state.currentIndex);
}

static bool SyncCurrentDesktopIndex(const char* reason)
{
    if (GetTickCount() < g_ignoreSwitchEventsUntil)
    {
        ClearCurrentDesktopThumbnail();
        return false;
    }

    int currentIndex = GetCurrentDesktopIndexFromRegistry();
    if (currentIndex < 0 || static_cast<size_t>(currentIndex) >= g_state.ids.size())
    {
        ClearCurrentDesktopThumbnail();
        return false;
    }

    if (currentIndex != g_state.currentIndex)
    {
        Log("Current desktop sync %s old=%d new=%d", reason, g_state.currentIndex, currentIndex);
        g_state.currentIndex = currentIndex;
    }
    ClearCurrentDesktopThumbnail();
    return true;
}

static bool IsAppWindow(HWND window)
{
    if (window == nullptr || window == g_window || !IsWindowVisible(window))
    {
        return false;
    }

    if (GetAncestor(window, GA_ROOT) != window)
    {
        return false;
    }

    LONG_PTR exStyle = GetWindowLongPtrW(window, GWL_EXSTYLE);
    if ((exStyle & WS_EX_TOOLWINDOW) != 0)
    {
        return false;
    }

    wchar_t className[128]{};
    GetClassNameW(window, className, 128);
    if (wcscmp(className, L"Shell_TrayWnd") == 0 ||
        wcscmp(className, L"Shell_SecondaryTrayWnd") == 0 ||
        wcscmp(className, L"Progman") == 0 ||
        wcscmp(className, L"WorkerW") == 0)
    {
        return false;
    }

    return true;
}

static void AssignForegroundWindowForDesktop(HWND window)
{
    if (!IsAppWindow(window) || g_publicDesktopManager == nullptr)
    {
        return;
    }

    GUID desktopId{};
    if (FAILED(g_publicDesktopManager->GetWindowDesktopId(window, &desktopId)))
    {
        return;
    }

    int index = FindDesktopIndexById(desktopId);
    if (index >= 0 && static_cast<size_t>(index) < g_state.foregroundWindows.size() && g_state.foregroundWindows[index] == nullptr)
    {
        g_state.foregroundWindows[index] = window;
    }
}

static BOOL CALLBACK FindForegroundWindowForDesktop(HWND window, LPARAM)
{
    AssignForegroundWindowForDesktop(window);
    return TRUE;
}

static void RefreshDesktopState()
{
    DesktopState next;

    GUID oldCurrentId{};
    if (g_state.currentIndex >= 0 && static_cast<size_t>(g_state.currentIndex) < g_state.ids.size())
    {
        oldCurrentId = g_state.ids[g_state.currentIndex];
    }

    if (g_desktopManager != nullptr)
    {
        IObjectArrayProbe* desktops = nullptr;
        if (SUCCEEDED(g_desktopManager->GetDesktops(&desktops)) && desktops != nullptr)
        {
            UINT count = 0;
            if (SUCCEEDED(desktops->GetCount(&count)))
            {
                next.ids.reserve(count);
                for (UINT i = 0; i < count; ++i)
                {
                    IVirtualDesktop* desktop = nullptr;
                    if (SUCCEEDED(desktops->GetAt(i, IID_IVirtualDesktop, reinterpret_cast<void**>(&desktop))) && desktop != nullptr)
                    {
                        next.ids.push_back(desktop->GetId());
                        desktop->Release();
                    }
                }
            }
            desktops->Release();
        }

        if (!next.ids.empty())
        {
            int newIndex = FindDesktopIndexIn(next.ids, oldCurrentId);
            if (newIndex >= 0)
            {
                next.currentIndex = newIndex;
            }
            else
            {
                next.currentIndex = min(g_state.currentIndex, static_cast<int>(next.ids.size()) - 1);
            }
        }
    }

    if (next.ids.empty())
    {
        next.ids.push_back(GUID{});
        next.currentIndex = 0;
    }

    next.foregroundWindows.assign(next.ids.size(), nullptr);
    next.thumbnails.assign(next.ids.size(), nullptr);
    for (size_t i = 0; i < next.ids.size(); ++i)
    {
        int oldIndex = FindDesktopIndexIn(g_state.ids, next.ids[i]);
        if (oldIndex >= 0 && static_cast<size_t>(oldIndex) < g_state.thumbnails.size())
        {
            next.thumbnails[i] = g_state.thumbnails[oldIndex];
            g_state.thumbnails[oldIndex] = nullptr;
        }
    }
    ClearThumbnails(g_state.thumbnails);
    g_state = next;
    SyncCurrentDesktopIndex("desktop refresh");

    HWND foreground = GetForegroundWindow();
    AssignForegroundWindowForDesktop(foreground);
    EnumWindows(FindForegroundWindowForDesktop, 0);
}

static void RefreshCurrentForegroundWindow()
{
    SyncCurrentDesktopIndex("foreground refresh");
    if (g_state.currentIndex < 0 || static_cast<size_t>(g_state.currentIndex) >= g_state.foregroundWindows.size())
    {
        return;
    }

    HWND foreground = GetForegroundWindow();
    if (IsAppWindow(foreground))
    {
        g_state.foregroundWindows[g_state.currentIndex] = foreground;
    }
}

static void ScheduleForegroundRefresh(HWND window, UINT delayMs = 250)
{
    if (window != nullptr)
    {
        SetTimer(window, ForegroundRefreshTimerId, delayMs, nullptr);
    }
}

static void CaptureThumbnailForDesktop(int index, bool allowCurrentDesktop = false, const char* reason = "unknown")
{
    if (index < 0 || static_cast<size_t>(index) >= g_state.thumbnails.size() || g_window == nullptr)
    {
        return;
    }
    if (index == g_state.currentIndex && !allowCurrentDesktop)
    {
        return;
    }

    int thumbScale = 2;
    int thumbWidth = max(1, HoverPreviewContentWidth(g_window) * thumbScale);
    int thumbHeight = max(1, HoverPreviewContentHeight(g_window) * thumbScale);

    POINT primaryPoint{0, 0};
    HMONITOR monitor = MonitorFromPoint(primaryPoint, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO monitorInfo{};
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (!GetMonitorInfoW(monitor, &monitorInfo))
    {
        return;
    }

    RECT source = monitorInfo.rcWork;
    int sourceWidth = source.right - source.left;
    int sourceHeight = source.bottom - source.top;
    if (sourceWidth <= 0 || sourceHeight <= 0)
    {
        return;
    }

    HDC screenDc = GetDC(nullptr);
    HDC memoryDc = CreateCompatibleDC(screenDc);
    HBITMAP bitmap = CreateCompatibleBitmap(screenDc, thumbWidth, thumbHeight);
    if (bitmap == nullptr)
    {
        DeleteDC(memoryDc);
        ReleaseDC(nullptr, screenDc);
        return;
    }

    HGDIOBJ oldBitmap = SelectObject(memoryDc, bitmap);
    SetStretchBltMode(memoryDc, HALFTONE);
    SetBrushOrgEx(memoryDc, 0, 0, nullptr);
    BOOL ok = StretchBlt(
        memoryDc,
        0,
        0,
        thumbWidth,
        thumbHeight,
        screenDc,
        source.left,
        source.top,
        sourceWidth,
        sourceHeight,
        SRCCOPY | CAPTUREBLT);
    SelectObject(memoryDc, oldBitmap);
    DeleteDC(memoryDc);
    ReleaseDC(nullptr, screenDc);

    if (!ok)
    {
        DeleteObject(bitmap);
        Log("Capture thumbnail failed index=%d err=%lu", index, GetLastError());
        return;
    }

    bool replacedExisting = g_state.thumbnails[index] != nullptr;
    if (replacedExisting)
    {
        DeleteObject(g_state.thumbnails[index]);
    }
    g_state.thumbnails[index] = bitmap;
    Log(
        "Thumbnail updated reason=%s target=%d current=%d replaced=%d foreground=0x%p size=%dx%d",
        reason,
        index,
        g_state.currentIndex,
        replacedExisting ? 1 : 0,
        GetForegroundWindow(),
        thumbWidth,
        thumbHeight);
}

static LRESULT CALLBACK LowLevelKeyboardProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) && GetTickCount() >= g_ignoreKeyboardCaptureUntil)
    {
        KBDLLHOOKSTRUCT* keyboard = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        bool arrow = keyboard != nullptr && (keyboard->vkCode == VK_LEFT || keyboard->vkCode == VK_RIGHT);
        bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0 ||
                    (GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0 ||
                    (GetAsyncKeyState(VK_RCONTROL) & 0x8000) != 0;
        bool win = (GetAsyncKeyState(VK_LWIN) & 0x8000) != 0 ||
                   (GetAsyncKeyState(VK_RWIN) & 0x8000) != 0;

        static DWORD lastCaptureTick = 0;
        DWORD now = GetTickCount();
        int targetIndex = -1;
        if (keyboard != nullptr && keyboard->vkCode == VK_LEFT)
        {
            targetIndex = g_state.currentIndex - 1;
        }
        else if (keyboard != nullptr && keyboard->vkCode == VK_RIGHT)
        {
            targetIndex = g_state.currentIndex + 1;
        }

        if (arrow &&
            ctrl &&
            win &&
            targetIndex >= 0 &&
            targetIndex < static_cast<int>(g_state.ids.size()) &&
            now - lastCaptureTick > 150)
        {
            lastCaptureTick = now;
            Log("Keyboard pre-switch capture current=%d", g_state.currentIndex);
            CaptureThumbnailForDesktop(g_state.currentIndex, true, "keyboard-pre-switch");
        }
    }

    return CallNextHookEx(g_keyboardHook, code, wParam, lParam);
}

static int DpiScale(HWND window, int value)
{
    UINT dpi = 96;
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    typedef UINT(WINAPI* GetDpiForWindowProc)(HWND);
    GetDpiForWindowProc getDpiForWindow = reinterpret_cast<GetDpiForWindowProc>(GetProcAddress(user32, "GetDpiForWindow"));
    if (getDpiForWindow != nullptr)
    {
        dpi = getDpiForWindow(window);
    }

    return MulDiv(value, static_cast<int>(dpi), 96);
}

static int PagerWidth(HWND window)
{
    int count = static_cast<int>(g_state.ids.size());
    int block = DpiScale(window, 54);
    int gap = DpiScale(window, 5);
    int padding = DpiScale(window, 6);
    return padding * 2 + count * block + max(0, count - 1) * gap;
}

static int HoverPreviewWidth(HWND window)
{
    return DpiScale(window, 220);
}

static int HoverPreviewHeight(HWND window)
{
    return DpiScale(window, 126);
}

static int HoverPreviewContentWidth(HWND window)
{
    return HoverPreviewWidth(window) - DpiScale(window, 12);
}

static int HoverPreviewContentHeight(HWND window)
{
    return HoverPreviewHeight(window) - DpiScale(window, 12);
}

static int OverlayPreviewBandHeight(HWND window)
{
    return HoverPreviewHeight(window) + DpiScale(window, 14);
}

static int OverlayWidth(HWND window)
{
    return max(PagerWidth(window), HoverPreviewWidth(window) + DpiScale(window, 12));
}

static int IndicatorBandHeight(HWND window)
{
    return DpiScale(window, 40);
}

static int OverlayHeight(HWND window, int taskbarHeight)
{
    int indicatorBand = min(taskbarHeight, IndicatorBandHeight(window));
    return indicatorBand + OverlayPreviewBandHeight(window);
}

static RECT BlockRect(HWND window, int index)
{
    RECT client{};
    GetClientRect(window, &client);

    int block = DpiScale(window, 54);
    int gap = DpiScale(window, 5);
    int totalWidth = PagerWidth(window);
    int startX = (client.right - client.left - totalWidth) / 2 + DpiScale(window, 6);
    int blockHeight = DpiScale(window, 34);
    int indicatorBand = min(client.bottom - client.top, IndicatorBandHeight(window));
    int bandTop = client.bottom - indicatorBand;
    int y = bandTop + (indicatorBand - blockHeight) / 2;

    RECT rect{};
    rect.left = startX + index * (block + gap);
    rect.top = y;
    rect.right = rect.left + block;
    rect.bottom = rect.top + blockHeight;
    return rect;
}

static RECT HoverPreviewRect(HWND window, int index)
{
    RECT client{};
    GetClientRect(window, &client);

    RECT block = BlockRect(window, index);
    int previewWidth = HoverPreviewWidth(window);
    int previewHeight = HoverPreviewHeight(window);
    int margin = DpiScale(window, 6);
    int gap = DpiScale(window, 8);
    int centerX = block.left + (block.right - block.left) / 2;

    RECT rect{};
    rect.left = centerX - previewWidth / 2;
    rect.right = rect.left + previewWidth;
    if (rect.left < client.left + margin)
    {
        rect.left = client.left + margin;
        rect.right = rect.left + previewWidth;
    }
    if (rect.right > client.right - margin)
    {
        rect.right = client.right - margin;
        rect.left = rect.right - previewWidth;
    }

    rect.top = block.top - gap - previewHeight;
    rect.bottom = rect.top + previewHeight;
    if (rect.top < client.top + margin)
    {
        rect.top = client.top + margin;
        rect.bottom = rect.top + previewHeight;
    }
    return rect;
}

static int HitTestDesktopIndex(HWND window, POINT point)
{
    for (int i = 0; i < static_cast<int>(g_state.ids.size()); ++i)
    {
        RECT rect = BlockRect(window, i);
        if (PtInRect(&rect, point))
        {
            return i;
        }
    }

    return -1;
}

static int HitTestDesktopIndexFromScreen(HWND window, POINT point)
{
    if (window == nullptr || !IsWindow(window))
    {
        return -1;
    }

    POINT clientPoint = point;
    if (!ScreenToClient(window, &clientPoint))
    {
        return -1;
    }

    return HitTestDesktopIndex(window, clientPoint);
}

static int HitTestDesktopDropIndex(HWND window, POINT point)
{
    int index = HitTestDesktopIndex(window, point);
    if (index >= 0)
    {
        return index;
    }

    for (int i = 0; i < static_cast<int>(g_state.ids.size()); ++i)
    {
        RECT block = BlockRect(window, i);
        InflateRect(&block, DpiScale(window, 8), DpiScale(window, 10));
        if (PtInRect(&block, point))
        {
            return i;
        }

        RECT preview = HoverPreviewRect(window, i);
        if (PtInRect(&preview, point))
        {
            return i;
        }
    }

    return -1;
}

static RECT ClientRectToScreenRect(HWND window, RECT rect)
{
    POINT topLeft{rect.left, rect.top};
    POINT bottomRight{rect.right, rect.bottom};
    ClientToScreen(window, &topLeft);
    ClientToScreen(window, &bottomRight);
    return RECT{topLeft.x, topLeft.y, bottomRight.x, bottomRight.y};
}

static int HitTestDesktopDropIndexFromScreen(HWND window, POINT point)
{
    if (window == nullptr || !IsWindow(window))
    {
        return -1;
    }

    for (int i = 0; i < static_cast<int>(g_state.ids.size()); ++i)
    {
        RECT block = ClientRectToScreenRect(window, BlockRect(window, i));
        InflateRect(&block, DpiScale(window, 12), DpiScale(window, 14));
        if (PtInRect(&block, point))
        {
            return i;
        }

        RECT preview = ClientRectToScreenRect(window, HoverPreviewRect(window, i));
        if (PtInRect(&preview, point))
        {
            return i;
        }
    }

    return -1;
}

static void LogDropHitTestMiss(HWND window, POINT point)
{
    RECT windowRect{};
    RECT client{};
    GetWindowRect(window, &windowRect);
    GetClientRect(window, &client);
    Log(
        "Drop hit miss point=(%ld,%ld) window=(%ld,%ld,%ld,%ld) client=(%ld,%ld,%ld,%ld) desktops=%zu",
        point.x,
        point.y,
        windowRect.left,
        windowRect.top,
        windowRect.right,
        windowRect.bottom,
        client.left,
        client.top,
        client.right,
        client.bottom,
        g_state.ids.size());

    for (int i = 0; i < static_cast<int>(g_state.ids.size()); ++i)
    {
        RECT block = ClientRectToScreenRect(window, BlockRect(window, i));
        RECT preview = ClientRectToScreenRect(window, HoverPreviewRect(window, i));
        Log(
            "Drop rect index=%d block=(%ld,%ld,%ld,%ld) preview=(%ld,%ld,%ld,%ld)",
            i,
            block.left,
            block.top,
            block.right,
            block.bottom,
            preview.left,
            preview.top,
            preview.right,
            preview.bottom);
    }
}

static HWND AppWindowFromPoint(POINT point)
{
    HWND hit = WindowFromPoint(point);
    if (hit == nullptr)
    {
        return nullptr;
    }

    HWND root = GetAncestor(hit, GA_ROOT);
    if (root != nullptr)
    {
        hit = root;
    }

    return IsAppWindow(hit) ? hit : nullptr;
}

static bool IsDragDistance(POINT from, POINT to)
{
    int thresholdX = max(4, GetSystemMetrics(SM_CXDRAG));
    int thresholdY = max(4, GetSystemMetrics(SM_CYDRAG));
    return abs(to.x - from.x) >= thresholdX || abs(to.y - from.y) >= thresholdY;
}

static bool CenterWindowOnMonitor(HWND appWindow, POINT anchorPoint)
{
    if (appWindow == nullptr || !IsWindow(appWindow) || IsIconic(appWindow) || IsZoomed(appWindow))
    {
        return false;
    }

    RECT windowRect{};
    if (!GetWindowRect(appWindow, &windowRect))
    {
        return false;
    }

    HMONITOR monitor = MonitorFromPoint(anchorPoint, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo{};
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (monitor == nullptr || !GetMonitorInfoW(monitor, &monitorInfo))
    {
        return false;
    }

    RECT workArea = monitorInfo.rcWork;
    int width = windowRect.right - windowRect.left;
    int height = windowRect.bottom - windowRect.top;
    int workWidth = workArea.right - workArea.left;
    int workHeight = workArea.bottom - workArea.top;
    if (width <= 0 || height <= 0 || workWidth <= 0 || workHeight <= 0)
    {
        return false;
    }

    int x = workArea.left + (workWidth - width) / 2;
    int y = workArea.top + (workHeight - height) / 2;
    x = max(workArea.left, min(x, workArea.right - width));
    y = max(workArea.top, min(y, workArea.bottom - height));

    BOOL ok = SetWindowPos(
        appWindow,
        nullptr,
        x,
        y,
        0,
        0,
        SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOSIZE);
    Log(
        "Center moved window hwnd=0x%p ok=%d from=(%ld,%ld,%ld,%ld) to=(%d,%d) work=(%ld,%ld,%ld,%ld) err=%lu",
        appWindow,
        ok ? 1 : 0,
        windowRect.left,
        windowRect.top,
        windowRect.right,
        windowRect.bottom,
        x,
        y,
        workArea.left,
        workArea.top,
        workArea.right,
        workArea.bottom,
        GetLastError());
    return ok != FALSE;
}

static bool MoveAppWindowToDesktop(HWND appWindow, int targetIndex)
{
    if (appWindow == nullptr ||
        !IsWindow(appWindow) ||
        !IsAppWindow(appWindow) ||
        targetIndex < 0 ||
        targetIndex >= static_cast<int>(g_state.ids.size()))
    {
        return false;
    }

    GUID sourceId{};
    int sourceIndex = -1;
    HRESULT sourceHr = g_publicDesktopManager != nullptr ? g_publicDesktopManager->GetWindowDesktopId(appWindow, &sourceId) : E_NOINTERFACE;
    if (SUCCEEDED(sourceHr))
    {
        sourceIndex = FindDesktopIndexById(sourceId);
        if (sourceIndex == targetIndex)
        {
            Log("Drop window ignored same desktop hwnd=0x%p target=%d", appWindow, targetIndex);
            return false;
        }
    }

    HRESULT hr = E_FAIL;
    HRESULT viewHr = E_FAIL;
    HRESULT canMoveHr = E_FAIL;
    BOOL canMove = FALSE;
    if (g_applicationViews != nullptr && g_desktopManager != nullptr)
    {
        IUnknown* view = nullptr;
        Log("MoveViewToDesktop begin hwnd=0x%p target=%d", appWindow, targetIndex);
        viewHr = g_applicationViews->GetViewForHwnd(appWindow, &view);
        if (SUCCEEDED(viewHr) && view != nullptr)
        {
            IVirtualDesktop* targetDesktop = GetDesktopAt(static_cast<UINT>(targetIndex));
            if (targetDesktop != nullptr)
            {
                canMoveHr = g_desktopManager->CanViewMoveDesktops(view, &canMove);
                if (SUCCEEDED(canMoveHr) && canMove)
                {
                    hr = g_desktopManager->MoveViewToDesktop(view, targetDesktop);
                }
                else
                {
                    hr = canMoveHr;
                }
                targetDesktop->Release();
            }
            view->Release();
        }
    }

    Log(
        "MoveViewToDesktop hwnd=0x%p source=%d target=%d sourceHr=0x%08X viewHr=0x%08X canMove=%d canMoveHr=0x%08X hr=0x%08X",
        appWindow,
        sourceIndex,
        targetIndex,
        static_cast<unsigned>(sourceHr),
        static_cast<unsigned>(viewHr),
        canMove ? 1 : 0,
        static_cast<unsigned>(canMoveHr),
        static_cast<unsigned>(hr));
    if (FAILED(hr) && g_publicDesktopManager != nullptr)
    {
        hr = g_publicDesktopManager->MoveWindowToDesktop(appWindow, g_state.ids[targetIndex]);
        Log(
            "MoveWindowToDesktop fallback hwnd=0x%p source=%d target=%d hr=0x%08X",
            appWindow,
            sourceIndex,
            targetIndex,
            static_cast<unsigned>(hr));
    }
    if (FAILED(hr))
    {
        return false;
    }

    CenterWindowOnMonitor(appWindow, g_dragDropPoint);

    for (HWND& foregroundWindow : g_state.foregroundWindows)
    {
        if (foregroundWindow == appWindow)
        {
            foregroundWindow = nullptr;
        }
    }
    if (static_cast<size_t>(targetIndex) < g_state.foregroundWindows.size())
    {
        g_state.foregroundWindows[targetIndex] = appWindow;
    }

    if (sourceIndex >= 0)
    {
        ClearThumbnailForDesktop(sourceIndex);
    }
    else
    {
        ClearCurrentDesktopThumbnail();
    }
    Log("Preserved target thumbnail after move target=%d source=%d", targetIndex, sourceIndex);

    ScheduleForegroundRefresh(g_window, 150);
    KillTimer(g_window, DesktopRefreshTimerId);
    SetTimer(g_window, DesktopRefreshTimerId, 500, nullptr);
    RenderPager(g_window);
    return true;
}

static void RepositionOverlay(HWND window)
{
    g_taskbar = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (g_taskbar == nullptr)
    {
        return;
    }

    g_tray = FindWindowExW(g_taskbar, nullptr, L"TrayNotifyWnd", nullptr);

    RECT taskbarRect{};
    RECT trayRect{};
    if (!GetWindowRect(g_taskbar, &taskbarRect))
    {
        return;
    }

    GetWindowRect(g_tray, &trayRect);
    int taskbarWidth = taskbarRect.right - taskbarRect.left;
    int taskbarHeight = taskbarRect.bottom - taskbarRect.top;
    int width = OverlayWidth(window);
    int height = OverlayHeight(window, taskbarHeight);
    int trayLeft = g_tray == nullptr ? taskbarWidth - DpiScale(window, 96) : trayRect.left - taskbarRect.left;
    int x = taskbarRect.left + max(DpiScale(window, 8), trayLeft - width - DpiScale(window, 8));
    int y = taskbarRect.bottom - height;

    SetWindowPos(window, HWND_TOPMOST, x, y, width, height, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
}

static bool IsTaskbarRelatedWindow(HWND window)
{
    if (window == nullptr || window == g_window)
    {
        return false;
    }

    HWND current = window;
    for (int depth = 0; current != nullptr && depth < 4; ++depth)
    {
        if (current == g_taskbar || current == g_tray)
        {
            return true;
        }

        wchar_t className[128]{};
        GetClassNameW(current, className, 128);
        if (wcscmp(className, L"Shell_TrayWnd") == 0 ||
            wcscmp(className, L"Shell_SecondaryTrayWnd") == 0 ||
            wcscmp(className, L"TrayNotifyWnd") == 0 ||
            wcscmp(className, L"NotifyIconOverflowWindow") == 0 ||
            wcscmp(className, L"TopLevelWindowForOverflowXamlIsland") == 0)
        {
            return true;
        }

        current = GetAncestor(current, GA_PARENT);
    }

    return false;
}

static bool IsPointInTaskbarArea(POINT point)
{
    if (g_taskbar == nullptr)
    {
        g_taskbar = FindWindowW(L"Shell_TrayWnd", nullptr);
    }
    if (g_taskbar == nullptr)
    {
        return false;
    }

    RECT taskbarRect{};
    return GetWindowRect(g_taskbar, &taskbarRect) && PtInRect(&taskbarRect, point);
}

static void ScheduleTaskbarRefresh(UINT delayMs = TaskbarRefreshDelayMs, int burstCount = 10)
{
    if (g_window == nullptr)
    {
        return;
    }

    g_taskbarRefreshBurstsRemaining = max(g_taskbarRefreshBurstsRemaining, burstCount);
    KillTimer(g_window, TaskbarRefreshTimerId);
    SetTimer(g_window, TaskbarRefreshTimerId, delayMs == 0 ? 1 : delayMs, nullptr);
}

static void CALLBACK TaskbarEventProc(
    HWINEVENTHOOK,
    DWORD,
    HWND eventWindow,
    LONG objectId,
    LONG,
    DWORD,
    DWORD)
{
    if (g_window == nullptr || objectId != OBJID_WINDOW || !IsTaskbarRelatedWindow(eventWindow))
    {
        return;
    }

    PostMessageW(g_window, WM_TASKBAR_REFRESH, 0, 0);
}

static LRESULT CALLBACK LowLevelMouseProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code == HC_ACTION)
    {
        MSLLHOOKSTRUCT* mouse = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
        if (mouse != nullptr)
        {
            if (wParam == WM_LBUTTONDOWN)
            {
                g_dragCandidateWindow = AppWindowFromPoint(mouse->pt);
                g_dragStartPoint = mouse->pt;
                if (g_dragCandidateWindow != nullptr)
                {
                    Log("Drag candidate hwnd=0x%p start=(%ld,%ld)", g_dragCandidateWindow, mouse->pt.x, mouse->pt.y);
                }
            }
            else if (wParam == WM_LBUTTONUP)
            {
                HWND draggedWindow = g_dragCandidateWindow;
                g_dragCandidateWindow = nullptr;
                int target = HitTestDesktopDropIndexFromScreen(g_window, mouse->pt);
                Log(
                    "Drag release hwnd=0x%p start=(%ld,%ld) end=(%ld,%ld) distance=%d target=%d",
                    draggedWindow,
                    g_dragStartPoint.x,
                    g_dragStartPoint.y,
                    mouse->pt.x,
                    mouse->pt.y,
                    draggedWindow != nullptr && IsDragDistance(g_dragStartPoint, mouse->pt) ? 1 : 0,
                    target);
                if (draggedWindow != nullptr && IsDragDistance(g_dragStartPoint, mouse->pt) && target < 0)
                {
                    LogDropHitTestMiss(g_window, mouse->pt);
                }
                if (draggedWindow != nullptr &&
                    IsDragDistance(g_dragStartPoint, mouse->pt) &&
                    target >= 0)
                {
                    g_dragDropPoint = mouse->pt;
                    g_dragDropTarget = target;
                    g_suppressPagerClickUntil = GetTickCount() + 300;
                    PostMessageW(g_window, WM_DRAGGED_WINDOW_DROPPED, reinterpret_cast<WPARAM>(draggedWindow), 0);
                }
            }

            if ((wParam == WM_LBUTTONDOWN || wParam == WM_LBUTTONUP || wParam == WM_RBUTTONDOWN || wParam == WM_RBUTTONUP) &&
                IsPointInTaskbarArea(mouse->pt) &&
                g_window != nullptr)
            {
                PostMessageW(g_window, WM_TASKBAR_REFRESH, 1, 0);
            }
        }
    }

    return CallNextHookEx(g_mouseHook, code, wParam, lParam);
}

static void RegisterTaskbarEventHook()
{
    if (g_taskbarEventHook != nullptr)
    {
        return;
    }

    g_taskbarEventHook = SetWinEventHook(
        EVENT_OBJECT_SHOW,
        EVENT_OBJECT_LOCATIONCHANGE,
        nullptr,
        TaskbarEventProc,
        0,
        0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
    Log("Taskbar event hook=0x%p err=%lu", g_taskbarEventHook, GetLastError());
}

static void UnregisterTaskbarEventHook()
{
    if (g_taskbarEventHook == nullptr)
    {
        return;
    }

    UnhookWinEvent(g_taskbarEventHook);
    g_taskbarEventHook = nullptr;
}

class NotificationSink : public IVirtualDesktopNotification
{
public:
    ULONG refCount = 1;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override
    {
        if (object == nullptr)
        {
            return E_POINTER;
        }

        if (IsEqualGUID(riid, IID_IUnknown) || IsEqualGUID(riid, IID_IVirtualDesktopNotification))
        {
            *object = static_cast<IVirtualDesktopNotification*>(this);
            AddRef();
            return S_OK;
        }

        *object = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return InterlockedIncrement(&refCount);
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG count = InterlockedDecrement(&refCount);
        if (count == 0)
        {
            delete this;
        }
        return count;
    }

    HRESULT STDMETHODCALLTYPE VirtualDesktopCreated(IVirtualDesktop*) override { NotifyRefresh("Created"); return S_OK; }
    HRESULT STDMETHODCALLTYPE VirtualDesktopDestroyBegin(IVirtualDesktop*, IVirtualDesktop*) override { Log("VD event: DestroyBegin"); return S_OK; }
    HRESULT STDMETHODCALLTYPE VirtualDesktopDestroyFailed(IVirtualDesktop*, IVirtualDesktop*) override { Log("VD event: DestroyFailed"); return S_OK; }
    HRESULT STDMETHODCALLTYPE VirtualDesktopDestroyed(IVirtualDesktop*, IVirtualDesktop*) override { NotifyRefresh("Destroyed"); return S_OK; }
    HRESULT STDMETHODCALLTYPE VirtualDesktopMoved(IVirtualDesktop*, INT64, INT64) override { NotifyRefresh("Moved"); return S_OK; }
    HRESULT STDMETHODCALLTYPE VirtualDesktopNameChanged(IVirtualDesktop*, void*) override { Notify(); return S_OK; }
    HRESULT STDMETHODCALLTYPE ViewVirtualDesktopChanged(IUnknown*) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE CurrentVirtualDesktopChanged(IVirtualDesktop* desktopOld, IVirtualDesktop* desktopNew) override
    {
        if (desktopNew != nullptr)
        {
            extern GUID g_pendingDesktopId;
            extern bool g_hasPendingDesktopSwitch;
            g_pendingDesktopId = desktopNew->GetId();
            g_hasPendingDesktopSwitch = true;
        }
        NotifySwitch("CurrentChanged");
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE VirtualDesktopWallpaperChanged(IVirtualDesktop*, void*) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE VirtualDesktopSwitched(IVirtualDesktop*, int) override { Log("VD event: Switched"); return S_OK; }
    HRESULT STDMETHODCALLTYPE RemoteVirtualDesktopConnected(IVirtualDesktop*) override { NotifyRefresh("RemoteConnected"); return S_OK; }

private:
    void Notify()
    {
        if (g_window != nullptr)
        {
            PostMessageW(g_window, WM_VD_CHANGED, 0, 0);
        }
    }

    void NotifyRefresh(const char* eventName)
    {
        Log("VD event: %s", eventName);
        Notify();
    }

    void NotifySwitch(const char* eventName)
    {
        Log("VD event: %s", eventName);
        if (GetTickCount() < g_ignoreSwitchEventsUntil)
        {
            Log("VD switch event ignored during local switch");
            return;
        }

        if (g_window != nullptr)
        {
            PostMessageW(g_window, WM_VD_CHANGED, VdChangedSwitch, 0);
        }
    }

};

static NotificationSink* g_sink = nullptr;

static bool InitializeVirtualDesktop()
{
    HRESULT hr = CoCreateInstance(CLSID_ImmersiveShell, nullptr, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&g_shell));
    Log("CoCreateInstance ImmersiveShell hr=0x%08X", static_cast<unsigned>(hr));
    if (FAILED(hr) || g_shell == nullptr)
    {
        MessageBoxW(nullptr, L"无法初始化 ImmersiveShell COM 服务。", L"WorkspacePagerMvp", MB_OK | MB_ICONERROR);
        return false;
    }

    hr = g_shell->QueryService(CLSID_VirtualDesktopManagerInternal, IID_IVirtualDesktopManagerInternal, reinterpret_cast<void**>(&g_desktopManager));
    Log("QueryService DesktopManager hr=0x%08X", static_cast<unsigned>(hr));
    if (FAILED(hr) || g_desktopManager == nullptr)
    {
        MessageBoxW(nullptr, L"无法访问当前系统的虚拟桌面管理接口。", L"WorkspacePagerMvp", MB_OK | MB_ICONERROR);
        return false;
    }

    hr = CoCreateInstance(CLSID_VirtualDesktopManager, nullptr, CLSCTX_INPROC_SERVER, IID_IVirtualDesktopManager, reinterpret_cast<void**>(&g_publicDesktopManager));
    Log("CoCreateInstance PublicDesktopManager hr=0x%08X", static_cast<unsigned>(hr));

    hr = g_shell->QueryService(IID_IApplicationViewCollection, IID_IApplicationViewCollection, reinterpret_cast<void**>(&g_applicationViews));
    Log("QueryService ApplicationViewCollection hr=0x%08X", static_cast<unsigned>(hr));

    hr = g_shell->QueryService(CLSID_VirtualNotificationService, IID_IVirtualDesktopNotificationService, reinterpret_cast<void**>(&g_notificationService));
    Log("QueryService NotificationService hr=0x%08X", static_cast<unsigned>(hr));
    if (SUCCEEDED(hr) && g_notificationService != nullptr)
    {
        g_sink = new NotificationSink();
        hr = g_notificationService->Register(g_sink, &g_notificationCookie);
        Log("Register notification hr=0x%08X cookie=%lu", static_cast<unsigned>(hr), g_notificationCookie);
        if (FAILED(hr))
        {
            g_sink->Release();
            g_sink = nullptr;
        }
    }

    RefreshDesktopState();
    int registryCurrentIndex = GetCurrentDesktopIndexFromRegistry();
    if (registryCurrentIndex >= 0)
    {
        g_state.currentIndex = registryCurrentIndex;
        ClearCurrentDesktopThumbnail();
    }
    Log("Initial desktops=%zu current=%d", g_state.ids.size(), g_state.currentIndex);
    return true;
}

static void ShutdownVirtualDesktop()
{
    ClearThumbnails(g_state.thumbnails);

    if (g_notificationService != nullptr && g_notificationCookie != 0)
    {
        g_notificationService->Unregister(g_notificationCookie);
        g_notificationCookie = 0;
    }

    if (g_sink != nullptr)
    {
        g_sink->Release();
        g_sink = nullptr;
    }

    ReleaseUnknown(g_notificationService);
    ReleaseUnknown(g_applicationViews);
    ReleaseUnknown(g_publicDesktopManager);
    ReleaseUnknown(g_desktopManager);
    ReleaseUnknown(g_shell);
    g_notificationService = nullptr;
    g_applicationViews = nullptr;
    g_publicDesktopManager = nullptr;
    g_desktopManager = nullptr;
    g_shell = nullptr;
}

static void SendKey(WORD key, bool keyUp)
{
    INPUT input{};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = key;
    input.ki.dwFlags = keyUp ? KEYEVENTF_KEYUP : 0;
    SendInput(1, &input, sizeof(input));
}

static void SendVirtualDesktopShortcut(bool moveRight)
{
    WORD arrow = moveRight ? VK_RIGHT : VK_LEFT;
    SendKey(VK_CONTROL, false);
    SendKey(VK_LWIN, false);
    SendKey(arrow, false);
    SendKey(arrow, true);
    SendKey(VK_LWIN, true);
    SendKey(VK_CONTROL, true);
}

static void SwitchToDesktop(int index)
{
    if (index < 0 || index >= static_cast<int>(g_state.ids.size()) || index == g_state.currentIndex)
    {
        return;
    }

    IVirtualDesktop* desktop = GetDesktopAt(static_cast<UINT>(index));
    if (desktop != nullptr)
    {
        int delta = index - g_state.currentIndex;
        Log("Send Ctrl+Win+Arrow target=%d current=%d delta=%d", index, g_state.currentIndex, delta);
        RefreshCurrentForegroundWindow();
        CaptureThumbnailForDesktop(g_state.currentIndex, true, "pager-switch");
        g_state.currentIndex = index;
        ClearCurrentDesktopThumbnail();
        g_ignoreSwitchEventsUntil = GetTickCount() + static_cast<DWORD>(max(900, abs(delta) * 450));
        g_ignoreKeyboardCaptureUntil = GetTickCount() + static_cast<DWORD>(max(900, abs(delta) * 450));
        RenderPager(g_window);
        for (int i = 0; i < abs(delta); ++i)
        {
            SendVirtualDesktopShortcut(delta > 0);
            Sleep(80);
        }
        desktop->Release();
    }
}

static void DrawRoundedRect(HDC dc, RECT rect, int radius, HBRUSH brush)
{
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    HPEN pen = CreatePen(PS_NULL, 0, RGB(0, 0, 0));
    HGDIOBJ oldPen = SelectObject(dc, pen);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(pen);
}

static void StrokeRoundedRect(HDC dc, RECT rect, int radius, COLORREF color, int width)
{
    HBRUSH hollow = static_cast<HBRUSH>(GetStockObject(HOLLOW_BRUSH));
    HGDIOBJ oldBrush = SelectObject(dc, hollow);
    HPEN pen = CreatePen(PS_SOLID, width, color);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(pen);
}

static void FillRectAlpha(unsigned char* pixels, int width, int height, RECT rect, COLORREF color, unsigned char alpha)
{
    rect.left = max(0, min(rect.left, width));
    rect.right = max(0, min(rect.right, width));
    rect.top = max(0, min(rect.top, height));
    rect.bottom = max(0, min(rect.bottom, height));

    unsigned char r = static_cast<unsigned char>(GetRValue(color));
    unsigned char g = static_cast<unsigned char>(GetGValue(color));
    unsigned char b = static_cast<unsigned char>(GetBValue(color));

    // UpdateLayeredWindow expects premultiplied BGRA.
    unsigned char pr = static_cast<unsigned char>((static_cast<int>(r) * alpha) / 255);
    unsigned char pg = static_cast<unsigned char>((static_cast<int>(g) * alpha) / 255);
    unsigned char pb = static_cast<unsigned char>((static_cast<int>(b) * alpha) / 255);

    for (int y = rect.top; y < rect.bottom; ++y)
    {
        unsigned char* row = pixels + y * width * 4;
        for (int x = rect.left; x < rect.right; ++x)
        {
            unsigned char* pixel = row + x * 4;
            pixel[0] = pb;
            pixel[1] = pg;
            pixel[2] = pr;
            pixel[3] = alpha;
        }
    }
}

static void RepairAlphaInRect(unsigned char* pixels, int width, int height, RECT rect)
{
    rect.left = max(0, min(rect.left, width));
    rect.right = max(0, min(rect.right, width));
    rect.top = max(0, min(rect.top, height));
    rect.bottom = max(0, min(rect.bottom, height));

    for (int y = rect.top; y < rect.bottom; ++y)
    {
        unsigned char* row = pixels + y * width * 4;
        for (int x = rect.left; x < rect.right; ++x)
        {
            unsigned char* pixel = row + x * 4;
            if (pixel[3] == 0 && (pixel[0] != 0 || pixel[1] != 0 || pixel[2] != 0))
            {
                pixel[3] = 255;
            }
        }
    }
}

static void ApplyColorKeyAlpha(unsigned char* pixels, int width, int height, COLORREF transparentColor)
{
    unsigned char transparentR = static_cast<unsigned char>(GetRValue(transparentColor));
    unsigned char transparentG = static_cast<unsigned char>(GetGValue(transparentColor));
    unsigned char transparentB = static_cast<unsigned char>(GetBValue(transparentColor));

    for (int y = 0; y < height; ++y)
    {
        unsigned char* row = pixels + y * width * 4;
        for (int x = 0; x < width; ++x)
        {
            unsigned char* pixel = row + x * 4;
            bool transparent = pixel[0] == transparentB && pixel[1] == transparentG && pixel[2] == transparentR;
            pixel[3] = transparent ? 0 : 255;
            if (transparent)
            {
                pixel[0] = 0;
                pixel[1] = 0;
                pixel[2] = 0;
            }
        }
    }
}

static void EnsureMinimumAlphaInRect(unsigned char* pixels, int width, int height, RECT rect, COLORREF color, unsigned char alpha)
{
    rect.left = max(0, min(rect.left, width));
    rect.right = max(0, min(rect.right, width));
    rect.top = max(0, min(rect.top, height));
    rect.bottom = max(0, min(rect.bottom, height));

    unsigned char r = static_cast<unsigned char>(GetRValue(color));
    unsigned char g = static_cast<unsigned char>(GetGValue(color));
    unsigned char b = static_cast<unsigned char>(GetBValue(color));
    unsigned char pr = static_cast<unsigned char>((static_cast<int>(r) * alpha) / 255);
    unsigned char pg = static_cast<unsigned char>((static_cast<int>(g) * alpha) / 255);
    unsigned char pb = static_cast<unsigned char>((static_cast<int>(b) * alpha) / 255);

    for (int y = rect.top; y < rect.bottom; ++y)
    {
        unsigned char* row = pixels + y * width * 4;
        for (int x = rect.left; x < rect.right; ++x)
        {
            unsigned char* pixel = row + x * 4;
            if (pixel[3] < alpha)
            {
                pixel[0] = pb;
                pixel[1] = pg;
                pixel[2] = pr;
                pixel[3] = alpha;
            }
        }
    }
}

struct IconResult
{
    HICON icon = nullptr;
    bool owned = false;
};

static IconResult GetProcessIcon(HWND window)
{
    DWORD processId = 0;
    GetWindowThreadProcessId(window, &processId);
    if (processId == 0)
    {
        return {};
    }

    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (process == nullptr)
    {
        return {};
    }

    wchar_t path[MAX_PATH]{};
    DWORD pathLength = MAX_PATH;
    BOOL ok = QueryFullProcessImageNameW(process, 0, path, &pathLength);
    CloseHandle(process);
    if (!ok || path[0] == L'\0')
    {
        return {};
    }

    SHFILEINFOW fileInfo{};
    DWORD_PTR result = SHGetFileInfoW(
        path,
        0,
        &fileInfo,
        sizeof(fileInfo),
        SHGFI_ICON | SHGFI_LARGEICON);
    if (result == 0 || fileInfo.hIcon == nullptr)
    {
        return {};
    }

    return {fileInfo.hIcon, true};
}

static IconResult GetWindowIcon(HWND window)
{
    if (window == nullptr || !IsWindow(window))
    {
        return {};
    }

    IconResult processIcon = GetProcessIcon(window);
    if (processIcon.icon != nullptr)
    {
        return processIcon;
    }

    HICON icon = reinterpret_cast<HICON>(SendMessageW(window, WM_GETICON, ICON_BIG, 0));
    if (icon == nullptr)
    {
        icon = reinterpret_cast<HICON>(GetClassLongPtrW(window, GCLP_HICON));
    }
    if (icon == nullptr)
    {
        icon = reinterpret_cast<HICON>(SendMessageW(window, WM_GETICON, ICON_SMALL2, 0));
    }
    if (icon == nullptr)
    {
        icon = reinterpret_cast<HICON>(SendMessageW(window, WM_GETICON, ICON_SMALL, 0));
    }
    if (icon == nullptr)
    {
        icon = reinterpret_cast<HICON>(GetClassLongPtrW(window, GCLP_HICONSM));
    }

    return {icon, false};
}

static RECT DrawDesktopIcon(HDC dc, HWND pagerWindow, RECT card, HWND appWindow, bool active)
{
    int iconSize = DpiScale(pagerWindow, 24);
    int x = card.left + ((card.right - card.left) - iconSize) / 2;
    int y = card.top + ((card.bottom - card.top) - iconSize) / 2;
    RECT iconRect{x, y, x + iconSize, y + iconSize};

    IconResult icon = GetWindowIcon(appWindow);
    if (icon.icon != nullptr)
    {
        DrawIconEx(dc, x, y, icon.icon, iconSize, iconSize, 0, nullptr, DI_NORMAL);
        if (icon.owned)
        {
            DestroyIcon(icon.icon);
        }
        return iconRect;
    }

    HBRUSH brush = CreateSolidBrush(active ? RGB(0, 120, 212) : RGB(190, 190, 190));
    DrawRoundedRect(dc, iconRect, DpiScale(pagerWindow, 4), brush);
    DeleteObject(brush);
    return iconRect;
}

static bool DrawBitmapScaled(HDC dc, RECT target, HBITMAP bitmap)
{
    if (bitmap == nullptr)
    {
        return false;
    }

    BITMAP bitmapInfo{};
    if (GetObjectW(bitmap, sizeof(bitmapInfo), &bitmapInfo) == 0 || bitmapInfo.bmWidth <= 0 || bitmapInfo.bmHeight <= 0)
    {
        return false;
    }
    if (target.right <= target.left || target.bottom <= target.top)
    {
        return false;
    }

    HDC sourceDc = CreateCompatibleDC(dc);
    HGDIOBJ oldBitmap = SelectObject(sourceDc, bitmap);
    SetStretchBltMode(dc, HALFTONE);
    SetBrushOrgEx(dc, 0, 0, nullptr);
    StretchBlt(
        dc,
        target.left,
        target.top,
        target.right - target.left,
        target.bottom - target.top,
        sourceDc,
        0,
        0,
        bitmapInfo.bmWidth,
        bitmapInfo.bmHeight,
        SRCCOPY);
    SelectObject(sourceDc, oldBitmap);
    DeleteDC(sourceDc);
    return true;
}

static bool DrawDesktopThumbnail(HDC dc, HWND pagerWindow, RECT card, HBITMAP thumbnail)
{
    RECT thumbRect{
        card.left + DpiScale(pagerWindow, 2),
        card.top + DpiScale(pagerWindow, 2),
        card.right - DpiScale(pagerWindow, 2),
        card.bottom - DpiScale(pagerWindow, 7)};
    return DrawBitmapScaled(dc, thumbRect, thumbnail);
}

static void DrawHoverPreview(HDC dc, HWND pagerWindow, int index)
{
    if (index < 0 || static_cast<size_t>(index) >= g_state.ids.size())
    {
        return;
    }

    RECT preview = HoverPreviewRect(pagerWindow, index);
    StrokeRoundedRect(dc, preview, DpiScale(pagerWindow, 4), RGB(64, 156, 255), DpiScale(pagerWindow, 1));

    RECT content{
        preview.left + DpiScale(pagerWindow, 6),
        preview.top + DpiScale(pagerWindow, 6),
        preview.right - DpiScale(pagerWindow, 6),
        preview.bottom - DpiScale(pagerWindow, 6)};

    HBITMAP thumbnail = static_cast<size_t>(index) < g_state.thumbnails.size() ? g_state.thumbnails[index] : nullptr;
    if (!DrawBitmapScaled(dc, content, thumbnail))
    {
        HWND appWindow = static_cast<size_t>(index) < g_state.foregroundWindows.size() ? g_state.foregroundWindows[index] : nullptr;
        DrawDesktopIcon(dc, pagerWindow, content, appWindow, index == g_state.currentIndex);
    }
}

static void RenderPager(HWND window)
{
    RECT client{};
    GetClientRect(window, &client);
    int width = client.right - client.left;
    int height = client.bottom - client.top;
    if (width <= 0 || height <= 0)
    {
        return;
    }
    SyncCurrentDesktopIndex("render");

    BITMAPINFO bitmapInfo{};
    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = -height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    unsigned char* pixels = nullptr;
    HDC memoryDc = CreateCompatibleDC(nullptr);
    HBITMAP bitmap = CreateDIBSection(memoryDc, &bitmapInfo, DIB_RGB_COLORS, reinterpret_cast<void**>(&pixels), nullptr, 0);
    if (bitmap == nullptr)
    {
        DeleteDC(memoryDc);
        return;
    }

    HGDIOBJ oldBitmap = SelectObject(memoryDc, bitmap);

    COLORREF transparentColor = RGB(255, 0, 255);
    HBRUSH transparentBrush = CreateSolidBrush(transparentColor);
    FillRect(memoryDc, &client, transparentBrush);
    DeleteObject(transparentBrush);

    COLORREF activeBar = RGB(0, 120, 212);
    COLORREF hoverBar = RGB(64, 156, 255);
    COLORREF inactiveBar = RGB(120, 120, 120);
    COLORREF hitTestColor = RGB(1, 1, 1);
    int hoverIndex = (g_hoverIndex >= 0 && static_cast<size_t>(g_hoverIndex) < g_state.ids.size()) ? g_hoverIndex : -1;
    int previewIndex = hoverIndex == g_state.currentIndex ? -1 : hoverIndex;

    for (int i = 0; i < static_cast<int>(g_state.ids.size()); ++i)
    {
        RECT rect = BlockRect(window, i);
        bool active = i == g_state.currentIndex;
        bool hovered = i == hoverIndex;

        if (!active)
        {
            HBITMAP thumbnail = static_cast<size_t>(i) < g_state.thumbnails.size() ? g_state.thumbnails[i] : nullptr;
            if (!DrawDesktopThumbnail(memoryDc, window, rect, thumbnail))
            {
                HWND appWindow = static_cast<size_t>(i) < g_state.foregroundWindows.size() ? g_state.foregroundWindows[i] : nullptr;
                DrawDesktopIcon(memoryDc, window, rect, appWindow, active);
            }
        }

        if (hovered)
        {
            StrokeRoundedRect(memoryDc, rect, DpiScale(window, 3), hoverBar, DpiScale(window, 1));
        }

        int barWidth = DpiScale(window, (active || hovered) ? 30 : 18);
        int barHeight = DpiScale(window, (active || hovered) ? 3 : 2);
        int barX = rect.left + ((rect.right - rect.left) - barWidth) / 2;
        RECT bar{
            barX,
            rect.bottom - DpiScale(window, 4),
            barX + barWidth,
            rect.bottom - DpiScale(window, 4) + barHeight};
        HBRUSH barBrush = CreateSolidBrush(active ? activeBar : (hovered ? hoverBar : inactiveBar));
        FillRect(memoryDc, &bar, barBrush);
        DeleteObject(barBrush);
    }

    DrawHoverPreview(memoryDc, window, previewIndex);

    if (pixels != nullptr)
    {
        ApplyColorKeyAlpha(pixels, width, height, transparentColor);
        for (int i = 0; i < static_cast<int>(g_state.ids.size()); ++i)
        {
            EnsureMinimumAlphaInRect(pixels, width, height, BlockRect(window, i), hitTestColor, 1);
        }
    }

    RECT windowRect{};
    GetWindowRect(window, &windowRect);
    POINT destination{windowRect.left, windowRect.top};
    POINT source{0, 0};
    SIZE size{width, height};
    BLENDFUNCTION blend{};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;
    UpdateLayeredWindow(window, nullptr, &destination, &size, memoryDc, &source, 0, &blend, ULW_ALPHA);

    SelectObject(memoryDc, oldBitmap);
    DeleteObject(bitmap);
    DeleteDC(memoryDc);
}

static void RemoveTrayIcon(HWND window)
{
    if (!g_trayIconAdded)
    {
        return;
    }

    NOTIFYICONDATAW data{};
    data.cbSize = sizeof(data);
    data.hWnd = window;
    data.uID = TrayIconId;
    Shell_NotifyIconW(NIM_DELETE, &data);
    g_trayIconAdded = false;
}

static bool AddTrayIcon(HWND window)
{
    NOTIFYICONDATAW data{};
    data.cbSize = sizeof(data);
    data.hWnd = window;
    data.uID = TrayIconId;
    data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    data.uCallbackMessage = WM_TRAYICON;
    data.hIcon = LoadIconW(nullptr, MAKEINTRESOURCEW(32512));
    wcscpy_s(data.szTip, L"WorkspacePagerMvp");

    g_trayIconAdded = Shell_NotifyIconW(NIM_ADD, &data) != FALSE;
    Log("Add tray icon ok=%d err=%lu", g_trayIconAdded ? 1 : 0, GetLastError());
    return g_trayIconAdded;
}

static void ShowTrayContextMenu(HWND window)
{
    POINT screenPoint{};
    if (!GetCursorPos(&screenPoint))
    {
        return;
    }

    HMENU menu = CreatePopupMenu();
    if (menu == nullptr)
    {
        return;
    }

    AppendMenuW(menu, MF_STRING | MF_GRAYED, TrayMenuSettingsCommand, L"设置");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, TrayMenuExitCommand, L"退出");

    SetForegroundWindow(window);
    UINT command = TrackPopupMenu(
        menu,
        TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
        screenPoint.x,
        screenPoint.y,
        0,
        window,
        nullptr);
    DestroyMenu(menu);
    PostMessageW(window, WM_NULL, 0, 0);

    if (command == TrayMenuExitCommand)
    {
        Log("Exit requested from tray menu");
        DestroyWindow(window);
    }
}

static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        SetTimer(window, RepositionTimerId, 1000, nullptr);
        return 0;

    case WM_TIMER:
        if (wParam == RepositionTimerId)
        {
            RefreshCurrentForegroundWindow();
            RepositionOverlay(window);
            RenderPager(window);
        }
        else if (wParam == TaskbarRefreshTimerId)
        {
            KillTimer(window, TaskbarRefreshTimerId);
            RepositionOverlay(window);
            RenderPager(window);
            if (g_taskbarRefreshBurstsRemaining > 0)
            {
                --g_taskbarRefreshBurstsRemaining;
                SetTimer(window, TaskbarRefreshTimerId, TaskbarRefreshBurstDelayMs, nullptr);
            }
        }
        else if (wParam == DesktopRefreshTimerId)
        {
            KillTimer(window, DesktopRefreshTimerId);
            Log("Delayed desktop refresh begin");
            RefreshDesktopState();
            Log("Delayed desktop refresh desktops=%zu current=%d", g_state.ids.size(), g_state.currentIndex);
            RepositionOverlay(window);
            RenderPager(window);
        }
        else if (wParam == ForegroundRefreshTimerId)
        {
            KillTimer(window, ForegroundRefreshTimerId);
            RefreshCurrentForegroundWindow();
            Log("Foreground icon refresh current=%d", g_state.currentIndex);
            RenderPager(window);
        }
        else if (wParam == SwitchRefreshTimerId)
        {
            KillTimer(window, SwitchRefreshTimerId);
            bool updated = false;

            if (g_hasPendingDesktopSwitch)
            {
                int newIndex = FindDesktopIndexById(g_pendingDesktopId);
                if (newIndex >= 0 && static_cast<size_t>(newIndex) < g_state.ids.size())
                {
                    g_state.currentIndex = newIndex;
                    ClearCurrentDesktopThumbnail();
                    g_hasPendingDesktopSwitch = false;
                    updated = true;
                    Log("Pending switch refresh current=%d", g_state.currentIndex);
                }
            }

            if (!updated)
            {
                int registryCurrentIndex = GetCurrentDesktopIndexFromRegistry();
                if (registryCurrentIndex >= 0 && static_cast<size_t>(registryCurrentIndex) < g_state.ids.size())
                {
                    g_state.currentIndex = registryCurrentIndex;
                    ClearCurrentDesktopThumbnail();
                    Log("Registry switch refresh current=%d", g_state.currentIndex);
                }
            }

            RenderPager(window);
        }
        return 0;

    case WM_TASKBAR_REFRESH:
        if (wParam != 0)
        {
            ScheduleTaskbarRefresh(1, 15);
            RepositionOverlay(window);
            RenderPager(window);
        }
        else
        {
            ScheduleTaskbarRefresh();
        }
        return 0;

    case WM_VD_CHANGED:
        if (wParam == VdChangedSwitch)
        {
            KillTimer(window, SwitchRefreshTimerId);
            SetTimer(window, SwitchRefreshTimerId, 150, nullptr);
            Log("WM_VD_CHANGED scheduled registry switch refresh");
        }
        else
        {
            KillTimer(window, DesktopRefreshTimerId);
            SetTimer(window, DesktopRefreshTimerId, DesktopStructureRefreshDelayMs, nullptr);
            Log("WM_VD_CHANGED scheduled delayed refresh");
        }
        return 0;

    case WM_TRAYICON:
        if (wParam == TrayIconId && (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU))
        {
            ShowTrayContextMenu(window);
        }
        return 0;

    case WM_DRAGGED_WINDOW_DROPPED:
    {
        HWND draggedWindow = reinterpret_cast<HWND>(wParam);
        int target = g_dragDropTarget >= 0 ? g_dragDropTarget : HitTestDesktopDropIndexFromScreen(window, g_dragDropPoint);
        g_dragDropTarget = -1;
        if (target >= 0)
        {
            MoveAppWindowToDesktop(draggedWindow, target);
        }
        return 0;
    }

    case WM_MOUSEMOVE:
    {
        if (!g_trackingMouse)
        {
            TRACKMOUSEEVENT track{};
            track.cbSize = sizeof(track);
            track.dwFlags = TME_LEAVE;
            track.hwndTrack = window;
            g_trackingMouse = TrackMouseEvent(&track) != FALSE;
        }

        POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        int hoverIndex = HitTestDesktopIndex(window, point);
        if (hoverIndex != g_hoverIndex)
        {
            g_hoverIndex = hoverIndex;
            RenderPager(window);
        }
        return 0;
    }

    case WM_MOUSELEAVE:
        g_trackingMouse = false;
        if (g_hoverIndex != -1)
        {
            g_hoverIndex = -1;
            RenderPager(window);
        }
        return 0;

    case WM_LBUTTONUP:
    {
        POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        int target = HitTestDesktopIndex(window, point);
        if (target >= 0)
        {
            if (GetTickCount() < g_suppressPagerClickUntil)
            {
                return 0;
            }
            SwitchToDesktop(target);
        }
        return 0;
    }

    case WM_MOUSEWHEEL:
    {
        int count = static_cast<int>(g_state.ids.size());
        if (count <= 1)
        {
            return 0;
        }

        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        int direction = delta < 0 ? 1 : -1;
        int target = (g_state.currentIndex + direction + count) % count;
        Log("Mouse wheel delta=%d target=%d", delta, target);
        SwitchToDesktop(target);
        return 0;
    }

    case WM_PAINT:
        ValidateRect(window, nullptr);
        RenderPager(window);
        return 0;

    case WM_DISPLAYCHANGE:
    case WM_SETTINGCHANGE:
    case WM_DPICHANGED:
        RepositionOverlay(window);
        RenderPager(window);
        return 0;

    case WM_DESTROY:
        UnregisterTaskbarEventHook();
        RemoveTrayIcon(window);
        KillTimer(window, RepositionTimerId);
        KillTimer(window, DesktopRefreshTimerId);
        KillTimer(window, ForegroundRefreshTimerId);
        KillTimer(window, SwitchRefreshTimerId);
        KillTimer(window, TaskbarRefreshTimerId);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, wchar_t*, int)
{
    SetUnhandledExceptionFilter(UnhandledExceptionHandler);
    ConfigureDpiAwareness();
    Log("Starting WorkspacePagerMvp");
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    if (!InitializeVirtualDesktop())
    {
        ShutdownVirtualDesktop();
        CoUninitialize();
        return 1;
    }

    g_keyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandleW(nullptr), 0);
    Log("Keyboard hook hwnd=0x%p err=%lu", g_keyboardHook, GetLastError());
    g_mouseHook = SetWindowsHookExW(WH_MOUSE_LL, LowLevelMouseProc, GetModuleHandleW(nullptr), 0);
    Log("Mouse hook hwnd=0x%p err=%lu", g_mouseHook, GetLastError());

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.hInstance = instance;
    wc.lpfnWndProc = WindowProc;
    wc.lpszClassName = WindowClassName;
    wc.hCursor = LoadCursor(nullptr, IDC_HAND);
    wc.hbrBackground = nullptr;
    RegisterClassExW(&wc);
    Log("RegisterClass done");

    g_taskbar = FindWindowW(L"Shell_TrayWnd", nullptr);
    Log("Taskbar hwnd=0x%p", g_taskbar);
    if (g_taskbar == nullptr)
    {
        MessageBoxW(nullptr, L"找不到 Windows 任务栏窗口。", L"WorkspacePagerMvp", MB_OK | MB_ICONERROR);
        ShutdownVirtualDesktop();
        CoUninitialize();
        return 1;
    }

    DWORD exStyle = WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED;
    DWORD style = WS_POPUP | WS_CLIPSIBLINGS;
    g_window = CreateWindowExW(
        exStyle,
        WindowClassName,
        L"WorkspacePagerMvp",
        style,
        0,
        0,
        OverlayWidth(nullptr),
        OverlayHeight(nullptr, DpiScale(nullptr, 40)),
        g_taskbar,
        nullptr,
        instance,
        nullptr);
    Log("CreateWindow hwnd=0x%p err=%lu", g_window, GetLastError());

    if (g_window == nullptr)
    {
        ShutdownVirtualDesktop();
        CoUninitialize();
        return 1;
    }

    AddTrayIcon(g_window);
    RegisterTaskbarEventHook();
    RepositionOverlay(g_window);
    RenderPager(g_window);
    ShowWindow(g_window, SW_SHOWNOACTIVATE);
    Log("Entering message loop");

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    ShutdownVirtualDesktop();
    if (g_keyboardHook != nullptr)
    {
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = nullptr;
    }
    if (g_mouseHook != nullptr)
    {
        UnhookWindowsHookEx(g_mouseHook);
        g_mouseHook = nullptr;
    }
    CoUninitialize();
    Log("Exiting WorkspacePagerMvp");
    return 0;
}
