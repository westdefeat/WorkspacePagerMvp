#include "AppShared.h"

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

bool InitializeVirtualDesktop()
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

void ShutdownVirtualDesktop()
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
