# WorkspacePagerMvp

Windows taskbar workspace pager MVP. It shows your virtual desktops as cached taskbar thumbnails and lets you switch desktops by clicking or scrolling.

Windows 任务栏虚拟桌面指示器 MVP。它会把当前虚拟桌面显示成任务栏上的缓存缩略图，并支持点击或滚轮切换桌面。

## Features / 功能

- Shows all current Windows virtual desktops.
- Captures and caches a thumbnail for a desktop when leaving it.
- Falls back to the foreground/topmost app icon when a desktop has no cached thumbnail yet.
- Shows the current desktop as an empty placeholder with a highlighted underline, because it is live and not continuously captured.
- Highlights the active desktop.
- Updates through Windows virtual desktop notification events.
- Switches desktops with the native `Ctrl + Win + Left/Right` shortcut path for smoother animation.
- Supports mouse wheel switching on the indicator, including wrapping from the last desktop to the first.
- Runs as a taskbar overlay near the system tray.
- Does not create its own taskbar button.

- 显示当前所有 Windows 虚拟桌面。
- 离开某个桌面时截取并缓存该桌面的缩略图。
- 如果某个桌面还没有缓存缩略图，则回退显示前台/最上层应用图标。
- 当前桌面显示为空白占位并使用高亮横杠，因为当前桌面是实时变化的，不会持续截图。
- 高亮当前桌面。
- 通过 Windows 虚拟桌面事件通知实时刷新。
- 使用系统原生 `Ctrl + Win + 左/右` 快捷键路径切换，动画更顺滑。
- 支持在指示器上使用鼠标滚轮切换桌面，包括从最后一个循环到第一个。
- 作为任务栏 overlay 显示在系统托盘左侧附近。
- 不在任务栏生成自己的程序按钮。

## Files / 文件

- `WorkspacePagerMvp.cpp`: main Win32/C++ source file.
- `build.ps1`: PowerShell build script.
- `WorkspacePagerMvp.exe`: generated executable after building.
- `WorkspacePagerMvp.log`: runtime diagnostic log, generated automatically.

- `WorkspacePagerMvp.cpp`：Win32/C++ 主源码。
- `build.ps1`：PowerShell 构建脚本。
- `WorkspacePagerMvp.exe`：构建后生成的可执行文件。
- `WorkspacePagerMvp.log`：运行时诊断日志，会自动生成。

## Run / 运行

From PowerShell:

在 PowerShell 中运行：

```powershell
cd c:\Users\xiaolin\source\windows_workspaces\WorkspacePagerMvp
.\WorkspacePagerMvp.exe
```

Stop it with:

停止程序：

```powershell
taskkill /IM WorkspacePagerMvp.exe /F
```

## Build / 构建

Requires LLVM `clang++` at:

需要本机安装 LLVM `clang++`，默认路径：

```text
C:\Program Files\LLVM\bin\clang++.exe
```

Build:

构建：

```powershell
cd c:\Users\xiaolin\source\windows_workspaces\WorkspacePagerMvp
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

Build to a custom output name:

构建到自定义文件名：

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Output WorkspacePagerMvpDebug.exe
```

## Notes / 说明

This tool uses undocumented Windows virtual desktop COM interfaces. It is known to work on the tested Windows build `10.0.26220`, but these internal interface IDs and vtables may change after Windows updates.

本工具使用了未公开的 Windows 虚拟桌面 COM 接口。它已在当前测试系统 `10.0.26220` 上验证可用，但这些内部接口的 GUID 和 vtable 可能会随 Windows 更新变化。

The thumbnails are cached snapshots, not live previews. Windows does not provide a public API for third-party apps to capture live thumbnails of inactive virtual desktops. The tool captures the currently visible desktop before/after switching and reuses that cached image for the matching desktop.

当前缩略图是缓存截图，不是实时预览。Windows 没有公开 API 允许第三方直接获取非活动虚拟桌面的实时缩略图。本工具会在切换前后截取当前可见桌面，并把缓存图用于对应桌面。

## Current Limitations / 当前限制

- The overlay cannot reserve real taskbar space, so it cannot push pinned taskbar icons aside.
- The safest position is near the system tray on the right side.
- Clicking a non-adjacent desktop switches through intermediate desktops because it follows the native `Ctrl + Win + Left/Right` shortcut path.
- Manual `Ctrl + Win + Left/Right` switching is detected through shell notifications, but direction is inferred conservatively because the undocumented callback object is intentionally not dereferenced for stability.
- If Windows changes the virtual desktop COM interfaces, the tool may need an update.

- overlay 无法真正占用任务栏布局空间，因此不能把固定图标挤开。
- 当前最稳的位置是在右侧系统托盘附近。
- 点击非相邻桌面时，会通过中间桌面切过去，因为它走的是原生 `Ctrl + Win + 左/右` 快捷键路径。
- 手动 `Ctrl + Win + 左/右` 切换会通过 Shell 通知同步，但为了稳定性不会解引用未公开回调对象，因此方向只能保守推断。
- 如果 Windows 更新改变虚拟桌面 COM 接口，工具可能需要适配。

## Troubleshooting / 故障排查

If the indicator does not appear:

如果指示器没有出现：

```powershell
tasklist /FI "IMAGENAME eq WorkspacePagerMvp.exe"
```

Check the log:

查看日志：

```powershell
Get-Content .\WorkspacePagerMvp.log
```

Restart:

重启：

```powershell
taskkill /IM WorkspacePagerMvp.exe /F
.\WorkspacePagerMvp.exe
```
