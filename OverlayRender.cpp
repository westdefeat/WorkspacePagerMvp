#include "AppShared.h"

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

void RenderPager(HWND window)
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
