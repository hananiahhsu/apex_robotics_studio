from launch import LaunchDescription
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os


def generate_launch_description():
    share = get_package_share_directory('apexpainterv15_imported_project_description')
    urdf_path = os.path.join(share, 'urdf', 'apexpainterv15.urdf')
    rviz_path = os.path.join(share, 'rviz', 'view_robot.rviz')
    with open(urdf_path, 'r', encoding='utf-8') as stream:
        robot_description = stream.read()
    return LaunchDescription([
        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            parameters=[{'robot_description': robot_description}],
            output='screen'),
        Node(
            package='joint_state_publisher_gui',
            executable='joint_state_publisher_gui',
            output='screen'),
        Node(
            package='rviz2',
            executable='rviz2',
            arguments=['-d', rviz_path],
            output='screen')
    ])
