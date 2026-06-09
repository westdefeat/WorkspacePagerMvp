#pragma once

#include <windows.h>
#include <ole2.h>

inline const GUID CLSID_ImmersiveShell = {0xC2F03A33, 0x21F5, 0x47FA, {0xB4, 0xBB, 0x15, 0x63, 0x62, 0xA2, 0xF2, 0x39}};
inline const GUID CLSID_VirtualDesktopManagerInternal = {0xC5E0CDCA, 0x7B6E, 0x41B2, {0x9F, 0xC4, 0xD9, 0x39, 0x75, 0xCC, 0x46, 0x7B}};
inline const GUID CLSID_VirtualDesktopManager = {0xAA509086, 0x5CA9, 0x4C25, {0x8F, 0x95, 0x58, 0x9D, 0x3C, 0x07, 0xB4, 0x8A}};
inline const GUID CLSID_VirtualNotificationService = {0xA501FDEC, 0x4A09, 0x464C, {0xAE, 0x4E, 0x1B, 0x9C, 0x21, 0xB8, 0x49, 0x18}};
inline const GUID IID_IVirtualDesktopManagerInternal = {0x53F5CA0B, 0x158F, 0x4124, {0x90, 0x0C, 0x05, 0x71, 0x58, 0x06, 0x0B, 0x27}};
inline const GUID IID_IVirtualDesktopManager = {0xA5CD92FF, 0x29BE, 0x454C, {0x8D, 0x04, 0xD8, 0x28, 0x79, 0xFB, 0x3F, 0x1B}};
inline const GUID IID_IVirtualDesktopNotificationService = {0x0CD45E71, 0xD927, 0x4F15, {0x8B, 0x0A, 0x8F, 0xEF, 0x52, 0x53, 0x37, 0xBF}};
inline const GUID IID_IVirtualDesktopNotification = {0xB9E5E94D, 0x233E, 0x49AB, {0xAF, 0x5C, 0x2B, 0x45, 0x41, 0xC3, 0xAA, 0xDE}};
inline const GUID IID_IVirtualDesktop = {0x3F07F4BE, 0xB107, 0x441A, {0xAF, 0x0F, 0x39, 0xD8, 0x25, 0x29, 0x07, 0x2C}};
inline const GUID IID_IApplicationViewCollection = {0x1841C6D7, 0x4F9D, 0x42C0, {0xAF, 0x41, 0x87, 0x47, 0x53, 0x8F, 0x10, 0xE5}};

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
