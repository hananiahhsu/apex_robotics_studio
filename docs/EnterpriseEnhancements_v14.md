# ApexRoboticsStudio v1.4.0 Enhancements

This round focuses on making the robot-description ingest and ROS2/MoveIt export path substantially closer to a real robotics software workflow.

## What was strengthened

### 1. URDF/Xacro/package/mesh ingest

The importer now handles more ROS-style description patterns:

- `package.xml`-based package discovery, not only `share/<pkg>` style lookup
- `$(env VAR)` token expansion
- `$(optenv VAR default)` token expansion
- `$(dirname)` token expansion for local mesh and include paths
- `package://...` mesh URI resolution through discovered package roots
- `$(find package_name)` include and resource expansion across ROS package roots

This materially improves the ability to ingest real-world robot-description trees exported from ROS workspaces.

### 2. Stronger ROS2 / MoveIt export skeleton

The exported workspace now generates three coordinated packages:

- `<stem>_description`
- `<stem>_bringup`
- `<stem>_moveit_config`

The MoveIt package was expanded with:

- `config/scene_objects.yaml`
- `src/apex_scene_loader_node.cpp`
- `rviz/moveit_demo.rviz`
- `launch/demo_moveit.launch.py` that starts the scene loader and RViz

This means the export is no longer only a `MoveGroupInterface` smoke test. It now includes a generated planning-scene obstacle bootstrap path for the workcell obstacles already present in the Apex project.

### 3. Qt workbench source upgrade

The optional Qt workstation source was enhanced to better reflect a real desktop workbench direction:

- mesh catalog table in the main window
- artifact viewer panel
- delivery dossier export action
- tighter linkage between imported projects and exported artifacts

This remains a source-level enhancement in this environment because the container does not include a Qt SDK.

## What was validated locally

- debug configure/build completed for the CLI and test executable
- all 35 tests passed
- Xacro import was run on a package-aware sample with `$(find)`, `package://...`, and `$(dirname)`
- ROS2 workspace export was run on the imported project and generated the three ROS packages plus scene loader and scene YAML
- install and CPack package generation succeeded

## What remains environment-dependent

- building and running the Qt desktop executable still requires Qt5/Qt6 on the target machine
- compiling the exported ROS2 workspace still requires a real ROS2 / MoveIt environment
- Windows and cross-compiled runtime verification were not repeated in this container
