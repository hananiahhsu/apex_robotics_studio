from launch import LaunchDescription
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os


def generate_launch_description():
    config = os.path.join(get_package_share_directory('apex_workcell_bringup'), 'config', 'project.yaml')
    return LaunchDescription([
        Node(
            package='apex_workcell_bringup',
            executable='apex_project_bridge_node',
            name='apex_project_bridge_node',
            parameters=[config],
            output='screen')
    ])
