#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <std_msgs/msg/string.hpp>

#include <sstream>
#include <vector>

class ApexProjectBridgeNode final : public rclcpp::Node
{
public:
  ApexProjectBridgeNode()
  : rclcpp::Node("apex_project_bridge_node")
  {
    joint_names_ = this->declare_parameter<std::vector<std::string>>("joint_names", std::vector<std::string>{});
    joint_state_pub_ = this->create_publisher<sensor_msgs::msg::JointState>("joint_states", 10);
    summary_pub_ = this->create_publisher<std_msgs::msg::String>("apex_project_summary", 10);
    timer_ = this->create_wall_timer(std::chrono::milliseconds(500), std::bind(&ApexProjectBridgeNode::OnTimer, this));
  }

private:
  void OnTimer()
  {
    sensor_msgs::msg::JointState state;
    state.header.stamp = this->now();
    state.name = joint_names_;
    state.position.assign(joint_names_.size(), 0.0);
    joint_state_pub_->publish(state);

    std_msgs::msg::String summary;
    std::ostringstream stream;
    stream << "Project: ApexPainterV16 imported project | Robot: ApexPainterV16 | Obstacles: 2";
    summary.data = stream.str();
    summary_pub_->publish(summary);
  }

  std::vector<std::string> joint_names_;
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_state_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr summary_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<ApexProjectBridgeNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
