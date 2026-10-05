# LVGL 子模块

动态配色另使用官方 Material Color Utilities 的 C++ 最小源码集合，提交固定为 `5b3618b16fdc3825e21d5679bafd144662088ea1`。它以源码副本存放于 `third_party/material-color-utilities/`，来源、许可证与本地改动见该目录的 `UPSTREAM.md`。仅 `HexFromArgb` 的字符串格式化替换为标准库，以避免新增 Abseil 依赖；色彩算法未修改。构建不读取 `refer/`。

依赖路径为 `third_party/lvgl`，目标版本为正式发布的 `v9.6.0`。
使用固定提交，不跟随 master。

当前主仓库索引已记录 v9.6.0 的子模块指针，本地也已完成初始化。首次从远端克隆主仓库时：

```bash
git submodule update --init --recursive
```

主仓库提交子模块指针后，其他设备拉取项目时使用：

```bash
git submodule update --init --recursive
```

核对版本与固定提交：

```bash
git -C third_party/lvgl describe --tags --exact-match
git submodule status
```

预期标签为 `v9.6.0`。不要将 `third_party/lvgl` 加入 `.gitignore`，主仓库需要记录它的子模块提交指针。
