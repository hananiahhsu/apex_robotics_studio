# Qt Workbench Guide v1.2

## Status

The Qt frontend source is present and connected to backend services, but it was not compiled in the current validation environment because Qt is not installed in the container.

## Enable the frontend

Configure with:

```bash
cmake -S . -B out/build/qt \
  -DCMAKE_BUILD_TYPE=Release \
  -DARS_ENABLE_QT_FRONTEND=ON
```

Then build:

```bash
cmake --build out/build/qt -j
```

Expected target:

- `apex_robotics_workstation_qt`

## Current frontend capabilities in source

- Open `.arsproject`
- Import URDF / Xacro into a project
- Show project tree / catalog
- Run job simulation summary
- Export dashboard HTML
- Export ROS 2 / MoveIt skeleton workspace
- Show generated HTML dashboard content in the workbench

## Dependency strategy

- Qt6 Widgets is preferred
- Qt5 Widgets is accepted as fallback
- The frontend remains optional so the backend SDK and CI pipeline can stay lightweight
