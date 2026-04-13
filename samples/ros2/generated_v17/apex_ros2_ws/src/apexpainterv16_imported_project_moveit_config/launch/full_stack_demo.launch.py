from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from ament_index_python.packages import get_package_share_directory
import os

def generate_launch_description():
    bringup_share = get_package_share_directory('apexpainterv16_imported_project_bringup')
    moveit_share = get_package_share_directory('apexpainterv16_imported_project_moveit_config')
    actions = [
        IncludeLaunchDescription(PythonLaunchDescriptionSource(os.path.join(bringup_share, 'launch', 'bringup.launch.py'))),
        IncludeLaunchDescription(PythonLaunchDescriptionSource(os.path.join(moveit_share, 'launch', 'bringup_with_moveit.launch.py')))
    ]
    mtc_share = get_package_share_directory('apexpainterv16_imported_project_moveit_task_demo')
    actions.append(IncludeLaunchDescription(PythonLaunchDescriptionSource(os.path.join(mtc_share, 'launch', 'mtc_demo.launch.py'))))
    return LaunchDescription(actions)
