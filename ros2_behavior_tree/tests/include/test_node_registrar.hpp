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

#ifndef TEST_NODE_REGISTRAR_HPP_
#define TEST_NODE_REGISTRAR_HPP_

#include "behaviortree_cpp_v3/behavior_tree.h"
#include "behaviortree_cpp_v3/bt_factory.h"

namespace ros2_behavior_tree
{

class TestNodeRegistrar
{
public:
  static void RegisterNodes(BT::BehaviorTreeFactory & factory);
};

}  // namespace ros2_behavior_tree

#endif  // TEST_NODE_REGISTRAR_HPP_
