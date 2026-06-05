# WorkspacePagerMvp

Windows taskbar workspace pager MVP. It shows your virtual desktops as small taskbar thumbnails and lets you switch desktops by clicking them.

Windows 任务栏虚拟桌面指示器 MVP。它会把当前虚拟桌面显示成任务栏上的小缩略卡片，并支持点击切换桌面。

## Features / 功能

- Shows all current Windows virtual desktops.
- Highlights the active desktop.
- Updates through Windows virtual desktop notification events.
- Switches desktops with the native `Ctrl + Win + Left/Right` shortcut path for smoother animation.
- Runs as a taskbar overlay near the system tray.
- Does not create its own taskbar button.

- 显示当前所有 Windows 虚拟桌面。
- 高亮当前桌面。
- 通过 Windows 虚拟桌面事件通知实时刷新。
- 使用系统原生 `Ctrl + Win + 左/右` 快捷键路径切换，动画更顺滑。
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

The thumbnail cards are not real live desktop previews. Windows does not provide a public API for third-party apps to capture live thumbnails of each virtual desktop. The current UI is a compact visual approximation based on each desktop's window distribution.

当前缩略卡片不是真正的实时桌面预览。Windows 没有公开 API 允许第三方直接获取每个虚拟桌面的实时缩略图。当前 UI 是基于各桌面窗口分布绘制的轻量示意图。

## Current Limitations / 当前限制

- The overlay cannot reserve real taskbar space, so it cannot push pinned taskbar icons aside.
- The safest position is near the system tray on the right side.
- Clicking a non-adjacent desktop switches through intermediate desktops because it follows the native `Ctrl + Win + Left/Right` shortcut path.
- If Windows changes the virtual desktop COM interfaces, the tool may need an update.

- overlay 无法真正占用任务栏布局空间，因此不能把固定图标挤开。
- 当前最稳的位置是在右侧系统托盘附近。
- 点击非相邻桌面时，会通过中间桌面切过去，因为它走的是原生 `Ctrl + Win + 左/右` 快捷键路径。
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
