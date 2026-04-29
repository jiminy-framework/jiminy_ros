// MIT License
//
// Copyright (c) 2026 Miguel Ángel González Santamarta
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef MINI_JIMINY__MINI_JIMINY_NODE_HPP_
#define MINI_JIMINY__MINI_JIMINY_NODE_HPP_

#include <memory>

#include <rclcpp_lifecycle/lifecycle_node.hpp>

#include "mini_jiminy/mini_jiminy.hpp"
#include "mini_jiminy_msgs/srv/call_jiminy.hpp"
#include "mini_jiminy_msgs/srv/get_scenario.hpp"
#include "mini_jiminy_msgs/srv/load_scenario.hpp"

namespace mini_jiminy {

/**
 * @class JiminyNode
 * @brief A lifecycle node for the Mini Jiminy framework.
 */
class JiminyNode : public rclcpp_lifecycle::LifecycleNode {
public:
  /**
   * @brief Constructor for the JiminyNode class.
   */
  JiminyNode();

  /**
   * @brief Callback for the "configure" lifecycle transition.
   *
   * This method is called when the node transitions to the "configured" state.
   *
   * @param state The current lifecycle state.
   * @return The result of the transition, indicating success or failure.
   */
  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  on_configure(const rclcpp_lifecycle::State &state);

  /**
   * @brief Callback for the "activate" lifecycle transition.
   *
   * This method is called when the node transitions to the "active" state.
   *
   * @param state The current lifecycle state.
   * @return The result of the transition, indicating success or failure.
   */
  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  on_activate(const rclcpp_lifecycle::State &state);

  /**
   * @brief Callback for the "deactivate" lifecycle transition.
   *
   * This method is called when the node transitions to the "inactive" state.
   *
   * @param state The current lifecycle state.
   * @return The result of the transition, indicating success or failure.
   */
  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  on_deactivate(const rclcpp_lifecycle::State &state);

  /**
   * @brief Callback for the "cleanup" lifecycle transition.
   *
   * This method is called when the node transitions to the "cleaned up" state.
   *
   * @param state The current lifecycle state.
   * @return The result of the transition, indicating success or failure.
   */
  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  on_cleanup(const rclcpp_lifecycle::State &state);

  /**
   * @brief Callback for the "shutdown" lifecycle transition.
   *
   * This method is called when the node transitions to the "shutdown" state.
   *
   * @param state The current lifecycle state.
   * @return The result of the transition, indicating success or failure.
   */
  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  on_shutdown(const rclcpp_lifecycle::State &state);

private:
  /**
   * @brief Load the Jiminy configuration from a file.
   * @param config_file The path to the configuration file.
   * @return A unique pointer to the loaded Jiminy instance.
   */
  std::unique_ptr<Jiminy> load_jiminy(const std::string &config_file);

  /**
   * @brief Callback function for the Jiminy service.
   * @param request The service request containing the Jiminy configuration.
   * @param response The service response to be populated.
   */
  void call_jiminy_service_callback(
      const std::shared_ptr<mini_jiminy_msgs::srv::CallJiminy::Request> request,
      std::shared_ptr<mini_jiminy_msgs::srv::CallJiminy::Response> response);

  /**
   * @brief Callback function for the GetScenario service.
   * @param request The service request (empty).
   * @param response The service response to be populated with the scenario.
   */
  void get_scenario_service_callback(
      const std::shared_ptr<mini_jiminy_msgs::srv::GetScenario::Request>
          request,
      std::shared_ptr<mini_jiminy_msgs::srv::GetScenario::Response> response);

  /**
   * @brief Callback function for the LoadScenario service.
   * @param request The service request containing the config file path.
   * @param response The service response with the loaded scenario.
   */
  void load_scenario_service_callback(
      const std::shared_ptr<mini_jiminy_msgs::srv::LoadScenario::Request>
          request,
      std::shared_ptr<mini_jiminy_msgs::srv::LoadScenario::Response> response);

  /// @brief Unique pointer to the Jiminy instance.
  std::unique_ptr<Jiminy> jiminy_;
  /// @brief Configuration file path.
  std::string config_file_;

  /// @brief Service for handling Jiminy calls.
  std::shared_ptr<rclcpp::Service<mini_jiminy_msgs::srv::CallJiminy>>
      jiminy_service_;
  /// @brief Service for retrieving the scenario.
  std::shared_ptr<rclcpp::Service<mini_jiminy_msgs::srv::GetScenario>>
      get_scenario_service_;
  /// @brief Service for loading a new scenario from file.
  std::shared_ptr<rclcpp::Service<mini_jiminy_msgs::srv::LoadScenario>>
      load_scenario_service_;
};

} // namespace mini_jiminy

#endif // MINI_JIMINY__MINI_JIMINY_NODE_HPP
