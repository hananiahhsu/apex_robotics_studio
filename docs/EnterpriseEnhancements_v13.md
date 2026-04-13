# ApexRoboticsStudio v1.3.0 Enhancements

This version strengthens three product directions:

1. **Robot description ingest depth**
   - `xacro` internal expansion now supports `$(arg ...)` and `$(find ...)`
   - package root discovery checks explicit search roots plus `AMENT_PREFIX_PATH`, `COLCON_PREFIX_PATH`, and `ROS_PACKAGE_PATH`
   - `package://...` mesh URIs are resolved to concrete files and persisted in `.arsproject`

2. **ROS 2 / MoveIt export depth**
   - generated workspace now contains:
     - `<stem>_description`
     - `<stem>_bringup`
     - `<stem>_moveit_config`
   - exported description package contains copied mesh assets, rewritten mesh URIs, a display launch file, RViz config, and a ros2_control skeleton config
   - generated MoveIt config contains `MoveGroupInterface` and `PlanningSceneInterface` smoke-test sources

3. **Qt workbench source upgrade**
   - optional Qt frontend source now includes actions for:
     - open/import
     - save project
     - run analysis
     - export SVG
     - export dashboard
     - export session artifacts
     - export ROS 2 / MoveIt workspace

## Validation summary

Validated in the current Linux environment:

- CMake configure
- targeted build of libraries, tests, and main CLI
- full `ctest` pass (34 tests)
- CLI import of a sample `xacro`
- CLI export of a ROS 2 / MoveIt workspace
- install and CPack packaging

Not validated in the current environment:

- Qt frontend binary build and runtime (Qt SDK not installed here)
- compiling the exported ROS 2 workspace in a real ROS 2 / MoveIt environment
- Windows native builds
- runtime testing of cross-compiled artifacts
