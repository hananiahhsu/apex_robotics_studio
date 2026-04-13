#include <moveit/move_group_interface/move_group_interface.h>
#include <moveit/planning_scene_interface/planning_scene_interface.hpp>
#include <moveit_msgs/msg/collision_object.hpp>
#include <geometry_msgs/msg/pose.hpp>
#include <rclcpp/rclcpp.hpp>

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<rclcpp::Node>("apex_moveit_smoke_test");

  moveit::planning_interface::MoveGroupInterface move_group(node, "manipulator");
  moveit::planning_interface::PlanningSceneInterface planning_scene_interface;

  moveit_msgs::msg::CollisionObject collision_object;
  collision_object.id = "apex_fixture";
  collision_object.header.frame_id = move_group.getPlanningFrame();
  planning_scene_interface.applyCollisionObject(collision_object);

  move_group.setPlanningTime(5.0);
  move_group.setGoalTolerance(0.01);
  move_group.setNamedTarget("ready");

  moveit::planning_interface::MoveGroupInterface::Plan plan;
  const bool success = static_cast<bool>(move_group.plan(plan));
  RCLCPP_INFO(node->get_logger(), "MoveIt smoke test plan result: %s", success ? "SUCCESS" : "FAILED");

  rclcpp::shutdown();
  return success ? 0 : 1;
}
