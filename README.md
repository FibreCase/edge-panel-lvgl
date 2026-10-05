# LVGL Edge Panel

用于 Armbian 触摸屏的桌面信息面板，计划替换已有 flutter-pi 显示前端。
当前已实现 LVGL v9.6.0 的 1280×720 横屏 mock 界面、SDL 桌面模拟器，以及 DRM/KMS + 软件旋转 + evdev 平台层。设备后端已编译并通过逻辑测试，真实 ARM64 硬件验证待完成。

现有后端已完成静态研究，协议与接入建议见 [后端接入分析](docs/backend-integration.md)。参考源码位于 `refer/`，不纳入版本控制。

LVGL 使用 `third_party/lvgl` Git 子模块，固定到 `v9.6.0`。首次建立及后续拉取步骤见 [依赖说明](docs/dependencies.md)。

## 运行 mock 界面

使用 Clang 原生编译或交叉编译 ARM64，请见 [ARM64 构建说明](docs/arm64-build.md)。工具链与 preset 已提供，交叉构建需要匹配 Armbian 的 sysroot。

直接在 ARM64 设备编译并通过 DRM 显示，请使用 `arm64-drm-native` preset，步骤见 [DRM 设备运行说明](docs/drm-device.md)。它不依赖 SDL，并提供五点触摸校准页。

Debian/Ubuntu/Armbian 的桌面开发环境安装依赖：

```bash
sudo apt install build-essential cmake pkg-config libsdl2-dev libfreetype6-dev fonts-noto-cjk
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/edge-panel
```

窗口需要可用的图形会话，当前不能直接通过 DRM 接管 Armbian 屏幕。按 Esc 或关闭窗口退出。
程序自动查找常见 Noto CJK 字体路径，也可用 `EDGE_PANEL_FONT=/path/to/font.ttf ./build/edge-panel` 指定包含中英文、数字与标点的字体。

左侧显示系统本地时间、mock 天气，右侧显示示例日程和消息。点击“下一条消息”循环查看文本、程序生成的风景图片和应用通知；“刷新示例”交替改变温度；“切换连接”模拟离线，保留最近数据且暂停 mock 刷新。所有网络数据均为 mock，没有请求真实后端。

界面配色为动态的 Material Design 3 深色方案，种子色相每分钟前进 6°，一小时转满整个色环，详见下节。

无窗口测试与截图：

```bash
ctest --test-dir build --output-on-failure
SDL_VIDEODRIVER=dummy ./build/edge-panel --screenshot build/panel.bmp
SDL_VIDEODRIVER=dummy ./build/edge-panel --seed-hue 200 --message 1 --screenshot build/image-message.bmp
```

测试覆盖三类消息循环、图片显隐、刷新、离线时不更新数据，以及 HCT 色彩空间、MD3 深色方案与种子轮换。截图由实际 LVGL 软件绘制生成。

## 动态配色

配色由 `src/ui/theme/` 生成，不依赖 LVGL，可脱离图形环境测试。

- **色彩空间**：`material_color.cpp` 包装官方 Material Color Utilities 的 HCT solver，处理色域映射，不再维护自写 CAM16 求解算法。
- **配色方案**：`color_scheme.cpp` 使用官方 `SchemeTonalSpot`，深色、标准对比度（0.0）。种子用 HCT 色相、彩度 48、明度 50 构造，官方方案的主色调板彩度为 36；各颜色角色通过官方动态角色 API 获取，不再以固定基线颜色代替动态方案。源码及提交记录位于 `third_party/material-color-utilities/`，不依赖 `refer/`。
- **种子轮换**：`seed_hue_at()` 返回 `分钟 × 6°`，整点从 0° 开始，一小时转满 360°。`Panel::tick()` 检测到分钟变化时重算方案并重新着色。
- **应用方式**：界面对象在创建时绑定配色角色（`Panel::bind`），配色变化时统一按角色重刷，不做整屏重建。示例风景图也用当前配色重新生成，随主题一起转动。

界面角色映射：页面背景 `surface`；时间卡 `primaryContainer`；天气/日程卡 `surfaceContainerHigh`；消息卡 `surfaceContainerLow`；内嵌胶囊与标签 `surfaceContainerHighest`；按钮为 MD3 填充色调按钮（`secondaryContainer`，按下态叠加 12% `onSecondaryContainer`）。

`--seed-hue <角度>` 可固定种子色相并停止轮换，用于复现截图与测试。仅接受完整的有限数值，负角度及超过 360° 的值归一化到色环；拒绝 NaN、Infinity 和含尾随字符的输入。

## 显示与触摸

- 屏幕物理分辨率：720×1280。
- UI 逻辑分辨率：1280×720，横屏布局。
- 安装方向：支持 90° 或 270°，最终以屏幕摆放方向确定。
- UI 始终使用逻辑坐标，平台层负责逻辑画面到物理屏幕的映射。
- 触摸先依据设备 ABS 范围归一化，再映射到 UI 坐标；支持轴交换与反向，并明确变换由系统、驱动或应用中的哪一层承担，避免重复处理。
- 首个设备测试页面包含四角目标、中心目标与拖动轨迹；校验点击和拖动均准确。

