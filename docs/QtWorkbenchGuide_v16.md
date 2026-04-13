# Qt Workbench Guide v1.6

## What changed in v1.6

The optional Qt workstation source was pushed further toward a real desktop workflow:

- status bar now includes robot description package dependency count
- artifact pane shows imported source kind, mesh count, and description package count
- existing actions remain wired for:
  - open/save project
  - import URDF/Xacro
  - run analysis
  - export SVG
  - export dashboard
  - export session
  - export ROS2 / MoveIt workspace
  - export integration package
  - evaluate release gate
  - create snapshot
  - export delivery dossier

## Build flag

```bash
cmake -S . -B out/build/qt -DARS_ENABLE_QT_FRONTEND=ON
```

## Current boundary

The Qt source has been enhanced, but Qt was not available in the current container, so the frontend was not compiled or run here.

That means v1.6 improves the desktop product source, but not its runtime validation status.
