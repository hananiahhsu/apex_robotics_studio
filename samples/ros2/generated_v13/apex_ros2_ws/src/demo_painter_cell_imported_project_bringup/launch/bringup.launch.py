from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os


def generate_launch_description():
    config = os.path.join(get_package_share_directory('demo_painter_cell_imported_project_bringup'), 'config', 'project.yaml')
    display_launch = os.path.join(get_package_share_directory('demo_painter_cell_imported_project_description'), 'launch', 'display.launch.py')
    return LaunchDescription([
        IncludeLaunchDescription(PythonLaunchDescriptionSource(display_launch)),
        Node(
            package='demo_painter_cell_imported_project_bringup',
            executable='apex_project_bridge_node',
            name='apex_project_bridge_node',
            parameters=[config],
            output='screen')
    ])
