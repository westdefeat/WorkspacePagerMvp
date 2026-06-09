#include "AppShared.h"

LRESULT CALLBACK LowLevelKeyboardProc(int code, WPARAM wParam, LPARAM lParam)
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

void ScheduleTaskbarRefresh(UINT delayMs, int burstCount)
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

LRESULT CALLBACK LowLevelMouseProc(int code, WPARAM wParam, LPARAM lParam)
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

void RegisterTaskbarEventHook()
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

void UnregisterTaskbarEventHook()
{
    if (g_taskbarEventHook == nullptr)
    {
        return;
    }

    UnhookWinEvent(g_taskbarEventHook);
    g_taskbarEventHook = nullptr;
}
