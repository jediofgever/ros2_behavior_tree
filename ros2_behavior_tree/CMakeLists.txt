cmake_minimum_required(VERSION 3.5)
project(ros2_behavior_tree)

find_package(ament_cmake REQUIRED)
find_package(behaviortree_cpp_v3 REQUIRED)
find_package(builtin_interfaces REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(std_srvs REQUIRED)

include_directories(
  include
)

set(library_name ${PROJECT_NAME})

add_library(${library_name} SHARED
  src/behavior_tree.cpp
  src/recovery_node.cpp
)

add_library(ros2_behavior_tree_nodes SHARED
  src/bt_registrar.cpp
)

add_executable(minimal examples/minimal/main.cpp)

add_executable(node
  examples/node/main.cpp
  examples/node/sample_node.cpp
)

add_executable(action_server_node
  examples/action_server_node/main.cpp
  examples/action_server_node/sample_action_server_node.cpp
)

add_executable(lifecycle_node
  examples/lifecycle_node/main.cpp
  examples/lifecycle_node/sample_lifecycle_node.cpp
)

set(dependencies
  behaviortree_cpp_v3
  rclcpp
  rclcpp_action
  rclcpp_lifecycle
  std_msgs
  std_srvs
)

ament_target_dependencies(${library_name}
  ${dependencies}
)

ament_target_dependencies(ros2_behavior_tree_nodes
  ${dependencies}
)

ament_target_dependencies(minimal
  ${dependencies}
)

ament_target_dependencies(node
  ${dependencies}
)

ament_target_dependencies(action_server_node
  ${dependencies}
)

ament_target_dependencies(lifecycle_node
  ${dependencies}
)

target_link_libraries(minimal ${library_name})
target_link_libraries(node ${library_name})
target_link_libraries(lifecycle_node ${library_name})
target_link_libraries(action_server_node ${library_name})

target_compile_definitions(ros2_behavior_tree_nodes PRIVATE
  BT_PLUGIN_EXPORT
)

install(TARGETS ${library_name} ros2_behavior_tree_nodes minimal node action_server_node lifecycle_node
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include/
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_export_include_directories(include)

ament_export_libraries(
  ${library_name}
  ros2_behavior_tree_nodes
)

ament_export_dependencies(${dependencies})

ament_package()