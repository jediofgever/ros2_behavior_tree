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

#include "sample_action_server_node.hpp"

#include <memory>
#include <string>

namespace ros2_behavior_tree
{

// The Behavior Tree to execute
const char SampleActionServerNode::bt_xml_[] =
  R"(
<root main_tree_to_execute="MainTree">
  <BehaviorTree ID="MainTree">
    <Sequence name="say_hello">
      <Message msg="Hello, World!"/>
    </Sequence>
  </BehaviorTree>
</root>
)";

SampleActionServerNode::SampleActionServerNode()
: Node("sample_action_server_node"), bt_(bt_xml_)
{
}

SampleActionServerNode::~SampleActionServerNode()
{
}

BtStatus
SampleActionServerNode::executeBehaviorTree()
{
  return bt_.execute();
}

}  // namespace ros2_behavior_tree
