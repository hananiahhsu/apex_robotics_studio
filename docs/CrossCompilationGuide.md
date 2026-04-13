# Cross-Compilation Guide

## 1. Why this repo treats cross-compilation as a first-class concern

In real robotics businesses, the build host and the delivery target are often different. A common pattern is:

- developers build on Linux x64,
- deploy runtime components to Linux ARM64 edge devices,
- and deliver desktop engineering tools to Windows x64.

That makes cross-compilation part of the product architecture, not an afterthought.

## 2. Targets supported by this repository

### 2.1 Host builds
- Linux x64 -> Linux x64
- Windows x64 -> Windows x64

### 2.2 Cross builds
- Linux x64 -> Linux ARM64 (`aarch64`)
- Linux x64 -> Windows x64 (`mingw-w64`)

## 3. Repository assets that implement the cross-build system

- `cmake/toolchains/linux-gcc-aarch64.cmake`
- `cmake/toolchains/linux-mingw-w64-x86_64.cmake`
- `CMakePresets.json`
- `CMakeUserPresets.json.example`
- `scripts/bootstrap_ubuntu_cross_toolchains.sh`
- `scripts/build_cross_linux_aarch64.sh`
- `scripts/build_cross_windows_mingw.sh`
- `vcpkg/triplets/apex-linux-arm64.cmake`
- `vcpkg/triplets/apex-mingw-x64.cmake`

## 4. Best-practice rules used here

### Rule 1: keep target knowledge in toolchain files
Do not spread compiler paths, sysroot logic, and search rules across random shell scripts.

### Rule 2: keep presets as the team-facing entry points
Humans and CI should call stable presets rather than retyping long configure commands.

### Rule 3: separate host tools from target artifacts
Executables that must run during build belong to the host. Headers, libraries, and packages for the deliverable belong to the target.

### Rule 4: make install trees consumable by downstream products
A project is more valuable when another CMake product can `find_package()` it directly.

### Rule 5: make machine-specific paths local overrides
Team-wide presets are committed. Machine-specific roots and sysroots belong in `CMakeUserPresets.json` or environment variables.

## 5. Linux x64 -> Linux ARM64

### 5.1 Required toolchain components
Typical Ubuntu setup:

- `gcc-aarch64-linux-gnu`
- `g++-aarch64-linux-gnu`
- `libc6-dev-arm64-cross`
- `qemu-user-static` if you want to run tests on the host through emulation

The repository includes:

```bash
sudo ./scripts/bootstrap_ubuntu_cross_toolchains.sh
```

### 5.2 Required environment variables
At minimum:

```bash
export ARS_AARCH64_SYSROOT=/usr/aarch64-linux-gnu
```

Optional, for tests through QEMU:

```bash
export ARS_CROSS_EMULATOR="/usr/bin/qemu-aarch64-static;-L;/usr/aarch64-linux-gnu"
```

Optional, if your GCC cross toolchain is not on PATH:

```bash
export ARS_AARCH64_GCC_ROOT=/opt/toolchains/gcc-aarch64
```

### 5.3 Build command

```bash
./scripts/build_cross_linux_aarch64.sh
```

### 5.4 What the toolchain file does
`cmake/toolchains/linux-gcc-aarch64.cmake` configures:

- target system name = `Linux`
- target processor = `aarch64`
- cross compilers = `aarch64-linux-gnu-*`
- optional `CMAKE_SYSROOT`
- `CMAKE_FIND_ROOT_PATH_MODE_*` rules so headers/libraries/packages come from target roots, while build-time executables are resolved on the host
- optional `CMAKE_CROSSCOMPILING_EMULATOR`

## 6. Linux x64 -> Windows x64

### 6.1 Required toolchain components
Typical Ubuntu setup:

- `binutils-mingw-w64-x86-64`
- `gcc-mingw-w64-x86-64`
- `g++-mingw-w64-x86-64`

The repository includes the same bootstrap helper for that setup.

### 6.2 Optional environment variables
If MinGW is not already on PATH:

```bash
export ARS_MINGW_ROOT=/opt/mingw-w64
```

If you maintain a dedicated MinGW sysroot:

```bash
export ARS_MINGW_SYSROOT=/opt/mingw-w64/x86_64-w64-mingw32
```

### 6.3 Build command

```bash
./scripts/build_cross_windows_mingw.sh
```

### 6.4 What the toolchain file does
`cmake/toolchains/linux-mingw-w64-x86_64.cmake` configures:

- target system name = `Windows`
- target processor = `x86_64`
- cross compilers = `x86_64-w64-mingw32-*`
- target search roots for libraries/includes/packages
- host-only search behavior for build-time programs

## 7. Using presets directly

List presets:

```bash
cmake --list-presets
```

Configure/build/test/install examples:

```bash
cmake --preset linux-aarch64-release
cmake --build --preset build-linux-aarch64-release
ctest --preset test-linux-aarch64-release
cmake --install out/build/linux-aarch64-release
```

```bash
cmake --preset linux-to-windows-x64-release
cmake --build --preset build-linux-to-windows-x64-release
cmake --install out/build/linux-to-windows-x64-release
```

## 8. Machine-specific override pattern

Do **not** edit the checked-in preset for your local machine.

Instead:

1. copy `CMakeUserPresets.json.example` to `CMakeUserPresets.json`
2. fill in local toolchain and sysroot paths
3. keep that file untracked

This keeps the team preset stable while still allowing developer-specific environments.

## 9. vcpkg integration path for future dependencies

When you later introduce third-party dependencies, use the repository overlay triplets:

- `vcpkg/triplets/apex-linux-arm64.cmake`
- `vcpkg/triplets/apex-mingw-x64.cmake`

Example:

```bash
cmake --preset linux-aarch64-release \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_OVERLAY_TRIPLETS="$PWD/vcpkg/triplets" \
  -DVCPKG_TARGET_TRIPLET=apex-linux-arm64
```

The triplet then chainloads the repository toolchain file, so the target platform definition and the package-manager target definition stay aligned.

## 10. CI implementation strategy in this repository

The GitHub Actions workflow now contains four jobs:

- Linux host build
- Windows host build
- Linux x64 -> Linux ARM64 cross build
- Linux x64 -> Windows x64 cross build

That is a strong baseline for a product that will later add GUI layers and external dependencies.

## 11. Recommended enterprise extensions

### 11.1 Binary cache and mirrors
When third-party dependencies grow, introduce a binary cache and mirror strategy to avoid network instability in CI.

### 11.2 Sysroot governance
Treat sysroots as versioned delivery inputs. Do not let every developer use arbitrary target root contents.

### 11.3 Release promotion
Separate pull-request builds from release builds. Release builds should tag toolchain version, sysroot version, and dependency lock state.

### 11.4 Artifact naming
Name artifacts by target and configuration, for example:

- `apex-robotics-studio-linux-x64-release`
- `apex-robotics-studio-linux-aarch64-release`
- `apex-robotics-studio-windows-x64-release`

### 11.5 Downstream SDK validation
Regularly validate that another CMake project can consume the installed package with `find_package(ApexRoboticsStudio CONFIG REQUIRED)`.
