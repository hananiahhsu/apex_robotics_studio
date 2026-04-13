from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os


def generate_launch_description():
    moveit_launch = os.path.join(get_package_share_directory('apexpainterv15_imported_project_moveit_config'), 'launch', 'move_group.launch.py')
    return LaunchDescription([
        IncludeLaunchDescription(PythonLaunchDescriptionSource(moveit_launch)),
        Node(package='apexpainterv15_imported_project_moveit_task_demo', executable='apex_mtc_demo', output='screen')
    ])
