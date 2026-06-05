#include <windows.h>
#include <windowsx.h>
#include <ole2.h>
#include <cstdarg>
#include <cstdio>
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

static const wchar_t* WindowClassName = L"WorkspacePagerMvpWindow";
static const UINT WM_VD_CHANGED = WM_APP + 1;
static const UINT_PTR RepositionTimerId = 1;

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
    virtual BOOL STDMETHODCALLTYPE CanViewMoveDesktops(IUnknown* view) = 0;
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
    std::vector<int> windowCounts;
    int currentIndex = 0;
};

static HWND g_window = nullptr;
static HWND g_taskbar = nullptr;
static HWND g_tray = nullptr;
static IServiceProvider* g_shell = nullptr;
static IVirtualDesktopManagerInternal* g_desktopManager = nullptr;
static IVirtualDesktopManager* g_publicDesktopManager = nullptr;
static IVirtualDesktopNotificationService* g_notificationService = nullptr;
static DWORD g_notificationCookie = 0;
static DesktopState g_state;
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

static COLORREF Blend(COLORREF from, COLORREF to, double t)
{
    int r = static_cast<int>(GetRValue(from) + (GetRValue(to) - GetRValue(from)) * t);
    int g = static_cast<int>(GetGValue(from) + (GetGValue(to) - GetGValue(from)) * t);
    int b = static_cast<int>(GetBValue(from) + (GetBValue(to) - GetBValue(from)) * t);
    return RGB(r, g, b);
}

static void ReleaseUnknown(IUnknown* object)
{
    if (object != nullptr)
    {
        object->Release();
    }
}

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

static BOOL CALLBACK CountWindowForDesktop(HWND window, LPARAM)
{
    if (!IsAppWindow(window) || g_publicDesktopManager == nullptr)
    {
        return TRUE;
    }

    GUID desktopId{};
    if (FAILED(g_publicDesktopManager->GetWindowDesktopId(window, &desktopId)))
    {
        return TRUE;
    }

    int index = FindDesktopIndexById(desktopId);
    if (index >= 0 && static_cast<size_t>(index) < g_state.windowCounts.size())
    {
        g_state.windowCounts[index]++;
    }

    return TRUE;
}

static void RefreshDesktopState()
{
    DesktopState next;

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

        IVirtualDesktop* current = g_desktopManager->GetCurrentDesktop();
        if (current != nullptr)
        {
            GUID currentId = current->GetId();
            for (size_t i = 0; i < next.ids.size(); ++i)
            {
                if (IsEqualGUID(next.ids[i], currentId))
                {
                    next.currentIndex = static_cast<int>(i);
                    break;
                }
            }
            current->Release();
        }
    }

    if (next.ids.empty())
    {
        next.ids.push_back(GUID{});
        next.currentIndex = 0;
    }

    next.windowCounts.assign(next.ids.size(), 0);
    g_state = next;
    EnumWindows(CountWindowForDesktop, 0);
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
    int block = DpiScale(window, 42);
    int gap = DpiScale(window, 6);
    int padding = DpiScale(window, 6);
    return padding * 2 + count * block + max(0, count - 1) * gap;
}

