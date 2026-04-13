from launch import LaunchDescription
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os

def generate_launch_description():
    description_share = get_package_share_directory('apexpainterv16_imported_project_description')
    controllers = os.path.join(description_share, 'config', 'ros2_controllers.yaml')
    return LaunchDescription([
        Node(package='controller_manager', executable='ros2_control_node', parameters=[controllers], output='screen'),
        Node(package='controller_manager', executable='spawner', arguments=['joint_state_broadcaster'], output='screen'),
        Node(package='controller_manager', executable='spawner', arguments=['arm_controller'], output='screen')
    ])
