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

#ifndef ROS2_BEHAVIOR_TREE__RECOVERY_NODE_HPP_
#define ROS2_BEHAVIOR_TREE__RECOVERY_NODE_HPP_

#include <string>

#include "behaviortree_cpp/control_node.h"

namespace ros2_behavior_tree
{

//
// @brief The RecoveryNode has only two children and returns SUCCESS if and only if the first child
// returns SUCCESS.
//
// - If the first child returns FAILURE, the second child will be executed.  After that the first
//   child is executed again if the second child returns SUCCESS.
//
// - If the first or second child returns RUNNING, this node returns RUNNING.
//
// - If the second child returns FAILURE, this control node will stop the loop and returns FAILURE.
//
// - TODO(mjeronimo): explain the retry count
//
class RecoveryNode : public BT::ControlNode
{
public:
  RecoveryNode(const std::string & name, const BT::NodeConfiguration & config);
  ~RecoveryNode() override = default;

  // Define this node's ports
  static BT::PortsList providedPorts()
  {
    return {BT::InputPort<int>("number_of_retries", 1, "Number of retries")};
  }

private:
  BT::NodeStatus tick() override;
  void halt() override;

  unsigned int current_child_idx_;
  unsigned int number_of_retries_;
  unsigned int retry_count_;
};

}  // namespace ros2_behavior_tree

#endif  // ROS2_BEHAVIOR_TREE__RECOVERY_NODE_HPP_
