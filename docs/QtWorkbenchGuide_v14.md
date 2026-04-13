# Qt Workbench Guide v1.4

## Current status

The Qt workstation remains optional and requires Qt5/Qt6 on the build machine.

This container does not include a Qt SDK, so the desktop executable was not compiled or run here.

## Source-level upgrades in v1.4

The workbench source now includes:

- project tree
- HTML dashboard view
- segment table
- mesh resource table
- artifact viewer pane
- import URDF/Xacro action
- save project action
- run analysis action
- export SVG action
- export dashboard action
- export session action
- export ROS2 / MoveIt workspace action
- export delivery dossier action

## Intended desktop workflow

1. Open or import a project
2. Inspect joints, obstacles, mesh catalog, and robot-description source
3. Run simulation / analysis
4. Export dashboard / session / dossier
5. Export ROS2 / MoveIt workspace for downstream robotics integration

## Honest boundary

This is now substantially closer to a real workbench source tree, but it still needs a Qt-enabled machine for compile and runtime validation.
