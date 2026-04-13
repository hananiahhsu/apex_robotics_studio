# Robot Description and ROS2 / MoveIt Guide v1.7

## Import a package-aware Xacro project

```bash
export ROS_PACKAGE_PATH=$PWD/samples/robot_description_packages
./out/build/v17_debug/apex_robotics_studio import-robot-description   samples/xacro/demo_painter_cell_v17.xacro   samples/projects/imported_from_xacro_v17.arsproject
```

## Inspect robot-description assets

```bash
./out/build/v17_debug/apex_robotics_studio inspect-robot-description   samples/projects/imported_from_xacro_v17.arsproject   samples/projects/robot_description_inspection_v17.html
```

The inspection report summarizes:
- source kind and provenance
- imported package dependencies
- mesh inventory
- resolved vs missing mesh assets
- duplicate mesh URIs

## Export a richer ROS2 / MoveIt workspace

```bash
./out/build/v17_debug/apex_robotics_studio export-ros2-workspace   samples/projects/imported_from_xacro_v17.arsproject   samples/ros2/generated_v17
```

New export artifacts in v1.7 include:
- `workspace_manifest.json`
- `dependency_catalog.md`
- `workspace_doctor.py`
- `src/*_description/config/resource_manifest.json`
- `src/*_moveit_config/launch/full_stack_demo.launch.py`
