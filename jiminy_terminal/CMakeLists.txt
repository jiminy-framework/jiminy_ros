cmake_minimum_required(VERSION 3.5)
project(jiminy_terminal)

set(CMAKE_CXX_STANDARD 17)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(jiminy_msgs REQUIRED)

include_directories(include)

add_library(jiminy_terminal_lib SHARED
  src/jiminy_terminal/terminal.cpp
)
target_link_libraries(jiminy_terminal_lib
  rclcpp::rclcpp
  ${jiminy_msgs_TARGETS}
)

add_executable(${PROJECT_NAME}
  src/terminal_node.cpp
)
target_link_libraries(${PROJECT_NAME}
  jiminy_terminal_lib
  rclcpp::rclcpp
  ${jiminy_msgs_TARGETS}
  readline
)

install(TARGETS
  jiminy_terminal_lib
  ${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include/
)

ament_package()
