# WorkspacePagerMvp

WorkspacePagerMvp is a small Win32/C++ prototype that adds a virtual desktop pager to the Windows taskbar. It shows one block per Windows virtual desktop, keeps cached thumbnail previews for inactive desktops, and lets you switch desktops by clicking or using the mouse wheel.

The project is intentionally minimal: native C++ source files, a PowerShell build script, a Ninja build file, and no external runtime dependency. It is built as an experiment around the Windows virtual desktop APIs and taskbar overlay behavior.

## Features

- Shows all current Windows virtual desktops in a compact taskbar overlay.
- Highlights the active desktop with an underline.
- Captures a cached thumbnail when leaving a desktop and reuses it as that desktop's preview.
- Falls back to the foreground app icon when a desktop does not have a cached thumbnail yet.
- Leaves the current desktop preview blank because it is live and is not continuously captured.
- Updates after virtual desktop create, destroy, move, rename, and switch notifications.
- Switches desktops through the native `Ctrl + Win + Left/Right` path for smooth Windows animations.
- Supports mouse wheel switching on the pager, including wraparound from the last desktop to the first.
- Stays near the system tray and does not create its own taskbar button.

## Screenshot

No screenshot is included yet. The UI is a thin taskbar overlay made of desktop preview blocks, with the active desktop marked by a highlighted bottom bar.

## Requirements

- Windows 11, tested on build `10.0.26220`.
- LLVM `clang++`, expected by default at:

```text
C:\Program Files\LLVM\bin\clang++.exe
```

- Ninja, available on `PATH`.

If Ninja is missing, install it with:

```powershell
winget install --id Ninja-build.Ninja -e
```

## Build

From PowerShell:

```powershell
cd "c:\Users\xiaolin\source\windows_workspaces\WorkspacePagerMvp"
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

The build script calls Ninja, which compiles the C++ source files in parallel by default and writes intermediate object files under `.build/`.

Limit the number of parallel build jobs:

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Jobs 4
```

Build with a custom output file name:

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Output WorkspacePagerMvpDebug.exe
```

The build uses C++17 and links directly against the Win32 libraries used by the app.

## Run

```powershell
cd "c:\Users\xiaolin\source\windows_workspaces\WorkspacePagerMvp"
.\WorkspacePagerMvp.exe
```

Stop the running process:

```powershell
taskkill /IM WorkspacePagerMvp.exe /F
```

## How It Works

WorkspacePagerMvp uses a small always-on-top Win32 overlay window positioned near the Windows taskbar tray area. The overlay paints one slot per virtual desktop. Each slot can show a cached desktop bitmap, an application icon fallback, or an empty current-desktop placeholder.

For virtual desktop state, the app uses the documented `IVirtualDesktopManager` interface where possible, but it also calls undocumented Shell COM interfaces such as `IVirtualDesktopManagerInternal`, `IVirtualDesktopNotificationService`, `IVirtualDesktopNotification`, and `IVirtualDesktop`. These interfaces are used to enumerate desktops, identify the current desktop, and receive desktop change notifications.

For switching, the app intentionally sends the native `Ctrl + Win + Left/Right` shortcut instead of directly invoking the internal switch method. This keeps Windows' own transition animation and avoids depending on more internal behavior than necessary.

## Important Notes

This is an MVP and uses undocumented Windows virtual desktop COM interfaces. Those GUIDs, vtables, and callback shapes are not part of Microsoft's stable public API. They may change after Windows updates, especially on Insider builds.

The previews are cached snapshots, not live previews. Windows does not expose a public API that lets third-party apps capture live thumbnails of inactive virtual desktops. WorkspacePagerMvp captures the visible desktop around switching time and reuses that bitmap as a best-effort preview.

## Current Limitations

- The overlay cannot reserve real taskbar layout space, so it cannot push pinned taskbar icons aside.
- The safest placement is near the right-side system tray area.
- Clicking a non-adjacent desktop switches through intermediate desktops because the app follows the native keyboard shortcut path.
- Manual `Ctrl + Win + Left/Right` switching is detected through Shell notifications, but direction is inferred conservatively for stability.
- Windows updates may require updating the internal virtual desktop interface definitions.

## Troubleshooting

Check whether the process is running:

```powershell
tasklist /FI "IMAGENAME eq WorkspacePagerMvp.exe"
```

Check the runtime log:

```powershell
Get-Content .\WorkspacePagerMvp.log
```

Restart the app:

```powershell
taskkill /IM WorkspacePagerMvp.exe /F
.\WorkspacePagerMvp.exe
```

## References and Credits

WorkspacePagerMvp does not vendor or link against the projects below, but they were useful references for understanding the undocumented Windows virtual desktop surface and the tradeoffs involved.

- [Ciantic/VirtualDesktopAccessor](https://github.com/Ciantic/VirtualDesktopAccessor): a widely used DLL for accessing Windows 10/11 virtual desktop features, especially from AutoHotkey. Useful as a reference for COM identifiers, exported desktop operations, and the reality of per-build compatibility.
- [Slion/VirtualDesktop](https://github.com/Slion/VirtualDesktop): a C# wrapper around `IVirtualDesktopManager` and related undocumented interfaces. Useful for understanding the interface set and the need to handle Windows build-specific changes.
- [dankrusi/WindowsVirtualDesktopHelper](https://github.com/dankrusi/WindowsVirtualDesktopHelper): a lightweight helper app for showing and switching virtual desktops. Useful as a product-level reference for tray/taskbar virtual desktop indicators.
- [shanselman/MaximizeToVirtualDesktop](https://github.com/shanselman/MaximizeToVirtualDesktop): documents the difference between Microsoft's limited documented virtual desktop API and the undocumented interfaces needed for richer desktop control.
- [Microsoft PowerToys issue #6797](https://github.com/microsoft/PowerToys/issues/6797): useful background on why PowerToys avoids undocumented virtual desktop APIs and why this area remains fragile for third-party tools.

## 中文简介

WorkspacePagerMvp 是一个 Windows 任务栏虚拟桌面分页器原型。它会在任务栏托盘附近显示每个虚拟桌面的预览块，支持点击和鼠标滚轮切换桌面。

项目主要用英文文档维护。需要注意的是，它使用了 Windows 未公开的虚拟桌面 COM 接口，因此 Windows 更新后可能需要重新适配接口定义。

## License

MIT License. See `LICENSE`.
