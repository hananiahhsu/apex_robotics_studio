# Robot Description / ROS 2 / MoveIt Guide v1.3

## Importing a URDF or Xacro description

```bash
apex_robotics_studio import-robot-description robot.urdf imported.arsproject
apex_robotics_studio import-robot-description robot.xacro imported.arsproject
```

### Supported import behaviors

- revolute joint extraction
- `xacro:include`
- `$(arg name)` substitution
- `$(find package_name)` substitution
- `package://...` mesh URI capture and resolution
- persistence of mesh metadata into `.arsproject`

### Package search roots

The importer searches these roots:

- explicit `UrdfImportOptions::PackageSearchRoots`
- `AMENT_PREFIX_PATH`
- `COLCON_PREFIX_PATH`
- `ROS_PACKAGE_PATH`

## Exporting a ROS 2 / MoveIt workspace skeleton

```bash
apex_robotics_studio export-ros2-workspace imported.arsproject out_dir
```

Generated workspace layout:

```text
apex_ros2_ws/
  src/
    <stem>_description/
    <stem>_bringup/
    <stem>_moveit_config/
```

### Description package

Contains:

- `package.xml`
- `CMakeLists.txt`
- `urdf/<robot>.urdf`
- `launch/display.launch.py`
- `rviz/view_robot.rviz`
- `config/ros2_controllers.yaml`
- copied mesh assets under `meshes/imported/`

### Bringup package

Contains:

- `launch/bringup.launch.py`
- `config/project.yaml`
- `src/apex_project_bridge_node.cpp`

### MoveIt config package

Contains:

- `launch/demo_moveit.launch.py`
- `config/joint_limits.yaml`
- `config/kinematics.yaml`
- `config/ompl_planning.yaml`
- `config/moveit_controllers.yaml`
- `config/ros2_controllers.yaml`
- `srdf/apex_generated.srdf`
- `src/apex_moveit_smoke_test.cpp`

## v1.3 sample chain

A stronger sample chain is included in:

- `samples/robot_description_packages/`
- `samples/xacro/demo_painter_cell_v13.xacro`
- `samples/projects/imported_from_xacro_v13.arsproject`
- `samples/ros2/generated_v13/`
