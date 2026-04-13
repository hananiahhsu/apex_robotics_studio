#include <moveit/planning_scene_interface/planning_scene_interface.hpp>
#include <moveit_msgs/msg/collision_object.hpp>
#include <shape_msgs/msg/solid_primitive.hpp>
#include <geometry_msgs/msg/pose.hpp>
#include <rclcpp/rclcpp.hpp>

#include <vector>

class ApexSceneLoaderNode final : public rclcpp::Node
{
public:
  ApexSceneLoaderNode()
  : rclcpp::Node("apex_scene_loader_node")
  {
    timer_ = this->create_wall_timer(std::chrono::milliseconds(250), std::bind(&ApexSceneLoaderNode::PublishScene, this));
  }

private:
  void PublishScene()
  {
    if (published_)
    {
      return;
    }
    moveit::planning_interface::PlanningSceneInterface planning_scene_interface;
    std::vector<moveit_msgs::msg::CollisionObject> collision_objects;
    {
      moveit_msgs::msg::CollisionObject object;
      object.header.frame_id = "base_link";
      object.id = "ImportedFixture";
      shape_msgs::msg::SolidPrimitive primitive;
      primitive.type = primitive.BOX;
      primitive.dimensions = {0.300000, 0.250000, 0.400000};
      geometry_msgs::msg::Pose pose;
      pose.orientation.w = 1.0;
      pose.position.x = 0.700000;
      pose.position.y = 0.225000;
      pose.position.z = 0.000000;
      object.primitives.push_back(primitive);
      object.primitive_poses.push_back(pose);
      object.operation = object.ADD;
      collision_objects.push_back(object);
    }
    {
      moveit_msgs::msg::CollisionObject object;
      object.header.frame_id = "base_link";
      object.id = "SafetyZone";
      shape_msgs::msg::SolidPrimitive primitive;
      primitive.type = primitive.BOX;
      primitive.dimensions = {0.150000, 1.600000, 1.000000};
      geometry_msgs::msg::Pose pose;
      pose.orientation.w = 1.0;
      pose.position.x = 1.025000;
      pose.position.y = 0.000000;
      pose.position.z = 0.000000;
      object.primitives.push_back(primitive);
      object.primitive_poses.push_back(pose);
      object.operation = object.ADD;
      collision_objects.push_back(object);
    }

    planning_scene_interface.applyCollisionObjects(collision_objects);
    RCLCPP_INFO(this->get_logger(), "Loaded %zu collision objects into the planning scene.", collision_objects.size());
    published_ = true;
  }

  bool published_ = false;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<ApexSceneLoaderNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
