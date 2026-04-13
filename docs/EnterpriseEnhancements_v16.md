# ApexRoboticsStudio v1.6.0 Enhancements

This release deepens three high-value capabilities:

1. More complete robot description import
2. Stronger ROS2 / MoveIt workspace export
3. Better-connected Qt workstation frontend source

## Robot description import

New or strengthened behaviors in v1.6:

- Persist robot description package dependencies into `.arsproject`
- Continue resolving `package://` mesh URIs and `package.xml` package discovery
- Support `$(eval ...)` in the lightweight Xacro path used by the importer tests
- Surface description dependencies in the project catalog tree

The imported project now records:

- `source_path`
- `expanded_description_path`
- `source_kind`
- `package_dependencies`

## ROS2 / MoveIt export

The generated workspace now includes additional product-grade artifacts:

- `config/package_dependencies.yaml` in the description package
- root-level `workspace_manifest.json`
- optional `*_moveit_task_demo` package
- `apex_mtc_demo.cpp`
- `launch/mtc_demo.launch.py`
- `config/task_parameters.yaml`

This moves the export from a simple bringup/config skeleton closer to a real handoff workspace.

## Qt workstation source

The optional Qt frontend source remains gated behind `ARS_ENABLE_QT_FRONTEND`, but the source now consumes more of the backend state:

- description package count is reflected in the status line
- artifact panel now shows source kind, mesh count, and package dependency count
- the frontend source remains wired to import, analysis, dashboard, session, integration, gate, snapshot, delivery dossier, and ROS2 export actions

## Validation summary

Validated in the current Linux container:

- CMake configure
- Debug build of `apex_robotics_studio_tests`
- Debug build of `apex_robotics_studio`
- `ctest --output-on-failure`
- `import-robot-description`
- `project-tree`
- `export-ros2-workspace`
- `cmake --install`
- `cpack`

Not validated in the current container:

- Qt frontend compile/run
- exported ROS2 workspace compile/run in a real ROS2 + MoveIt environment
- Windows native build/run
- cross-compiled artifact runtime validation
