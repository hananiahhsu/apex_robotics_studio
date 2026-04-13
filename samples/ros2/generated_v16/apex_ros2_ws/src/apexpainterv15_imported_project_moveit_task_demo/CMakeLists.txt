cmake_minimum_required(VERSION 3.16)
project(apexpainterv15_imported_project_moveit_task_demo()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(moveit_ros_planning_interface REQUIRED)

add_executable(apex_mtc_demo src/apex_mtc_demo.cpp)
ament_target_dependencies(apex_mtc_demo rclcpp moveit_ros_planning_interface)

target_compile_features(apex_mtc_demo PUBLIC cxx_std_17)

install(TARGETS apex_mtc_demo DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY config launch DESTINATION share/${PROJECT_NAME})

ament_package()
