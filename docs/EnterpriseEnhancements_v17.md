# ApexRoboticsStudio v1.7.0 Enhancements

This round strengthens three high-value product lines:

1. **Robot description inspection and diagnostics**
   - New `inspect-robot-description` CLI command
   - New HTML/JSON inspection reports
   - Counts package dependencies, mesh resources, resolved meshes, missing meshes, and duplicate mesh URIs

2. **ROS2 / MoveIt export completeness**
   - Description package now includes `config/resource_manifest.json`
   - Workspace root now includes `dependency_catalog.md` and `workspace_doctor.py`
   - MoveIt package now includes `launch/full_stack_demo.launch.py`

3. **Qt workstation source enhancement**
   - New workbench action for robot-description inspection export
   - Artifact pane and status flow align with the new backend reporting path

The v1.7 backend command chain was validated locally in Linux Debug build mode.
