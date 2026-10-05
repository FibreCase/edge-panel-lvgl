# Linux AArch64 / Armbian, compiled using host Clang and LLD.
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(EDGE_PANEL_SYSROOT "$ENV{EDGE_PANEL_SYSROOT}" CACHE PATH "ARM64 sysroot with target development files")
if(NOT IS_ABSOLUTE "${EDGE_PANEL_SYSROOT}" OR NOT IS_DIRECTORY "${EDGE_PANEL_SYSROOT}/usr/include")
    message(FATAL_ERROR "Set EDGE_PANEL_SYSROOT to an absolute ARM64 sysroot containing usr/include")
endif()
set(CMAKE_SYSROOT "${EDGE_PANEL_SYSROOT}")
set(CMAKE_FIND_ROOT_PATH "${CMAKE_SYSROOT}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

find_program(EDGE_PANEL_CLANG NAMES clang REQUIRED)
find_program(EDGE_PANEL_CLANGXX NAMES clang++ REQUIRED)
find_program(EDGE_PANEL_LLD NAMES ld.lld REQUIRED)
set(CMAKE_C_COMPILER "${EDGE_PANEL_CLANG}")
set(CMAKE_CXX_COMPILER "${EDGE_PANEL_CLANGXX}")
set(CMAKE_ASM_COMPILER "${EDGE_PANEL_CLANG}")
# Distribution Clang config files may force a host GCC triple; ignore them.
set(CMAKE_C_COMPILER_ARG1 "--no-default-config")
set(CMAKE_CXX_COMPILER_ARG1 "--no-default-config")
set(CMAKE_ASM_COMPILER_ARG1 "--no-default-config")
set(CMAKE_C_COMPILER_TARGET aarch64-linux-gnu)
set(CMAKE_CXX_COMPILER_TARGET aarch64-linux-gnu)
set(CMAKE_ASM_COMPILER_TARGET aarch64-linux-gnu)

set(EDGE_PANEL_GCC_TOOLCHAIN "$ENV{EDGE_PANEL_GCC_TOOLCHAIN}" CACHE PATH "GCC runtime and libstdc++ prefix for target")
if(NOT EDGE_PANEL_GCC_TOOLCHAIN)
    # An Armbian sysroot with g++ installed normally supplies usr/lib/gcc
    # and usr/include/c++ under this prefix. Compilation still uses Clang.
    set(EDGE_PANEL_GCC_TOOLCHAIN "${CMAKE_SYSROOT}/usr")
endif()
set(CMAKE_C_COMPILER_EXTERNAL_TOOLCHAIN "${EDGE_PANEL_GCC_TOOLCHAIN}")
set(CMAKE_CXX_COMPILER_EXTERNAL_TOOLCHAIN "${EDGE_PANEL_GCC_TOOLCHAIN}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=${EDGE_PANEL_LLD}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=${EDGE_PANEL_LLD}")

# Build-time tools run on the host; headers, libraries and packages are target-only.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH FALSE CACHE BOOL "" FORCE)
set(ENV{PKG_CONFIG_PATH} "")
set(ENV{PKG_CONFIG_SYSROOT_DIR} "${CMAKE_SYSROOT}")
set(ENV{PKG_CONFIG_LIBDIR} "${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu/pkgconfig:${CMAKE_SYSROOT}/usr/lib/pkgconfig:${CMAKE_SYSROOT}/usr/share/pkgconfig:${CMAKE_SYSROOT}/lib/aarch64-linux-gnu/pkgconfig")
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES EDGE_PANEL_SYSROOT EDGE_PANEL_GCC_TOOLCHAIN EDGE_PANEL_CLANG EDGE_PANEL_CLANGXX EDGE_PANEL_LLD)
