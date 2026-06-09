#include "AppShared.h"

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

int FindDesktopIndexById(const GUID& id)
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

int GetCurrentDesktopIndexFromRegistry()
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

void ClearThumbnails(std::vector<HBITMAP>& thumbnails)
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

void ClearThumbnailForDesktop(int index)
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

void ClearCurrentDesktopThumbnail()
{
    ClearThumbnailForDesktop(g_state.currentIndex);
}

bool SyncCurrentDesktopIndex(const char* reason)
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

bool IsAppWindow(HWND window)
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

void RefreshDesktopState()
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

void RefreshCurrentForegroundWindow()
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

void ScheduleForegroundRefresh(HWND window, UINT delayMs)
{
    if (window != nullptr)
    {
        SetTimer(window, ForegroundRefreshTimerId, delayMs, nullptr);
    }
}

void CaptureThumbnailForDesktop(int index, bool allowCurrentDesktop, const char* reason)
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

bool MoveAppWindowToDesktop(HWND appWindow, int targetIndex)
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

void SwitchToDesktop(int index)
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
