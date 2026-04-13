cmake_minimum_required(VERSION 3.16)
project(apexpainterv15_imported_project_bringup()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(apex_project_bridge_node src/apex_project_bridge_node.cpp)
ament_target_dependencies(apex_project_bridge_node rclcpp sensor_msgs std_msgs)

target_compile_features(apex_project_bridge_node PUBLIC cxx_std_17)

install(TARGETS apex_project_bridge_node DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY launch config DESTINATION share/${PROJECT_NAME})

ament_package()
