# Robot Description Import and ROS 2 / MoveIt Export Guide

## 1. Import a robot description

URDF:

```bash
apex_robotics_studio import-robot-description robot.urdf imported_robot.arsproject
```

Xacro:

```bash
apex_robotics_studio import-robot-description robot.xacro imported_robot.arsproject
```

Behavior:

- Detects URDF vs Xacro from file extension and content.
- Attempts to expand Xacro using an external `xacro` executable when available.
- Falls back to an internal expander for a practical subset of Xacro features.
- Captures mesh paths referenced by the robot description.
- Writes an expanded sidecar `.expanded.urdf` next to the output project when the input is Xacro.
- Ensures the generated project includes a default job so the rest of the platform can immediately simulate and report on it.

## 2. Export a ROS 2 / MoveIt skeleton workspace

```bash
apex_robotics_studio export-ros2-workspace imported_robot.arsproject out/ros2_export
```

Expected result:

```text
out/ros2_export/apex_ros2_ws/
  README.md
  src/
    apex_workcell_bringup/
    apex_workcell_moveit_config/
```

## 3. Generated package contents

### Bringup package

- `package.xml`
- `CMakeLists.txt`
- `config/project.yaml`
- `launch/bridge.launch.py`
- `src/apex_project_bridge_node.cpp`

### MoveIt config skeleton

- `package.xml`
- `CMakeLists.txt`
- `config/joint_limits.yaml`
- `config/kinematics.yaml`
- `config/ompl_planning.yaml`
- `srdf/apex_generated.srdf`
- `launch/demo_moveit.launch.py`
- `src/apex_moveit_smoke_test.cpp`

## 4. Notes on scope

This exporter intentionally produces a product-integration skeleton rather than claiming to emit a fully production-ready ROS 2 / MoveIt workspace for arbitrary industrial robots. The goal is to provide:

- a clear integration contract;
- realistic package layout;
- source files that interviewers can inspect;
- a starting point for a team to continue against real robot drivers, TF trees, planning scenes, and MoveIt runtime services.