优先评估 DRM/KMS + evdev，无桌面环境直接运行；如果设备仅提供可用 framebuffer，则采用 fbdev + evdev。桌面开发使用 SDL。DRM 的可用性、像素格式、旋转能力与设备内核相关，应在设备上实测；若硬件旋转不可用，评估软件旋转及其耗时后决定后端。不要同时在系统和应用中旋转画面。

## 技术选型

使用 C++17 + CMake，调用 LVGL 的 C API。选择 LVGL 9.x 的固定发布版本，并固定依赖提交；具体版本在显示后端验证后确定。初期使用软件绘制和局部刷新，按实际 CPU 占用、内存和刷新耗时决定是否增加硬件加速。

保留现有 Flutter 项目的数据协议与服务接口，只替换显示端。第一版先用 mock 数据打通 UI，再实现一个真实数据源，避免将所有后端迁移与显示适配绑在一起。

## 模块结构

以下为计划目录，随实际实现逐步创建：

```text
CMakeLists.txt
cmake/                    # 工具链与依赖配置
config/                   # 模拟器、设备配置示例
src/
  main.cpp                # 启动与退出
  app/                    # 生命周期、调度、事件队列
  platform/               # 显示、输入、时钟、背光抽象
    sdl/                  # 桌面模拟器
    linux/                # DRM/fbdev、evdev、背光
  model/                  # PanelState、数据有效性与更新时间
  services/               # 数据提供者、网络请求、重连与缓存
  ui/
    screens/              # 主面板、设置、硬件诊断
    components/           # 信息卡片、状态栏、提示
    theme/                # 字体、颜色、间距与样式
assets/                   # 字体、图标与资源来源/许可
tests/                    # 状态、坐标变换、数据解析
deploy/                   # systemd 单元与安装说明
docs/                     # 设备记录与验证结果
```

数据流：数据提供者 → 有界事件队列 → UI 线程更新 PanelState → 对应组件刷新。
服务层不得持有 LVGL 对象；组件不直接访问网络或设备节点。低频指标合并更新，避免队列积压；错误、离线、过期数据应能在状态模型中表达。

主线程处理 LVGL 定时器、输入及状态更新，使用单调时钟并按下次任务时间等待，避免忙循环。后台工作线程只承担需要的 I/O，设置超时，并在退出时取消任务和回收资源。

## 首版界面

横屏采用顶部状态栏、中部卡片区和简洁的底部操作区。优先显示时钟、连接状态及用户选定的少量核心信息；预留天气、设备指标或其他服务卡片，但首版不要求全部实现。

布局集中管理尺寸、间距和字体，不在业务代码中散落坐标。触摸目标初步按至少 48 逻辑像素设计，并在实际屏幕上确认可点性。中文字体明确字符覆盖与资源体积；数据变化时刷新相关组件，避免持续重建整屏。离线时保留最近数据，并显示更新时间及过期状态。

## 配置与部署

配置覆盖显示后端、设备节点、物理模式、旋转方向、触摸变换、数据源地址、刷新间隔、日志级别和背光路径。启动时验证配置，失败时输出清晰原因。

目标设备使用 systemd 启动并配置失败重启，日志进入 journald。配置、缓存与程序资源分开存放；使用专用用户及必要的设备访问权限。部署时检查原 flutter-pi 服务或图形会话是否占用显示设备，切换步骤保留回退方式。网络未就绪时仍能打开面板，连接恢复后继续刷新。

## 实施阶段与验收

1. **设备调查**：记录板卡/SoC、Armbian 与内核版本、显示接口、DRM/fbdev 节点、触摸设备、当前 Flutter 启动方式和接口协议。
2. **最小运行**：固定 LVGL 版本，建立 CMake 与 SDL 模拟器；设备显示色块和文字，完成横屏旋转与五点触摸验证。
3. **面板骨架**：建立主题、状态模型与 mock 数据源；1280×720 界面在模拟器和设备一致，更新无明显闪烁。
4. **真实数据**：迁移一个已有数据源，验证超时、断网、过期提示和自动恢复；扩展其余卡片。
5. **设备运行**：接入背光、systemd 与部署流程；验证开机启动、服务重启、日志、退出和回退。
6. **稳定性与性能**：设备连续运行至少 24 小时，观察内存趋势、CPU、刷新耗时及触摸响应；按测量结果优化。

## 待确认

- 板卡型号、SoC、Armbian/内核版本，以及 HDMI、DSI 或其他显示接口。
- 横放时接口/线缆所在方向，以确定 90° 或 270°。
- 现有面板的核心功能与数据来源，是否已有独立后台服务。
- 是否需要视频、浏览器内容或复杂图表，这会影响渲染与进程划分。

## 参考

- [LVGL Linux 集成](https://lvgl.io/docs/open/integration/embedded_linux)
- [Linux 显示与输入驱动](https://lvgl.io/docs/open/integration/embedded_linux/drivers)
- [LVGL Linux 示例项目](https://github.com/lvgl/lv_port_linux)
