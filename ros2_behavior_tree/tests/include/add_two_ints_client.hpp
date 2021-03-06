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

#ifndef ADD_TWO_INTS_CLIENT_HPP_
#define ADD_TWO_INTS_CLIENT_HPP_

#include <memory>
#include <string>

#include "ros2_behavior_tree/ros2_service_client_node.hpp"
#include "ros2_behavior_tree/ros2_async_service_client_node.hpp"
#include "example_interfaces/srv/add_two_ints.hpp"

using AddTwoInts = example_interfaces::srv::AddTwoInts;

class AddTwoIntsClient : public ros2_behavior_tree::ROS2AsyncServiceClientNode<AddTwoInts>
{
public:
  explicit AddTwoIntsClient(const std::string & name, const BT::NodeConfiguration & config)
  : ROS2AsyncServiceClientNode<AddTwoInts>(name, config)
  {
  }

  static BT::PortsList providedPorts()
  {
    return augment_basic_ports({
      BT::InputPort<int64_t>("a", "The augend"),
      BT::InputPort<int64_t>("b", "The addend"),
      BT::OutputPort<int64_t>("sum", "The sum of the addition")
    });
  }

  void read_input_ports(std::shared_ptr<AddTwoInts::Request> request) override
  {
    if (!getInput<int64_t>("a", request->a)) {
      throw BT::RuntimeError("Missing parameter [a] in AddTwoInts node");
    }

    if (!getInput<int64_t>("b", request->b)) {
      throw BT::RuntimeError("Missing parameter [b] in AddTwoInts node");
    }
  }

  void write_output_ports(std::shared_ptr<AddTwoInts::Response> response) override
  {
    setOutput("sum", response->sum);
  }
};

#endif  // ADD_TWO_INTS_CLIENT_HPP_
