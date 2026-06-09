#include "AppShared.h"

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
