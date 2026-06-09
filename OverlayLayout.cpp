#include "AppShared.h"

int DpiScale(HWND window, int value)
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

int HoverPreviewContentWidth(HWND window)
{
    return HoverPreviewWidth(window) - DpiScale(window, 12);
}

int HoverPreviewContentHeight(HWND window)
{
    return HoverPreviewHeight(window) - DpiScale(window, 12);
}

static int OverlayPreviewBandHeight(HWND window)
{
    return HoverPreviewHeight(window) + DpiScale(window, 14);
}

int OverlayWidth(HWND window)
{
    return max(PagerWidth(window), HoverPreviewWidth(window) + DpiScale(window, 12));
}

static int IndicatorBandHeight(HWND window)
{
    return DpiScale(window, 40);
}

int OverlayHeight(HWND window, int taskbarHeight)
{
    int indicatorBand = min(taskbarHeight, IndicatorBandHeight(window));
    return indicatorBand + OverlayPreviewBandHeight(window);
}

RECT BlockRect(HWND window, int index)
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

RECT HoverPreviewRect(HWND window, int index)
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

int HitTestDesktopIndex(HWND window, POINT point)
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

int HitTestDesktopDropIndexFromScreen(HWND window, POINT point)
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

void LogDropHitTestMiss(HWND window, POINT point)
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

struct OverlayPlacement
{
    int baseX = 0;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

static int ClampOverlayX(const RECT& taskbarRect, int width, int x, int margin)
{
    int minX = taskbarRect.left + margin;
    int maxX = max(minX, taskbarRect.right - width - margin);
    return min(max(x, minX), maxX);
}

static bool CalculateOverlayPlacement(HWND window, OverlayPlacement& placement)
{
    g_taskbar = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (g_taskbar == nullptr)
    {
        return false;
    }

    g_tray = FindWindowExW(g_taskbar, nullptr, L"TrayNotifyWnd", nullptr);

    RECT taskbarRect{};
    RECT trayRect{};
    if (!GetWindowRect(g_taskbar, &taskbarRect))
    {
        return false;
    }

    if (g_tray != nullptr)
    {
        GetWindowRect(g_tray, &trayRect);
    }
    int taskbarWidth = taskbarRect.right - taskbarRect.left;
    int taskbarHeight = taskbarRect.bottom - taskbarRect.top;
    int width = OverlayWidth(window);
    int height = OverlayHeight(window, taskbarHeight);
    int trayLeft = g_tray == nullptr ? taskbarWidth - DpiScale(window, 96) : trayRect.left - taskbarRect.left;
    int margin = DpiScale(window, 8);
    int baseX = taskbarRect.left + max(margin, trayLeft - width - margin);

    placement.baseX = ClampOverlayX(taskbarRect, width, baseX, margin);
    placement.x = ClampOverlayX(taskbarRect, width, placement.baseX + g_overlayManualOffsetX, margin);
    placement.y = taskbarRect.bottom - height;
    placement.width = width;
    placement.height = height;
    g_overlayManualOffsetX = placement.x - placement.baseX;
    return true;
}

void RepositionOverlay(HWND window)
{
    OverlayPlacement placement{};
    if (!CalculateOverlayPlacement(window, placement))
    {
        return;
    }

    SetWindowPos(
        window,
        HWND_TOPMOST,
        placement.x,
        placement.y,
        placement.width,
        placement.height,
        SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
}

void MoveOverlayHorizontally(HWND window, int desiredX)
{
    OverlayPlacement placement{};
    if (!CalculateOverlayPlacement(window, placement))
    {
        return;
    }

    RECT taskbarRect{};
    if (!GetWindowRect(g_taskbar, &taskbarRect))
    {
        return;
    }

    int margin = DpiScale(window, 8);
    placement.x = ClampOverlayX(taskbarRect, placement.width, desiredX, margin);
    g_overlayManualOffsetX = placement.x - placement.baseX;

    SetWindowPos(
        window,
        HWND_TOPMOST,
        placement.x,
        placement.y,
        placement.width,
        placement.height,
        SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
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

