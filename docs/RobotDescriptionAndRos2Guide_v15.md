# Robot Description and ROS2 / MoveIt Guide v1.5

## 1. Import a Xacro-based robot description

```bash
./out/build/v15dbg/apex_robotics_studio import-robot-description \
  samples/xacro/demo_painter_cell_v15.xacro \
  samples/projects/imported_from_xacro_v15.arsproject
```

This v1.5 sample exercises:
- package discovery via `package.xml`
- `$(find demo_support)`
- `package://demo_support/...`
- `$(optenv ...)`
- `$(env ...)`
- `xacro:if`
- `xacro:unless`

Generated artifacts:
- `samples/projects/imported_from_xacro_v15.arsproject`
- `samples/projects/imported_from_xacro_v15.expanded.urdf`

## 2. Inspect the imported project tree

```bash
./out/build/v15dbg/apex_robotics_studio project-tree \
  samples/projects/imported_from_xacro_v15.arsproject
```

## 3. Export a ROS2 / MoveIt workspace skeleton

```bash
./out/build/v15dbg/apex_robotics_studio export-ros2-workspace \
  samples/projects/imported_from_xacro_v15.arsproject \
  samples/ros2/generated_v15
```

The generated workspace contains:
- `apexpainterv15_imported_project_description`
- `apexpainterv15_imported_project_bringup`
- `apexpainterv15_imported_project_moveit_config`

Key files to inspect:
- `samples/ros2/generated_v15/apex_ros2_ws/colcon.meta`
- `samples/ros2/generated_v15/apex_ros2_ws/src/apexpainterv15_imported_project_description/source/demo_painter_cell_v15.xacro`
- `samples/ros2/generated_v15/apex_ros2_ws/src/apexpainterv15_imported_project_description/launch/controllers.launch.py`
- `samples/ros2/generated_v15/apex_ros2_ws/src/apexpainterv15_imported_project_moveit_config/launch/move_group.launch.py`
- `samples/ros2/generated_v15/apex_ros2_ws/src/apexpainterv15_imported_project_moveit_config/config/moveit_cpp.yaml`
- `samples/ros2/generated_v15/apex_ros2_ws/src/apexpainterv15_imported_project_moveit_config/config/planning_scene_monitor_parameters.yaml`

## 4. Current boundary
The workspace export itself is validated here. Building and running that generated workspace still needs a host that has ROS2 and MoveIt installed.
