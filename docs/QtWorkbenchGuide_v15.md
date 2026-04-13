# Qt Workbench Guide v1.5

## What changed in v1.5
The optional Qt workbench source was extended beyond project viewing and basic export.

Additional source-level actions now include:
- Export Integration Package
- Evaluate Release Gate
- Create Snapshot

Existing source-level actions remain:
- Open `.arsproject`
- Import `URDF/Xacro`
- Save project
- Run analysis
- Export SVG
- Export dashboard
- Export session
- Export ROS2 / MoveIt workspace
- Export delivery dossier

## Important boundary
The Qt workbench still was **not compiled or launched in the current environment**, because this environment does not contain a Qt SDK.

So the honest status is:
- Qt workbench source continues to grow in a product direction
- its backend actions are grounded in already-validated non-Qt libraries
- runtime verification of the desktop frontend still needs a Qt-enabled machine
