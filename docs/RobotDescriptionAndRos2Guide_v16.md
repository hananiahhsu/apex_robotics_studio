# Robot Description and ROS2 / MoveIt Guide v1.6

## Import a Xacro robot description

```bash
./out/build/v16/apex_robotics_studio import-robot-description   samples/xacro/demo_painter_cell_v16.xacro   samples/projects/imported_from_xacro_v16.arsproject
```

What this v1.6 sample exercises:

- package-aware Xacro include resolution
- `package://` mesh capture
- package dependency catalog persistence
- generated expanded URDF sidecar

Generated artifacts:

- `samples/projects/imported_from_xacro_v16.arsproject`
- `samples/projects/imported_from_xacro_v16.expanded.urdf`

## Inspect the imported project tree

```bash
./out/build/v16/apex_robotics_studio project-tree   samples/projects/imported_from_xacro_v16.arsproject
```

The catalog now includes a `Description Packages` node when package dependencies are present.

## Export a ROS2 / MoveIt workspace

```bash
./out/build/v16/apex_robotics_studio export-ros2-workspace   samples/projects/imported_from_xacro_v16.arsproject   samples/ros2/generated_v16
```

Generated package families:

- `apexpainterv16_imported_project_description`
- `apexpainterv16_imported_project_bringup`
- `apexpainterv16_imported_project_moveit_config`
- `apexpainterv16_imported_project_moveit_task_demo`

Key generated files:

- `samples/ros2/generated_v16/apex_ros2_ws/workspace_manifest.json`
- `samples/ros2/generated_v16/apex_ros2_ws/colcon.meta`
- `samples/ros2/generated_v16/apex_ros2_ws/src/apexpainterv16_imported_project_description/config/package_dependencies.yaml`
- `samples/ros2/generated_v16/apex_ros2_ws/src/apexpainterv16_imported_project_moveit_task_demo/src/apex_mtc_demo.cpp`
- `samples/ros2/generated_v16/apex_ros2_ws/src/apexpainterv16_imported_project_moveit_task_demo/launch/mtc_demo.launch.py`

## Current boundary

The exported workspace skeleton has been generated and inspected locally, but it has not been compiled inside a real ROS2 / MoveIt SDK environment in this container.
