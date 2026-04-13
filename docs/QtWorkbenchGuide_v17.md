# Qt Workbench Guide v1.7

## What changed in v1.7

The optional Qt workstation source now includes a dedicated **Inspect Robot Description** action.

From the workbench source perspective, the frontend can now drive these backend paths:
- Open / Save Project
- Import URDF/Xacro
- Run Analysis
- Export SVG
- Export Dashboard
- Export Session
- Export ROS2 / MoveIt Workspace
- Export Integration Package
- Evaluate Release Gate
- Create Snapshot
- Export Delivery Dossier
- Inspect Robot Description

## Validation status

The Qt source was enhanced, but **it was not compiled in the current container**, because Qt5/Qt6 SDKs are not present here.
The backend actions wired to the Qt source were validated through the CLI.
