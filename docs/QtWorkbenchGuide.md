# Qt Workbench Guide

## Purpose

The Qt workbench module is the desktop UI growth path for ApexRoboticsStudio.
It is intentionally optional so the core project remains zero-dependency and easy to build in CI and cross-compilation environments.

## Build option

```bash
cmake -S . -B out/build/qt -DARS_ENABLE_QT_FRONTEND=ON
cmake --build out/build/qt
```

## Dependency resolution

The build first looks for Qt 6 Widgets.
If Qt 6 is not available, it falls back to Qt 5 Widgets.

## Current scope

The current UI is a preview shell, not a full production workbench yet. It provides:

- main window
- menu / toolbar shell
- project selection action
- catalog/log panel placeholders

## Why this is still valuable

In a real company project, desktop UI and simulation platform often evolve at different speeds.
Keeping the Qt module optional gives several benefits:

- backend libraries stay portable and testable
- CI for headless targets stays simple
- cross-compiling the core does not require GUI dependencies
- desktop UX can evolve independently

## Current validation boundary

The Qt module is included as source and CMake integration, but it was not compiled in the current Linux container because Qt SDK packages are not installed here.
