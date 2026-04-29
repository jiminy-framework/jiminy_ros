cmake_minimum_required(VERSION 3.5)
project(mini_jiminy)

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
find_package(mini_jiminy_msgs REQUIRED)
find_package(yaml-cpp REQUIRED)

# C++
include_directories(src)
include_directories(include)

set(DEPENDENCIES
  rclcpp::rclcpp
  rclcpp_lifecycle::rclcpp_lifecycle
  ${mini_jiminy_msgs_TARGETS}
  yaml-cpp
)
set(DEPENDENCIES_PACKAGE
  rclcpp
  rclcpp_lifecycle
  mini_jiminy_msgs
)

install(DIRECTORY include/
  DESTINATION include/${PROJECT_NAME}
)

# Library for reuse in tests
add_library(${PROJECT_NAME}_lib
  src/mini_jiminy/mini_jiminy.cpp
  src/mini_jiminy/mini_jiminy_node.cpp
)
target_link_libraries(${PROJECT_NAME}_lib PUBLIC ${DEPENDENCIES})
target_include_directories(${PROJECT_NAME}_lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)

# demo
add_executable(mini_jiminy_node 
  src/mini_jiminy_main.cpp 
)
target_link_libraries(mini_jiminy_node PUBLIC ${PROJECT_NAME}_lib)
install(TARGETS
  mini_jiminy_node
  DESTINATION lib/${PROJECT_NAME}
)

# Testing
if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(ament_cmake_pytest REQUIRED)

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

  # Python Integration Tests
  # Load scenario service tests
  ament_add_pytest_test(test_load_scenario_service
    ../test/integration/test_load_scenario_service.py
    TIMEOUT 120
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..
  )

  # Jiminy reasoning service tests  
  ament_add_pytest_test(test_jiminy_reasoning
    ../test/integration/test_jiminy_reasoning.py
    TIMEOUT 120
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..
  )
endif()

ament_package()