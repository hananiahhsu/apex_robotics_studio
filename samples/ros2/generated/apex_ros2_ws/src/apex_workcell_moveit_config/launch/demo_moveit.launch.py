from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='apex_workcell_moveit_config',
            executable='apex_moveit_smoke_test',
            output='screen')
    ])
