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

#ifndef ROS2_BEHAVIOR_TREE__REPEAT_UNTIL_NODE_HPP_
#define ROS2_BEHAVIOR_TREE__REPEAT_UNTIL_NODE_HPP_

#include <string>

#include "behaviortree_cpp/decorator_node.h"

namespace ros2_behavior_tree
{

class RepeatUntilNode : public BT::DecoratorNode
{
public:
  RepeatUntilNode(const std::string & name, const std::string & key, const bool value)
  : BT::DecoratorNode(name, {}),
    key_(key), target_value_(value), read_parameters_from_ports_(false)
  {
    setRegistrationID("RepeatUntil");
  }

  RepeatUntilNode(const std::string & name, const BT::NodeConfiguration & cfg)
  : BT::DecoratorNode(name, cfg), read_parameters_from_ports_(true)
  {
  }

  // Define this node's ports
  static BT::PortsList providedPorts()
  {
    return {
      BT::InputPort<std::string>("key", "The target key to use"),
      BT::InputPort<bool>("value", "The target value to match")
    };
  }

  void halt() override
  {
    DecoratorNode::halt();
  }

private:
  BT::NodeStatus tick() override
  {
    if (read_parameters_from_ports_) {
      if (!getInput<std::string>("key", key_)) {
        throw BT::RuntimeError("Missing parameter [key] in RepeatUntil node");
      }
      if (!getInput<bool>("value", target_value_)) {
        throw BT::RuntimeError("Missing parameter [value] in RepeatUntil node");
      }
    }

    auto status = child_node_->executeTick();

    if (status == BT::NodeStatus::FAILURE) {
      return BT::NodeStatus::FAILURE;
    }

    bool current_value = false;
    config().blackboard->get<bool>(key_, current_value);

    // We're waiting for the value on the blackboard to match the target
    return (current_value == target_value_) ? BT::NodeStatus::SUCCESS : BT::NodeStatus::RUNNING;
  }

  bool read_parameters_from_ports_;
  std::string key_;
  bool target_value_{true};
};

}  // namespace ros2_behavior_tree

#endif  // ROS2_BEHAVIOR_TREE__REPEAT_UNTIL_NODE_HPP_
