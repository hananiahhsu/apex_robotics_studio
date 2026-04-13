cmake_minimum_required(VERSION 3.16)
project(demo_painter_cell_imported_project_description()

find_package(ament_cmake REQUIRED)

install(DIRECTORY urdf meshes launch rviz config DESTINATION share/${PROJECT_NAME})

ament_package()
