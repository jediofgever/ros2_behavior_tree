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

#ifndef EXAMPLES__SIMPLE_NODE__SIMPLE_NODE_HPP_
#define EXAMPLES__SIMPLE_NODE__SIMPLE_NODE_HPP_

#include <memory>
#include <thread>

#include "ros2_behavior_tree/behavior_tree_engine.hpp"
#include "rclcpp/rclcpp.hpp"

namespace ros2_behavior_tree
{

class SimpleNode : public rclcpp::Node
{
public:
  SimpleNode();
  ~SimpleNode();

protected:
  BtStatus run();
  BehaviorTreeEngine bt_engine_;
  std::unique_ptr<std::thread> thread_;
};

}  // namespace ros2_behavior_tree

#endif  // EXAMPLES__SIMPLE_NODE__SIMPLE_NODE_HPP_