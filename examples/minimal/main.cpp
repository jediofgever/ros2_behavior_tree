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

#include <string>
#include <stdexcept>

#include "rclcpp/rclcpp.hpp"
#include "ros2_behavior_tree/behavior_tree_engine.hpp"

// The Behavior Tree to execute
static const char xml_text[] =
  R"(
<root main_tree_to_execute="MainTree">
  <BehaviorTree ID="MainTree">
    <Sequence name="say_hello">
      <Message msg="Hello,"/>
      <Message msg="World!"/>
    </Sequence>
  </BehaviorTree>
</root>
)";

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  // Create a behavior tree engine to run the BT XML, specifying which plugins to use
  ros2_behavior_tree::BehaviorTreeEngine bt_engine({"ros2_behavior_tree_nodes"});

  // Run the BT and determine the result
  auto rc = bt_engine.run(xml_text);

  switch (rc) {
    case ros2_behavior_tree::BtStatus::SUCCEEDED:
      break;

    case ros2_behavior_tree::BtStatus::FAILED:
      printf("BT failed\n");
      break;

    case ros2_behavior_tree::BtStatus::CANCELED:
      printf("BT was canceled\n");
      break;

    default:
      throw std::logic_error("Invalid return value from the BT");
  }

  rclcpp::shutdown();
  return 0;
}