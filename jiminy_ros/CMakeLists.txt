cmake_minimum_required(VERSION 3.5)
project(jiminy_ros)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(jiminy_msgs REQUIRED)
find_package(yaml-cpp REQUIRED)

# C++
include_directories(src)
include_directories(include)

set(DEPENDENCIES
  rclcpp::rclcpp
  rclcpp_lifecycle::rclcpp_lifecycle
  ${jiminy_msgs_TARGETS}
  yaml-cpp
)
set(DEPENDENCIES_PACKAGE
  rclcpp
  rclcpp_lifecycle
  jiminy_msgs
)

install(DIRECTORY include/
  DESTINATION include/${PROJECT_NAME}
)

# Library for reuse in tests
add_library(${PROJECT_NAME}_lib
  src/jiminy_ros/jiminy.cpp
  src/jiminy_ros/jiminy_node.cpp
)
target_link_libraries(${PROJECT_NAME}_lib PUBLIC ${DEPENDENCIES})
target_include_directories(${PROJECT_NAME}_lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)

# demo
add_executable(jiminy_node 
  src/jiminy_main.cpp 
)
target_link_libraries(jiminy_node PUBLIC ${PROJECT_NAME}_lib)
install(TARGETS
  jiminy_node
  DESTINATION lib/${PROJECT_NAME}
)

# Install scenarios
install(
  DIRECTORY scenarios
  DESTINATION share/${PROJECT_NAME}
)

# Testing
if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(ament_cmake_pytest REQUIRED)
  find_package(ament_index_cpp REQUIRED)

  # C++ Tests
  # Test for Jiminy class
  ament_add_gtest(test_jiminy
    test/test_jiminy.cpp
  )
  target_link_libraries(test_jiminy ${PROJECT_NAME}_lib)

  # Test for JiminyNode class
  ament_add_gtest(test_jiminy_node
    test/test_jiminy_node.cpp
  )
  target_link_libraries(test_jiminy_node ${PROJECT_NAME}_lib)

  # Integration tests for Jiminy services (uses ROS 2 nodes + MultiThreadedExecutor)
  ament_add_gtest(test_integration_jiminy
    test/test_integration_jiminy.cpp
    TIMEOUT 120
  )
  target_link_libraries(test_integration_jiminy ${PROJECT_NAME}_lib ament_index_cpp::ament_index_cpp)
endif()

ament_package()