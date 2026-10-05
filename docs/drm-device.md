# Armbian ARM64：原生 Clang / DRM

此后端直接通过 libdrm 接管 KMS 输出，不需要 SDL、X11 或 Wayland。界面数据仍然是 mock。
当前实现针对已确认的 720×1280 MIPI DSI 面板，UI 坐标始终为 1280×720。
本地已完成 Clang 构建、逻辑测试和错误路径检查；实际 ARM64 出图、触控及性能需要在设备验证。

## 构建

在 ARM64 Armbian 上执行：

```bash
sudo apt install clang cmake ninja-build pkg-config libdrm-dev libfreetype6-dev fonts-noto-cjk evtest
git submodule update --init --recursive
cmake --preset arm64-drm-native
cmake --build --preset arm64-drm-native --parallel
ctest --preset arm64-drm-native
file build/arm64-drm-native/edge-panel
```

产物为 `build/arm64-drm-native/edge-panel`。此 preset 使用执行机器的架构，不做交叉编译；在 ARM64 设备上预期为 ELF64 / AArch64。测试只运行无需屏幕的主题与坐标测试，不会占用 DRM 或触摸设备。

## 第一次运行

通过 SSH 操作。先停止 flutter-pi 或占用 DRM 的桌面/compositor 服务，记录服务名，测试结束后可重新启动原服务。确保后台没有另一个面板进程。

先只验证显示：

```bash
sudo ./build/arm64-drm-native/edge-panel --drm-device /dev/dri/card0 --rotation 90 --no-touch
```

程序自动选择该 card 上已连接、提供 720×1280 模式的 DSI 输出及兼容 CRTC。日志会打印实际 ID；如需明确指定，依据当前 `modetest -M rockchip -c -p` 的结果使用：

```bash
sudo ./build/arm64-drm-native/edge-panel --drm-device /dev/dri/card0 --connector 54 --crtc 39 --rotation 90 --no-touch
```

之前设备查询得到 connector 54 / CRTC 39，但 ID 可能随内核/启动配置变化，不写死在程序里。若安装方向相反，将 `--rotation 90` 换成 `--rotation 270`。旋转方向定义为从横屏逻辑画面到面板物理坐标的顺时针角度。

Ctrl+C 或 SIGTERM 会执行资源回收，并尝试恢复启动前的 CRTC 模式和连接器绑定。SIGKILL 或掉电无法执行恢复流程；若旧程序释放了原 framebuffer，模式恢复也可能失败，重新启动原显示服务即可。

## 触摸与校准

用 `sudo evtest` 查看设备列表，选择已通过设备树配置的 I²C 触摸设备，确认 ABS 轴事件。优先使用 `/dev/input/by-path/...` 的稳定链接，避免 event 编号变化。

```bash
sudo ./build/arm64-drm-native/edge-panel \
  --drm-device /dev/dri/card0 --rotation 90 \
  --touch-device /dev/input/eventX --touch-test
```

诊断页有四角和中心共五个目标，点击后变色并累计命中；同时显示当前逻辑坐标。应依次确认五点均可准确点击、拖动方向正确，再运行正常面板（移除 `--touch-test`）。

输入先按内核 ABS 范围归一化，再进行显示旋转的逆变换。支持单点 `ABS_X/Y + BTN_TOUCH` 和 type-B 多点协议，后者选择一个活动触点作为 LVGL 指针。type-A 无 slot 协议尚未支持。启动日志打印实际 ABS 范围；事件丢失时通过 ioctl 重新同步输入状态。

如果驱动原始坐标与物理屏幕方向不一致，可以组合：

- `--touch-swap-xy`：交换原始轴及其范围。
- `--touch-invert-x`、`--touch-invert-y`：交换轴并归一化后，在所选坐标空间反向。
- `--touch-space logical`：输入已经对应横屏逻辑方向时使用，跳过应用的逆旋转。默认是 `physical`。

这些参数用于校准，不能在设备树和应用中重复做相同变换。`--no-touch` 仅用于显示测试；正常运行需明确指定触摸路径，程序不会猜测输入设备。

设备路径也可通过 `EDGE_PANEL_DRM_DEVICE`、`EDGE_PANEL_TOUCH_DEVICE` 指定，命令行参数优先。中文字体通过 `EDGE_PANEL_FONT` 指定或自动查找 Noto CJK。

## 权限与运行

首次测试可使用 sudo。长期运行建议让服务用户具备 DRM card 和触摸节点的访问权限（通常为 video/input 组，以 `ls -l` 结果为准），并确认没有其他 DRM master。若报告 Acquire DRM master 失败，先检查旧前端与 compositor。

当前 DRM 后端采用 legacy modeset/page flip，配合 XRGB8888 dumb buffer；不需要 GPU 渲染节点或 plane 的硬件旋转。每次有完整 LVGL 更新提交时，将暂存画面软件旋转到空闲 scanout buffer，等待 vblank 翻页。正在扫描或等待翻页的 buffer 不会被写入；翻页期间的更新合并到 CPU 暂存画面。空闲时不持续重画，翻页超时会报错退出。

当前软件旋转会处理完整 1280×720 画面，尚未按脏矩形优化。设备验证时观察首次出图、每分钟配色切换和触摸按钮响应的 CPU/耗时，再决定是否优化。系统需支持 legacy KMS、dumb buffers 和 page flip；仅支持 atomic 的设备可能需要另加 atomic 路径。

`--self-test` 和 `--screenshot` 仅用于 SDL 版本，DRM 版本会拒绝，避免测试意外接管显示。DRM 的主题/坐标测试由 `ctest --preset arm64-drm-native` 提供。
