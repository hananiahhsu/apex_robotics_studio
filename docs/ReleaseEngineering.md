# Release Engineering Guide

## Goals

This guide defines a company-grade path for building, validating, packaging, and distributing ApexRoboticsStudio artifacts across host and cross targets.

## Recommended release stages

1. **Configure** with checked-in CMake presets.
2. **Build** with reproducible toolchain files.
3. **Test** on host and, when available, under target emulation for ARM64.
4. **Install** into a staging prefix.
5. **Package** with CPack.
6. **Publish** build logs, package checksums, and release notes.

## Local package creation on Linux

```bash
./scripts/package_linux.sh
```

Expected generators:
- `TGZ`
- `ZIP`

## Containerized builds

Build images:

```bash
docker build -f docker/linux-x64-dev.Dockerfile -t ars-linux-dev .
docker build -f docker/linux-aarch64-cross.Dockerfile -t ars-linux-aarch64-cross .
docker build -f docker/linux-mingw-cross.Dockerfile -t ars-linux-mingw-cross .
```

Run a build inside a container:

```bash
./scripts/build_in_container.sh ars-linux-dev ./scripts/build_linux.sh
./scripts/build_in_container.sh ars-linux-aarch64-cross ./scripts/build_cross_linux_aarch64.sh
./scripts/build_in_container.sh ars-linux-mingw-cross ./scripts/build_cross_windows_mingw.sh
```

## Release artifact checklist

- Versioned binaries
- Headers and CMake package config
- Sample `.arsproject` files
- Architecture / interview documentation
- Checksums
- CI logs
- Toolchain provenance

## Packaging note

Current packaging is oriented toward interview/demo delivery using archive packages. Commercial shipment would usually add:
- installer branding
- code signing
- SBOM generation
- license bundle
- third-party notices
- vulnerability scanning
- artifact retention policy
