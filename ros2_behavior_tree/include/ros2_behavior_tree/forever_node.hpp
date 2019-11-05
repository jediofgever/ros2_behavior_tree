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

#ifndef ROS2_BEHAVIOR_TREE__FOREVER_NODE_HPP_
#define ROS2_BEHAVIOR_TREE__FOREVER_NODE_HPP_

#include <chrono>
#include <string>

#include "behaviortree_cpp/decorator_node.h"

namespace ros2_behavior_tree
{

class Forever : public BT::DecoratorNode
{
public:
  explicit Forever(const std::string & name)
  : BT::DecoratorNode(name, {})
  {
  }
  Forever() = delete;

private:
  BT::NodeStatus tick() override;
};

inline BT::NodeStatus Forever::tick()
{
  setStatus(BT::NodeStatus::RUNNING);

  const BT::NodeStatus child_state = child_node_->executeTick();

  switch (child_state) {
    case BT::NodeStatus::SUCCESS:
    case BT::NodeStatus::RUNNING:
      return BT::NodeStatus::RUNNING;

    case BT::NodeStatus::FAILURE:
    default:
      child_node_->setStatus(BT::NodeStatus::IDLE);
      return BT::NodeStatus::FAILURE;
  }

  return status();
}

}  // namespace ros2_behavior_tree

#endif  // ROS2_BEHAVIOR_TREE__FOREVER_NODE_HPP_