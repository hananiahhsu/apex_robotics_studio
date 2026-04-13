cmake_minimum_required(VERSION 3.16)
project(apexpainterv16_imported_project_moveit_config()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(moveit_ros_planning_interface REQUIRED)
find_package(moveit_visual_tools REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(moveit_msgs REQUIRED)
find_package(shape_msgs REQUIRED)

add_executable(apex_moveit_smoke_test src/apex_moveit_smoke_test.cpp)
ament_target_dependencies(apex_moveit_smoke_test rclcpp moveit_ros_planning_interface moveit_visual_tools geometry_msgs moveit_msgs shape_msgs)

target_compile_features(apex_moveit_smoke_test PUBLIC cxx_std_17)

add_executable(apex_scene_loader_node src/apex_scene_loader_node.cpp)
ament_target_dependencies(apex_scene_loader_node rclcpp moveit_ros_planning_interface geometry_msgs moveit_msgs shape_msgs)

target_compile_features(apex_scene_loader_node PUBLIC cxx_std_17)

install(TARGETS apex_moveit_smoke_test apex_scene_loader_node DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY config launch srdf rviz DESTINATION share/${PROJECT_NAME})

ament_package()
