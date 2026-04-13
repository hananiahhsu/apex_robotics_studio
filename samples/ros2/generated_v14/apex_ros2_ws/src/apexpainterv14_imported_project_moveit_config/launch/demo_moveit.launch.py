from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os


def generate_launch_description():
    display_launch = os.path.join(get_package_share_directory('apexpainterv14_imported_project_description'), 'launch', 'display.launch.py')
    moveit_rviz = os.path.join(get_package_share_directory('apexpainterv14_imported_project_moveit_config'), 'rviz', 'moveit_demo.rviz')
    return LaunchDescription([
        IncludeLaunchDescription(PythonLaunchDescriptionSource(display_launch)),
        Node(
            package='apexpainterv14_imported_project_moveit_config',
            executable='apex_scene_loader_node',
            output='screen'),
        Node(
            package='apexpainterv14_imported_project_moveit_config',
            executable='apex_moveit_smoke_test',
            output='screen'),
        Node(
            package='rviz2',
            executable='rviz2',
            arguments=['-d', moveit_rviz],
            output='screen')
    ])