static RECT BlockRect(HWND window, int index)
{
    RECT client{};
    GetClientRect(window, &client);

    int block = DpiScale(window, 42);
    int gap = DpiScale(window, 6);
    int totalWidth = PagerWidth(window);
    int startX = (client.right - client.left - totalWidth) / 2 + DpiScale(window, 6);
    int y = (client.bottom - client.top - DpiScale(window, 24)) / 2;

    RECT rect{};
    rect.left = startX + index * (block + gap);
    rect.top = y;
    rect.right = rect.left + block;
    rect.bottom = rect.top + DpiScale(window, 24);
    return rect;
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
    int width = PagerWidth(window);
    int height = min(taskbarHeight, DpiScale(window, 34));
    int trayLeft = g_tray == nullptr ? taskbarWidth - DpiScale(window, 96) : trayRect.left - taskbarRect.left;
    int x = max(DpiScale(window, 8), trayLeft - width - DpiScale(window, 8));
    int y = max(0, (taskbarHeight - height) / 2);

    SetWindowPos(window, nullptr, x, y, width, height, SWP_NOACTIVATE | SWP_NOZORDER);
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
    HRESULT STDMETHODCALLTYPE VirtualDesktopDestroyBegin(IVirtualDesktop*, IVirtualDesktop*) override { NotifyRefresh("DestroyBegin"); return S_OK; }
    HRESULT STDMETHODCALLTYPE VirtualDesktopDestroyFailed(IVirtualDesktop*, IVirtualDesktop*) override { NotifyRefresh("DestroyFailed"); return S_OK; }
    HRESULT STDMETHODCALLTYPE VirtualDesktopDestroyed(IVirtualDesktop*, IVirtualDesktop*) override { NotifyRefresh("Destroyed"); return S_OK; }
    HRESULT STDMETHODCALLTYPE VirtualDesktopMoved(IVirtualDesktop*, INT64, INT64) override { NotifyRefresh("Moved"); return S_OK; }
    HRESULT STDMETHODCALLTYPE VirtualDesktopNameChanged(IVirtualDesktop*, void*) override { Notify(); return S_OK; }
    HRESULT STDMETHODCALLTYPE ViewVirtualDesktopChanged(IUnknown*) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE CurrentVirtualDesktopChanged(IVirtualDesktop*, IVirtualDesktop* desktopNew) override { NotifyCurrent("CurrentChanged", desktopNew); return S_OK; }
    HRESULT STDMETHODCALLTYPE VirtualDesktopWallpaperChanged(IVirtualDesktop*, void*) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE VirtualDesktopSwitched(IVirtualDesktop* desktop, int) override { NotifyCurrent("Switched", desktop); return S_OK; }
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

    void NotifyCurrent(const char* eventName, IVirtualDesktop* desktop)
    {
        int index = -1;
        if (desktop != nullptr)
        {
            index = FindDesktopIndexById(desktop->GetId());
        }

        Log("VD event: %s index=%d", eventName, index);
        if (g_window != nullptr)
        {
            PostMessageW(g_window, WM_VD_CHANGED, index >= 0 ? static_cast<WPARAM>(index + 1) : 0, 0);
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
    Log("Initial desktops=%zu current=%d", g_state.ids.size(), g_state.currentIndex);
    return true;
}

static void ShutdownVirtualDesktop()
{
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
    ReleaseUnknown(g_publicDesktopManager);
    ReleaseUnknown(g_desktopManager);
    ReleaseUnknown(g_shell);
    g_notificationService = nullptr;
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

static void DrawMiniWindows(HDC dc, HWND window, RECT card, int count, bool active)
{
    int visibleCount = min(max(count, 1), 4);
    COLORREF windowColor = active ? RGB(236, 248, 255) : RGB(205, 205, 205);
    COLORREF titleColor = active ? RGB(0, 120, 212) : RGB(135, 135, 135);

    for (int i = 0; i < visibleCount; ++i)
    {
        int col = i % 2;
        int row = i / 2;
        int left = card.left + DpiScale(window, 6) + col * DpiScale(window, 15);
        int top = card.top + DpiScale(window, 6) + row * DpiScale(window, 8);
        RECT mini{left, top, left + DpiScale(window, 12), top + DpiScale(window, 7)};

        HBRUSH body = CreateSolidBrush(windowColor);
        FillRect(dc, &mini, body);
        DeleteObject(body);

        RECT title{mini.left, mini.top, mini.right, mini.top + DpiScale(window, 2)};
        HBRUSH titleBrush = CreateSolidBrush(titleColor);
        FillRect(dc, &title, titleBrush);
        DeleteObject(titleBrush);
    }
}

static void PaintPager(HWND window)
{
    PAINTSTRUCT ps{};
    HDC dc = BeginPaint(window, &ps);

    RECT client{};
    GetClientRect(window, &client);

    HDC memoryDc = CreateCompatibleDC(dc);
    HBITMAP bitmap = CreateCompatibleBitmap(dc, client.right - client.left, client.bottom - client.top);
    HGDIOBJ oldBitmap = SelectObject(memoryDc, bitmap);

    HBRUSH transparentBrush = CreateSolidBrush(RGB(255, 0, 255));
    FillRect(memoryDc, &client, transparentBrush);
    DeleteObject(transparentBrush);

    COLORREF activeBackground = RGB(31, 111, 187);
    COLORREF inactiveBackground = RGB(64, 64, 64);
    COLORREF inactiveBorder = RGB(122, 122, 122);

    for (int i = 0; i < static_cast<int>(g_state.ids.size()); ++i)
    {
        RECT rect = BlockRect(window, i);
        bool active = i == g_state.currentIndex;
        HBRUSH brush = CreateSolidBrush(active ? activeBackground : inactiveBackground);
        DrawRoundedRect(memoryDc, rect, DpiScale(window, 5), brush);
        DeleteObject(brush);

        int count = static_cast<size_t>(i) < g_state.windowCounts.size() ? g_state.windowCounts[i] : 0;
        DrawMiniWindows(memoryDc, window, rect, count, active);
        StrokeRoundedRect(memoryDc, rect, DpiScale(window, 5), active ? RGB(180, 225, 255) : inactiveBorder, active ? DpiScale(window, 2) : DpiScale(window, 1));
    }

    BitBlt(dc, 0, 0, client.right - client.left, client.bottom - client.top, memoryDc, 0, 0, SRCCOPY);
    SelectObject(memoryDc, oldBitmap);
    DeleteObject(bitmap);
    DeleteDC(memoryDc);

    EndPaint(window, &ps);
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
            RepositionOverlay(window);
        }
        return 0;

    case WM_VD_CHANGED:
        if (wParam > 0 && static_cast<size_t>(wParam - 1) < g_state.ids.size())
        {
            g_state.currentIndex = static_cast<int>(wParam - 1);
            Log("WM_VD_CHANGED set current=%d", g_state.currentIndex);
        }
        else
        {
            RefreshDesktopState();
            Log("WM_VD_CHANGED refresh desktops=%zu current=%d", g_state.ids.size(), g_state.currentIndex);
        }
        RepositionOverlay(window);
        InvalidateRect(window, nullptr, FALSE);
        UpdateWindow(window);
        return 0;

    case WM_LBUTTONUP:
    {
        POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        for (int i = 0; i < static_cast<int>(g_state.ids.size()); ++i)
        {
            RECT rect = BlockRect(window, i);
            if (PtInRect(&rect, point))
            {
                SwitchToDesktop(i);
                return 0;
            }
        }
        return 0;
    }

    case WM_PAINT:
        PaintPager(window);
        return 0;

    case WM_DISPLAYCHANGE:
    case WM_SETTINGCHANGE:
    case WM_DPICHANGED:
        RepositionOverlay(window);
        InvalidateRect(window, nullptr, FALSE);
        return 0;

    case WM_DESTROY:
        KillTimer(window, RepositionTimerId);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, wchar_t*, int)
{
    Log("Starting WorkspacePagerMvp");
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    if (!InitializeVirtualDesktop())
    {
        ShutdownVirtualDesktop();
        CoUninitialize();
        return 1;
    }

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
        PagerWidth(nullptr),
        32,
        nullptr,
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

    SetLayeredWindowAttributes(g_window, RGB(255, 0, 255), 0, LWA_COLORKEY);
    HWND oldParent = SetParent(g_window, g_taskbar);
    Log("SetParent old=0x%p err=%lu", oldParent, GetLastError());
    RepositionOverlay(g_window);
    ShowWindow(g_window, SW_SHOWNOACTIVATE);
    UpdateWindow(g_window);
    Log("Entering message loop");

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    ShutdownVirtualDesktop();
    CoUninitialize();
    Log("Exiting WorkspacePagerMvp");
    return 0;
}
