#pragma once

#include <windows.h>
#include <windowsx.h>
#include <ole2.h>
#include <shellapi.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <vector>

#include "VirtualDesktopApi.h"

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

struct DesktopState
{
    std::vector<GUID> ids;
    std::vector<HWND> foregroundWindows;
    std::vector<HBITMAP> thumbnails;
    int currentIndex = 0;
};

extern HWND g_window;
extern HWND g_taskbar;
extern HWND g_tray;
extern HHOOK g_keyboardHook;
extern HHOOK g_mouseHook;
extern HWINEVENTHOOK g_taskbarEventHook;
extern IServiceProvider* g_shell;
extern IVirtualDesktopManagerInternal* g_desktopManager;
extern IVirtualDesktopManager* g_publicDesktopManager;
extern IApplicationViewCollection* g_applicationViews;
extern IVirtualDesktopNotificationService* g_notificationService;
extern DWORD g_notificationCookie;
extern DesktopState g_state;
extern DWORD g_ignoreSwitchEventsUntil;
extern DWORD g_ignoreKeyboardCaptureUntil;
extern int g_hoverIndex;
extern int g_taskbarRefreshBurstsRemaining;
extern bool g_trackingMouse;
extern bool g_trayIconAdded;
extern HWND g_dragCandidateWindow;
extern POINT g_dragStartPoint;
extern POINT g_dragDropPoint;
extern int g_dragDropTarget;
extern DWORD g_suppressPagerClickUntil;
extern GUID g_pendingDesktopId;
extern bool g_hasPendingDesktopSwitch;

void Log(const char* format, ...);
LONG WINAPI UnhandledExceptionHandler(EXCEPTION_POINTERS* exceptionInfo);
void ConfigureDpiAwareness();
void ReleaseUnknown(IUnknown* object);

int DpiScale(HWND window, int value);
int HoverPreviewContentWidth(HWND window);
int HoverPreviewContentHeight(HWND window);
int OverlayWidth(HWND window);
int OverlayHeight(HWND window, int taskbarHeight);
RECT BlockRect(HWND window, int index);
RECT HoverPreviewRect(HWND window, int index);
int HitTestDesktopIndex(HWND window, POINT point);
int HitTestDesktopDropIndexFromScreen(HWND window, POINT point);
void LogDropHitTestMiss(HWND window, POINT point);
void RepositionOverlay(HWND window);

int FindDesktopIndexById(const GUID& id);
int GetCurrentDesktopIndexFromRegistry();
void ClearThumbnails(std::vector<HBITMAP>& thumbnails);
void ClearThumbnailForDesktop(int index);
void ClearCurrentDesktopThumbnail();
bool SyncCurrentDesktopIndex(const char* reason);
bool IsAppWindow(HWND window);
void RefreshDesktopState();
void RefreshCurrentForegroundWindow();
void ScheduleForegroundRefresh(HWND window, UINT delayMs = 250);
void CaptureThumbnailForDesktop(int index, bool allowCurrentDesktop = false, const char* reason = "unknown");
bool MoveAppWindowToDesktop(HWND appWindow, int targetIndex);
void SwitchToDesktop(int index);

bool InitializeVirtualDesktop();
void ShutdownVirtualDesktop();

LRESULT CALLBACK LowLevelKeyboardProc(int code, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK LowLevelMouseProc(int code, WPARAM wParam, LPARAM lParam);
void ScheduleTaskbarRefresh(UINT delayMs = TaskbarRefreshDelayMs, int burstCount = 10);
void RegisterTaskbarEventHook();
void UnregisterTaskbarEventHook();

void RenderPager(HWND window);

bool AddTrayIcon(HWND window);
void RemoveTrayIcon(HWND window);
void ShowTrayContextMenu(HWND window);
