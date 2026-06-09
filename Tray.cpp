#include "AppShared.h"

void RemoveTrayIcon(HWND window)
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

bool AddTrayIcon(HWND window)
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

void ShowTrayContextMenu(HWND window)
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
