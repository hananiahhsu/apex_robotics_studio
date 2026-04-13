#include <moveit/move_group_interface/move_group_interface.h>
#include <rclcpp/rclcpp.hpp>

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<rclcpp::Node>("apex_mtc_demo");
  moveit::planning_interface::MoveGroupInterface move_group(node, "manipulator");
  RCLCPP_INFO(node->get_logger(), "Generated MoveIt task/demo skeleton ready for integration.");
  RCLCPP_INFO(node->get_logger(), "Planning frame: %s", move_group.getPlanningFrame().c_str());
  rclcpp::shutdown();
  return 0;
}
