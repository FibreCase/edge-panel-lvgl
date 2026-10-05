# ARM64 / Clang 构建

支持 Linux 开发机交叉编译到 Armbian ARM64，也支持直接在 ARM64 设备上编译。
`clang-native` 默认构建 SDL 模拟器，运行需要图形会话。设备直接显示请使用 `arm64-drm-native`，详见 [DRM 设备说明](drm-device.md)。

## 直接在 ARM64 设备编译

Debian/Ubuntu/Armbian 安装依赖：

```bash
sudo apt install clang cmake ninja-build pkg-config libsdl2-dev libfreetype6-dev fonts-noto-cjk
git submodule update --init --recursive
cmake --preset clang-native
cmake --build --preset clang-native --parallel
file build/clang-native/edge-panel
./build/clang-native/edge-panel
```

此 preset 使用当前机器的架构，ARM64 设备输出 ARM64；x86_64 开发机输出 x86_64。

## 开发机交叉编译

宿主机需要 Clang、Clang++、LLD (`ld.lld`)、CMake >= 3.20、Ninja 和 pkg-config。
Debian/Ubuntu 宿主机可安装 `clang lld cmake ninja-build pkg-config`；其他发行版安装对应包。

准备与目标 Armbian 用户空间匹配的 sysroot，至少包含目标系统的 libc 开发文件、GCC runtime/libstdc++ 开发文件、SDL2、FreeType 及其开发元数据和依赖。普通运行时 rootfs 往往缺少头文件及链接时需要的文件。

建议先在设备安装：

```bash
sudo apt install g++ libc6-dev libsdl2-dev libfreetype6-dev fonts-noto-cjk
```

再将设备的 `/usr`、`/lib` 和必要的动态链接器目录复制到开发机的独立 sysroot，例如 `/opt/sysroots/armbian-arm64`。保留目录结构及权限，并检查绝对符号链接不会指向宿主机目录；Debian usr-merge 系统需保留 `/lib` 到 `/usr/lib` 等关系。也可以使用对应系统构建工具提供的 SDK/sysroot。

```bash
export EDGE_PANEL_SYSROOT=/opt/sysroots/armbian-arm64
cmake --preset arm64-clang
cmake --build --preset arm64-clang --parallel
file build/arm64-clang/edge-panel
readelf -h build/arm64-clang/edge-panel
```

产物为 `build/arm64-clang/edge-panel`，预期 ELF64 / AArch64。它使用目标系统的动态库；复制单个程序后，目标设备仍需安装 SDL2、FreeType 和字体等运行依赖。

工具链默认在 `${EDGE_PANEL_SYSROOT}/usr` 查找 GCC runtime 和 libstdc++。若使用单独的 ARM64 GNU 工具链，可设置其安装前缀：

```bash
export EDGE_PANEL_GCC_TOOLCHAIN=/opt/toolchains/aarch64-linux-gnu
cmake --preset arm64-clang
```

仍使用 Clang 编译；GNU 工具链只提供 C++ 标准库、头文件和启动文件。缺少 `crt*.o`、`-lstdc++` 或标准头文件通常意味着这些开发文件缺失或前缀不匹配，不应通过关闭编译器链接检查绕过。

也可用 `-DEDGE_PANEL_SYSROOT=/absolute/path`、`-DEDGE_PANEL_GCC_TOOLCHAIN=/absolute/prefix` 指定路径。使用非默认版本的 LLVM 时可传 `-DEDGE_PANEL_CLANG=/path/clang`、`-DEDGE_PANEL_CLANGXX=/path/clang++`、`-DEDGE_PANEL_LLD=/path/ld.lld`。

换 sysroot、编译器或目标工具链后使用新的构建目录，避免 CMake 缓存混用依赖。`PKG_CONFIG_PATH` 在工具链中被清空，pkg-config 只搜索 sysroot 中的 ARM64 `.pc` 文件；交叉构建默认不构建或运行测试程序。

原生模拟器验证：

```bash
ctest --preset clang-native
```

参考：[Clang 交叉编译](https://clang.llvm.org/docs/CrossCompilation.html)、[CMake 工具链](https://cmake.org/cmake/help/v3.31/manual/cmake-toolchains.7.html)。
