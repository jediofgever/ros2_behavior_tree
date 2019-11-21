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

#ifndef ROS2_BEHAVIOR_TREE__ROS2_ACTION_CLIENT_NODE_HPP_
#define ROS2_BEHAVIOR_TREE__ROS2_ACTION_CLIENT_NODE_HPP_

#include <memory>
#include <string>

#include "behaviortree_cpp_v3/action_node.h"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"

namespace ros2_behavior_tree
{

template<class ActionT>
class ROS2ActionClientNode : public BT::CoroActionNode
{
public:
  ROS2ActionClientNode(const std::string & name, const BT::NodeConfiguration & config)
  : BT::CoroActionNode(name, config)
  {
    // Initialize the input and output messages
    goal_ = typename ActionT::Goal();
    result_ = typename rclcpp_action::ClientGoalHandle<ActionT>::WrappedResult();
  }

  ROS2ActionClientNode() = delete;

  // Define the ports required by the ROS2ActionClient node
  static BT::PortsList augment_basic_ports(BT::PortsList additional_ports)
  {
    BT::PortsList basic_ports = {
      BT::InputPort<std::string>("action_name", "The name of the action to call"),
      BT::InputPort<std::chrono::milliseconds>("wait_timeout",
        "The timeout value, in milliseconds, to use when waiting for the service"),
      BT::InputPort<std::chrono::milliseconds>("call_timeout",
        "The timeout value, in milliseconds, to use when calling the service"),
      BT::InputPort<std::shared_ptr<rclcpp::Node>>("client_node",
        "The (non-spinning) client node to use when making service calls")
    };

    basic_ports.insert(additional_ports.begin(), additional_ports.end());
    return basic_ports;
  }

  // Any subclass of ROS2ActionClientNode that defines additional ports must then define
  // its own providedPorts method and call augment_basic_ports to add the subclass's ports
  // to the required basic ports
  static BT::PortsList providedPorts()
  {
    return augment_basic_ports({});
  }

  // A derived class the defines input and/or output ports can override these methods
  // to get/set the ports
  virtual void read_input_ports() {}
  virtual void write_output_ports() {}
  virtual bool new_goal_received() {return false;}

  // The main override required by a BT action
  BT::NodeStatus tick() override
  {
    if (!getInput("action_name", action_name_)) {
      throw BT::RuntimeError("Missing parameter [action_name] in ROS2ServiceClientNode");
    }

    if (!getInput<std::chrono::milliseconds>("wait_timeout", wait_timeout_)) {
      throw BT::RuntimeError("Missing parameter [wait_timeout] in ROS2ServiceClientNode");
    }

    if (!getInput<std::chrono::milliseconds>("call_timeout", call_timeout_)) {
      throw BT::RuntimeError("Missing parameter [call_timeout] in ROS2ServiceClientNode");
    }

    if (!getInput<std::shared_ptr<rclcpp::Node>>("client_node", client_node_)) {
      throw BT::RuntimeError("Missing parameter [client_node] in ROS2ServiceClientNode");
    }

    read_input_ports();

    if (action_client_ == nullptr) {
      action_client_ = rclcpp_action::create_client<ActionT>(client_node_, action_name_);
    }

    // Make sure the action server is available there before continuing
    if (!action_client_->wait_for_action_server(std::chrono::milliseconds(wait_timeout_))) {
      RCLCPP_ERROR(client_node_->get_logger(),
        "Timed out waiting for action server \"%s\" to become available", action_name_.c_str());
      return BT::NodeStatus::FAILURE;
    }

    // Enable result awareness by providing an empty lambda function
    auto send_goal_options = typename rclcpp_action::Client<ActionT>::SendGoalOptions();
    send_goal_options.result_callback = [](auto) {};

new_goal_received:
    auto future_goal_handle = action_client_->async_send_goal(goal_, send_goal_options);
    if (rclcpp::spin_until_future_complete(client_node_, future_goal_handle) !=
      rclcpp::executor::FutureReturnCode::SUCCESS)
    {
      throw std::runtime_error("send_goal failed");
    }

    goal_handle_ = future_goal_handle.get();
    if (!goal_handle_) {
      throw std::runtime_error("Goal was rejected by the action server");
    }

    auto future_result = goal_handle_->async_result();
    rclcpp::executor::FutureReturnCode rc;
    do {
      rc = rclcpp::spin_until_future_complete(client_node_, future_result, call_timeout_);
      if (rc == rclcpp::executor::FutureReturnCode::TIMEOUT) {
        if (new_goal_received()) {
          // If we're received a new goal, cancel the current goal and start a new one
          auto future = action_client_->async_cancel_goal(goal_handle_);
          if (rclcpp::spin_until_future_complete(client_node_, future) !=
            rclcpp::executor::FutureReturnCode::SUCCESS)
          {
            RCLCPP_WARN(client_node_->get_logger(), "failed to cancel goal");
          }

          goto new_goal_received;
        }

        // Yield to any other CoroActionNodes (coroutines)
        setStatusRunningAndYield();
      }
    } while (rc != rclcpp::executor::FutureReturnCode::SUCCESS);

    result_ = future_result.get();
    switch (result_.code) {
      case rclcpp_action::ResultCode::SUCCEEDED:
        write_output_ports();
        return BT::NodeStatus::SUCCESS;

      case rclcpp_action::ResultCode::ABORTED:
        return BT::NodeStatus::FAILURE;

      case rclcpp_action::ResultCode::CANCELED:
        return BT::NodeStatus::SUCCESS;

      default:
        throw std::logic_error("ROS2ActionClientNode::tick: invalid status value");
    }
  }

  // The other (optional) override required by a BT action. In this case, we
  // make sure to cancel the ROS2 action if it is still running.
  void halt() override
  {
    if (should_cancel_goal()) {
      auto future_cancel = action_client_->async_cancel_goal(goal_handle_);
      if (rclcpp::spin_until_future_complete(client_node_, future_cancel) !=
        rclcpp::executor::FutureReturnCode::SUCCESS)
      {
        RCLCPP_ERROR(client_node_->get_logger(),
          "Failed to cancel action server for %s", action_name_.c_str());
      }
    }

    CoroActionNode::halt();
  }

protected:
  bool should_cancel_goal()
  {
#if 0
    return status() == BT::NodeStatus::RUNNING;
#else
    // Shut the node down if it is currently running
    if (status() != BT::NodeStatus::RUNNING) {
      return false;
    }

    rclcpp::spin_some(client_node_);
    auto status = goal_handle_->get_status();

    // Check if the goal is still executing
    if (status == action_msgs::msg::GoalStatus::STATUS_ACCEPTED ||
      status == action_msgs::msg::GoalStatus::STATUS_EXECUTING)
    {
      return true;
    }

    return false;
#endif
  }

  // bool goal_updated_{false};

  typename std::shared_ptr<rclcpp_action::Client<ActionT>> action_client_;
  typename rclcpp_action::ClientGoalHandle<ActionT>::SharedPtr goal_handle_;

  // The (non-spinning) node to use when calling the service
  rclcpp::Node::SharedPtr client_node_;

  std::string action_name_;

  std::chrono::milliseconds wait_timeout_;
  std::chrono::milliseconds call_timeout_;

  typename ActionT::Goal goal_;
  typename rclcpp_action::ClientGoalHandle<ActionT>::WrappedResult result_;
};

}  // namespace ros2_behavior_tree

#endif  // ROS2_BEHAVIOR_TREE__ROS2_ACTION_CLIENT_NODE_HPP_
