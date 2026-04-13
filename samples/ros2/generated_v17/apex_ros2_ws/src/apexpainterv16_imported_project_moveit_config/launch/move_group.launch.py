from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os

def generate_launch_description():
    moveit_share = get_package_share_directory('apexpainterv16_imported_project_moveit_config')
    description_share = get_package_share_directory('apexpainterv16_imported_project_description')
    display_launch = os.path.join(description_share, 'launch', 'display.launch.py')
    scene_yaml = os.path.join(moveit_share, 'config', 'scene_objects.yaml')
    return LaunchDescription([
        IncludeLaunchDescription(PythonLaunchDescriptionSource(display_launch)),
        Node(package='apexpainterv16_imported_project_moveit_config', executable='apex_scene_loader_node', parameters=[{'scene_yaml': scene_yaml}], output='screen'),
        Node(package='apexpainterv16_imported_project_moveit_config', executable='apex_moveit_smoke_test', output='screen')
    ])
