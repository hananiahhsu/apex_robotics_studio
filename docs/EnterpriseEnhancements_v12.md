# ApexRoboticsStudio v1.2.0 Enhancements

This release upgrades the platform in three major directions:

1. More complete robot description import pipeline
   - URDF import remains supported.
   - Xacro input is now supported through a hybrid strategy:
     - prefer an external `xacro` executable when available;
     - otherwise fall back to an internal lightweight expander that supports a practical subset needed for interview/demo projects.
   - Mesh references are discovered from URDF/Xacro visual and collision geometry tags and carried into the `.arsproject` model.
   - Imported projects now persist robot-description provenance and mesh-resource metadata.

2. ROS 2 / MoveIt integration skeleton export
   - Export a ROS 2 workspace skeleton containing:
     - a bringup package;
     - a bridge node source file;
     - a MoveIt config package skeleton;
     - launch and config files.
   - The export is designed as a product-facing integration handoff, not as a fake binary package.

3. Stronger Qt workstation frontend source
   - The optional Qt workbench is no longer only a UI shell.
   - The frontend source now connects to backend project loading, URDF/Xacro import, project tree generation, dashboard export, job simulation, and ROS 2 workspace export.

## What was actually validated locally

Validated in the current Linux container:

- CMake configure
- CMake build
- CTest
- `import-robot-description` on a sample Xacro project
- `export-ros2-workspace` on the imported project
- `project-tree` on the imported project
- install
- CPack package generation

## What was not validated here

Not validated in the current container:

- Qt frontend compilation and execution, because Qt SDK is not installed here
- Building the generated ROS 2 workspace, because ROS 2 / MoveIt toolchains are not installed here
- Windows native build
- Cross-target runtime verification for Linux -> Windows or Linux -> ARM64
