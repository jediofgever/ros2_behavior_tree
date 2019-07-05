// Copyright (c) 2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "ros2_behavior_tree/behavior_tree_engine.hpp"

#include <memory>
#include <string>

#include "behaviortree_cpp/blackboard/blackboard_local.h"
#include "ros2_behavior_tree/bt_conversions.hpp"
#include "ros2_behavior_tree/conditional_loop_node.hpp"
#include "ros2_behavior_tree/rate_controller_node.hpp"
#include "ros2_behavior_tree/recovery_node.hpp"
#include "rclcpp/rclcpp.hpp"

using namespace std::chrono_literals;

namespace ros2_behavior_tree
{

BehaviorTreeEngine::BehaviorTreeEngine()
{
  // Register our custom control nodes
  factory_.registerNodeType<RecoveryNode>("RecoveryNode");

  // Register our custom decorator nodes
  factory_.registerNodeType<RateController>("RateController");
  factory_.registerNodeType<ConditionalLoop>("ConditionalLoop");

  // Register our simple condition nodes
  // factory_.registerSimpleCondition("initialPoseReceived",
  //    std::bind(&NavigateToPoseBehaviorTree::initialPoseReceived, this, std::placeholders::_1));

  // Register our custom condition nodes
  // factory_.registerNodeType<ros2_behavior_tree::IsStuckCondition>("IsStuck");

  // Register our simple action nodes
  // factory_.registerSimpleAction("clearEntirely",
  //   std::bind(&NavigateToPoseBehaviorTree::clear, this, std::placeholders::_1));

  const BT::NodeParameters message_params {{"msg", "unknown"}};
  registerSimpleActionWithParameters("Message",
    std::bind(&BehaviorTreeEngine::message, this, std::placeholders::_1),
    message_params);

  const BT::NodeParameters set_condition_params {{"key", "unknown"}, {"value", "unknown"}};
  registerSimpleActionWithParameters("SetCondition",
    std::bind(&BehaviorTreeEngine::setCondition, this, std::placeholders::_1),
    set_condition_params);

  // Register our custom action nodes so that they can be included in XML description
  // factory_.registerNodeType<ros2_behavior_tree::ComputePathToPoseAction>("ComputePathToPose");
}

BtStatus
BehaviorTreeEngine::run(
  BT::Blackboard::Ptr & blackboard,
  const std::string & behavior_tree_xml,
  std::function<void()> onLoop,
  std::function<bool()> cancelRequested,
  std::chrono::milliseconds loopTimeout)
{
  // Parse the input XML and create the corresponding Behavior Tree
  BT::Tree tree = BT::buildTreeFromText(factory_, behavior_tree_xml, blackboard);

  rclcpp::WallRate loopRate(loopTimeout);
  BT::NodeStatus result = BT::NodeStatus::RUNNING;

  // Loop until something happens with ROS or the node completes
  while (rclcpp::ok() && result == BT::NodeStatus::RUNNING) {
    if (cancelRequested()) {
      tree.root_node->halt();
      return BtStatus::CANCELED;
    }

    onLoop();
    result = tree.root_node->executeTick();
    loopRate.sleep();
  }

  return (result == BT::NodeStatus::SUCCESS) ? BtStatus::SUCCEEDED : BtStatus::FAILED;
}

BtStatus
BehaviorTreeEngine::run(
  std::unique_ptr<BT::Tree> & tree,
  std::function<void()> onLoop,
  std::function<bool()> cancelRequested,
  std::chrono::milliseconds loopTimeout)
{
  rclcpp::WallRate loopRate(loopTimeout);
  BT::NodeStatus result = BT::NodeStatus::RUNNING;

  // Loop until something happens with ROS or the node completes w/ success or failure
  while (rclcpp::ok() && result == BT::NodeStatus::RUNNING) {
    if (cancelRequested()) {
      tree->root_node->halt();
      return BtStatus::CANCELED;
    }

    onLoop();
    result = tree->root_node->executeTick();
    loopRate.sleep();
  }

  return (result == BT::NodeStatus::SUCCESS) ? BtStatus::SUCCEEDED : BtStatus::FAILED;
}

BT::Tree
BehaviorTreeEngine::buildTreeFromText(std::string & xml_string, BT::Blackboard::Ptr blackboard)
{
  return BT::buildTreeFromText(factory_, xml_string, blackboard);
}

void
BehaviorTreeEngine::registerSimpleActionWithParameters(
  const std::string & ID,
  const BT::SimpleActionNode::TickFunctor & tick_functor,
  const BT::NodeParameters & params)
{
  BT::NodeBuilder builder =
    [tick_functor, ID](const std::string & name, const BT::NodeParameters & params) {
      return std::unique_ptr<BT::TreeNode>(new BT::SimpleActionNode(name, tick_functor, params));
    };

  BT::TreeNodeManifest manifest = {BT::NodeType::ACTION, ID, params};
  factory_.registerBuilder(manifest, builder);
}

#define ANSI_COLOR_RESET    "\x1b[0m"
#define ANSI_COLOR_BLUE     "\x1b[34m"

BT::NodeStatus
BehaviorTreeEngine::message(BT::TreeNode & tree_node)
{
  // std::string msg;
  // tree_node.getParam<std::string>("msg", msg);

  // RCLCPP_INFO(service_client_node_->get_logger(),
  //   ANSI_COLOR_BLUE "\33[1m%s\33[0m" ANSI_COLOR_RESET, msg.c_str());

  return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus
BehaviorTreeEngine::setCondition(BT::TreeNode & tree_node)
{
  std::string key;
  tree_node.getParam<std::string>("key", key);

  std::string value;
  tree_node.getParam<std::string>("value", value);

  tree_node.blackboard()->template set<bool>(key, (value == "true") ? true : false);  // NOLINT

  return BT::NodeStatus::SUCCESS;
}

}  // namespace ros2_behavior_tree
