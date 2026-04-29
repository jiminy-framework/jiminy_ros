cmake_minimum_required(VERSION 3.5)
project(mini_jiminy_msgs)

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/Fact.msg"
  "msg/Norm.msg"
  "msg/Contrary.msg"
  "msg/Priority.msg"
  "msg/Scenario.msg"
  "msg/Semantics.msg"
  "srv/CallJiminy.srv"
  "srv/GetScenario.srv"
)

ament_package()
