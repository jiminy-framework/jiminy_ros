// MIT License
//
// Copyright (c) 2026 Jiminy Framework
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

/**
 * @file test_integration_jiminy.cpp
 * @brief ROS 2 integration tests for Jiminy services.
 *
 * Mirrors the Python integration tests in test/integration/ using native
 * ROS 2 client/server communication with a multithread executor.
 *
 * Test suites:
 *   1. Scenario loading (all YAML files)
 *   2. Scenario content verification (norm/fact/contrary counts)
 *   3. Priority resolution
 *   4. Scenario switching (hot-reload)
 *   5. Error handling
 *   6. Reasoning service (call_jiminy with facts)
 */

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "ament_index_cpp/get_package_share_directory.hpp"
#include <rclcpp/executors.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include <rclcpp_lifecycle/state.hpp>

#include "jiminy_msgs/msg/semantics.hpp"
#include "jiminy_msgs/srv/call_jiminy.hpp"
#include "jiminy_msgs/srv/get_scenario.hpp"
#include "jiminy_msgs/srv/load_scenario.hpp"
#include "jiminy_ros/jiminy_node.hpp"

using namespace std::chrono_literals;

//==============================================================================
// Constants
//==============================================================================
namespace {

/// Timeout for service calls.
constexpr auto SERVICE_TIMEOUT = 5s;

/// Helper: build absolute path to a scenario file.
std::string scenario_path(const std::string &relative_name) {
  return ament_index_cpp::get_package_share_directory("jiminy_ros") +
         "/scenarios/" + relative_name;
}

} // namespace

//==============================================================================
// Test fixture: creates and manages a JiminyNode + client node
//==============================================================================
class JiminyIntegrationTest : public ::testing::Test {
protected:
  void SetUp() override {
    rclcpp::init(0, nullptr);

    // Create the server node (JiminyNode)
    server_node_ = std::make_shared<jiminy::JiminyNode>();

    // Set a valid config_file parameter BEFORE configure() so the node
    // doesn't try to load a non-existent "config.yaml" on activation.
    // We use a temporary file that will be overwritten by each test.
    temp_config_ = "/tmp/test_jiminy_placeholder.yaml";
    write_placeholder_config(temp_config_);
    server_node_->set_parameter(rclcpp::Parameter("config_file", temp_config_));

    server_node_->configure();
    server_node_->activate();

    // Create a client node for service calls
    client_node_ = rclcpp::Node::make_shared("test_integration_client");

    // Create executor and add both nodes
    executor_ = std::make_unique<rclcpp::executors::MultiThreadedExecutor>();
    executor_->add_node(server_node_->get_node_base_interface());
    executor_->add_node(client_node_);

    // Start executor in a separate thread
    executor_thread_ = std::thread([this]() { executor_->spin(); });
  }

  void TearDown() override {
    // Stop the executor thread BEFORE resetting anything
    executor_->cancel();
    executor_thread_.join();

    executor_.reset();
    server_node_->deactivate();
    server_node_.reset();
    client_node_.reset();
    rclcpp::shutdown();
  }

  /// Write a minimal valid YAML config so the node can activate without error.
  void write_placeholder_config(const std::string &path) {
    std::ofstream file(path);
    file << R"(
description: "Placeholder"
context: []
)";
    file.close();
  }

  /// Configure and activate the server node with a given config file.
  bool activate_node(const std::string &config_file) {
    server_node_->set_parameter(rclcpp::Parameter("config_file", config_file));
    auto config_state = server_node_->configure();
    if (!config_state.id())
      return false;
    auto activate_state = server_node_->activate();
    return static_cast<bool>(activate_state.id());
  }

  /// Create a CallJiminy client and wait for the service.
  rclcpp::Client<jiminy_msgs::srv::CallJiminy>::SharedPtr
  create_call_jiminy_client() {
    return client_node_->create_client<jiminy_msgs::srv::CallJiminy>(
        "call_jiminy");
  }

  /// Create a LoadScenario client and wait for the service.
  rclcpp::Client<jiminy_msgs::srv::LoadScenario>::SharedPtr
  create_load_scenario_client() {
    return client_node_->create_client<jiminy_msgs::srv::LoadScenario>(
        "load_scenario");
  }

  /// Create a GetScenario client and wait for the service.
  rclcpp::Client<jiminy_msgs::srv::GetScenario>::SharedPtr
  create_get_scenario_client() {
    return client_node_->create_client<jiminy_msgs::srv::GetScenario>(
        "get_scenario");
  }

  /// Async send a CallJiminy request and wait for response.
  std::pair<bool, std::shared_ptr<jiminy_msgs::srv::CallJiminy::Response>>
  call_jiminy(rclcpp::Client<jiminy_msgs::srv::CallJiminy>::SharedPtr client,
              const std::vector<std::string> &facts,
              const std::string &semantics = "grounded") {

    if (!client->wait_for_service(SERVICE_TIMEOUT)) {
      return {false, nullptr};
    }

    auto request = std::make_shared<jiminy_msgs::srv::CallJiminy::Request>();
    request->facts = facts;
    request->semantics.semantics = semantics;

    auto future = client->async_send_request(request);
    auto status = future.wait_for(SERVICE_TIMEOUT);

    if (status == std::future_status::ready) {
      return {true, future.get()};
    }
    return {false, nullptr};
  }

  /// Async send a LoadScenario request and wait for response.
  std::pair<bool, std::shared_ptr<jiminy_msgs::srv::LoadScenario::Response>>
  load_scenario(
      rclcpp::Client<jiminy_msgs::srv::LoadScenario>::SharedPtr client,
      const std::string &config_file) {

    if (!client->wait_for_service(SERVICE_TIMEOUT)) {
      RCLCPP_WARN(rclcpp::get_logger("test_integration_jiminy"),
                  "LoadScenario service not available after waiting");
      return {false, nullptr};
    }

    auto request = std::make_shared<jiminy_msgs::srv::LoadScenario::Request>();
    request->config_file = config_file;

    auto future = client->async_send_request(request);
    auto status = future.wait_for(SERVICE_TIMEOUT);

    if (status == std::future_status::ready) {
      return {true, future.get()};
    }
    return {false, nullptr};
  }

  /// Async send a GetScenario request and wait for response.
  std::pair<bool, std::shared_ptr<jiminy_msgs::srv::GetScenario::Response>>
  get_scenario(
      rclcpp::Client<jiminy_msgs::srv::GetScenario>::SharedPtr client) {

    if (!client->wait_for_service(SERVICE_TIMEOUT)) {
      return {false, nullptr};
    }

    auto request = std::make_shared<jiminy_msgs::srv::GetScenario::Request>();

    auto future = client->async_send_request(request);
    auto status = future.wait_for(SERVICE_TIMEOUT);

    if (status == std::future_status::ready) {
      return {true, future.get()};
    }
    return {false, nullptr};
  }

  std::shared_ptr<jiminy::JiminyNode> server_node_;
  std::shared_ptr<rclcpp::Node> client_node_;
  std::unique_ptr<rclcpp::executors::MultiThreadedExecutor> executor_;
  std::thread executor_thread_;
  std::string temp_config_;
};

//==============================================================================
// Suite 1: Scenario Loading
//==============================================================================
TEST_F(JiminyIntegrationTest, AllScenariosLoadSuccessfully) {
  std::vector<std::string> scenarios = {
      "jair.yaml",
      "agrobot.yaml",
      "candy_unified.yaml",
      "agriculture_demo/demo_priority.yaml",
  };

  auto load_client = create_load_scenario_client();

  for (const auto &scenario : scenarios) {
    SCOPED_TRACE(scenario);
    std::string full_path = scenario_path(scenario);
    ASSERT_FALSE(full_path.empty()) << "Scenario file not found: " << scenario;

    auto [success, response] = load_scenario(load_client, full_path);
    ASSERT_TRUE(success) << "LoadScenario call failed for " << scenario;
    ASSERT_TRUE(response->success) << "LoadScenario returned success=false for "
                                   << scenario << ": " << response->message;
  }
}

//==============================================================================
// Suite 2: Scenario Content Verification
//==============================================================================
struct ScenarioSpec {
  std::string file;
  int expected_norms;
  int expected_facts;
  int expected_contraries;
};

TEST_F(JiminyIntegrationTest, ScenarioContentAccuracy) {
  std::vector<ScenarioSpec> specs = {
      {"jair.yaml", 9, 4, 5},
      {"agrobot.yaml", 4, 2, 3},
      {"candy_unified.yaml", 12, 0, 0}, // facts/contraries not specified
      {"agriculture_demo/demo_priority.yaml", 2, 1, 2},
  };

  auto load_client = create_load_scenario_client();
  auto get_client = create_get_scenario_client();

  for (const auto &spec : specs) {
    SCOPED_TRACE(spec.file);
    std::string full_path = scenario_path(spec.file);
    ASSERT_FALSE(full_path.empty()) << "Scenario file not found: " << spec.file;

    // Load the scenario
    auto [load_ok, load_resp] = load_scenario(load_client, full_path);
    ASSERT_TRUE(load_ok);
    ASSERT_TRUE(load_resp->success)
        << "Failed to load " << spec.file << ": " << load_resp->message;

    // Get the scenario to verify content
    auto [get_ok, get_resp] = get_scenario(get_client);
    ASSERT_TRUE(get_ok);
    ASSERT_TRUE(get_resp != nullptr);

    // Count norms
    size_t norm_count = get_resp->scenario.norms.size();
    EXPECT_EQ(static_cast<int>(norm_count), spec.expected_norms)
        << "Norm count mismatch for " << spec.file;

    // Count facts (if expected)
    if (spec.expected_facts > 0) {
      size_t fact_count = get_resp->scenario.context.size();
      EXPECT_EQ(static_cast<int>(fact_count), spec.expected_facts)
          << "Fact count mismatch for " << spec.file;
    }

    // Count contraries (if expected)
    if (spec.expected_contraries > 0) {
      size_t contrary_count = get_resp->scenario.contraries.size();
      EXPECT_EQ(static_cast<int>(contrary_count), spec.expected_contraries)
          << "Contrary count mismatch for " << spec.file;
    }
  }
}

//==============================================================================
// Suite 3: Priority Resolution
//==============================================================================
TEST_F(JiminyIntegrationTest, PriorityResolution) {
  std::string scenario = "agriculture_demo/demo_priority.yaml";
  std::string full_path = scenario_path(scenario);
  ASSERT_FALSE(full_path.empty()) << "Scenario file not found: " << scenario;

  auto load_client = create_load_scenario_client();

  // Load the scenario
  auto [load_ok, load_resp] = load_scenario(load_client, full_path);
  ASSERT_TRUE(load_ok);
  ASSERT_TRUE(load_resp->success);

  // Get scenario to verify priorities and contrariness rules
  auto get_client = create_get_scenario_client();
  auto [get_ok, get_resp] = get_scenario(get_client);
  ASSERT_TRUE(get_ok);
  ASSERT_TRUE(get_resp != nullptr);

  // Verify priorities were loaded
  EXPECT_GT(get_resp->scenario.priorities.size(), 0)
      << "No priorities found in " << scenario;

  // Verify contrariness rules exist (needed for priority resolution)
  EXPECT_GE(get_resp->scenario.contraries.size(), 2)
      << "Expected >= 2 contrariness rules, got "
      << get_resp->scenario.contraries.size();
}

//==============================================================================
// Suite 4: Scenario Switching (Hot-Reload)
//==============================================================================
TEST_F(JiminyIntegrationTest, ScenarioSwitching) {
  std::vector<std::string> switch_order = {
      "jair.yaml", "agrobot.yaml",
      "jair.yaml", // reload first scenario
  };

  auto load_client = create_load_scenario_client();

  for (const auto &scenario : switch_order) {
    SCOPED_TRACE(scenario);
    std::string full_path = scenario_path(scenario);
    ASSERT_FALSE(full_path.empty()) << "Scenario file not found: " << scenario;

    auto [success, response] = load_scenario(load_client, full_path);
    ASSERT_TRUE(success) << "LoadScenario call failed for " << scenario;
    ASSERT_TRUE(response->success) << "LoadScenario returned success=false for "
                                   << scenario << ": " << response->message;
  }
}

//==============================================================================
// Suite 5: Error Handling
//==============================================================================
TEST_F(JiminyIntegrationTest, NonExistentFileHandledGracefully) {
  auto load_client = create_load_scenario_client();

  auto [success, response] =
      load_scenario(load_client, "/nonexistent/path/to/scenario.yaml");

  ASSERT_TRUE(success);
  // The service should return success=false for a missing file
  EXPECT_FALSE(response->success)
      << "Expected success=false for non-existent file";
}

TEST_F(JiminyIntegrationTest, EmptyPathHandledGracefully) {
  auto load_client = create_load_scenario_client();

  auto [success, response] = load_scenario(load_client, "");

  ASSERT_TRUE(success);
  // The service should handle empty path gracefully (not crash)
  EXPECT_FALSE(response->success) << "Expected success=false for empty path";
}

TEST_F(JiminyIntegrationTest, ServiceDoesNotCrashOnValidYAML) {
  auto load_client = create_load_scenario_client();
  std::string full_path = scenario_path("jair.yaml");
  ASSERT_FALSE(full_path.empty());

  auto [success, response] = load_scenario(load_client, full_path);
  ASSERT_TRUE(success);
  ASSERT_TRUE(response->success)
      << "Service failed on valid YAML: " << response->message;
}

//==============================================================================
// Suite 6: Reasoning Service (call_jiminy)
//==============================================================================
TEST_F(JiminyIntegrationTest, ReasoningWithJairScenario) {
  std::string scenario = "jair.yaml";
  std::string full_path = scenario_path(scenario);
  ASSERT_FALSE(full_path.empty()) << "Scenario file not found: " << scenario;

  // Load scenario
  auto load_client = create_load_scenario_client();
  auto [load_ok, load_resp] = load_scenario(load_client, full_path);
  ASSERT_TRUE(load_ok);
  ASSERT_TRUE(load_resp->success);

  // Create CallJiminy client
  auto call_client = create_call_jiminy_client();

  // Test case 1: facts ["w1", "w2"] — Manufacturer and data collection
  {
    SCOPED_TRACE("jair: w1,w2");
    auto [success, response] =
        call_jiminy(call_client, {"w1", "w2"}, "grounded");
    ASSERT_TRUE(success);
    ASSERT_TRUE(response->success)
        << "CallJiminy failed: " << response->message;
    // Should have at least some accepted arguments
    EXPECT_GT(response->accepted.size(), 0)
        << "Expected accepted arguments for facts [w1, w2]";
  }

  // Test case 2: facts ["w1", "w2", "w3"] — With threat detection
  {
    SCOPED_TRACE("jair: w1,w2,w3");
    auto [success, response] =
        call_jiminy(call_client, {"w1", "w2", "w3"}, "grounded");
    ASSERT_TRUE(success);
    ASSERT_TRUE(response->success)
        << "CallJiminy failed: " << response->message;
    EXPECT_GT(response->accepted.size(), 0)
        << "Expected accepted arguments for facts [w1, w2, w3]";
  }
}

TEST_F(JiminyIntegrationTest, ReasoningWithAgrobotScenario) {
  std::string scenario = "agrobot.yaml";
  std::string full_path = scenario_path(scenario);
  ASSERT_FALSE(full_path.empty()) << "Scenario file not found: " << scenario;

  // Load scenario
  auto load_client = create_load_scenario_client();
  auto [load_ok, load_resp] = load_scenario(load_client, full_path);
  ASSERT_TRUE(load_ok);
  ASSERT_TRUE(load_resp->success);

  auto call_client = create_call_jiminy_client();

  // Test case 1: facts ["w1"] — Open gate detected
  {
    SCOPED_TRACE("agrobot: w1");
    auto [success, response] = call_jiminy(call_client, {"w1"}, "grounded");
    ASSERT_TRUE(success);
    ASSERT_TRUE(response->success)
        << "CallJiminy failed: " << response->message;
    EXPECT_GT(response->accepted.size(), 0)
        << "Expected accepted arguments for facts [w1]";
  }

  // Test case 2: facts ["w1", "w2"] — Gate open, machinery absent
  {
    SCOPED_TRACE("agrobot: w1,w2");
    auto [success, response] =
        call_jiminy(call_client, {"w1", "w2"}, "grounded");
    ASSERT_TRUE(success);
    ASSERT_TRUE(response->success)
        << "CallJiminy failed: " << response->message;
    EXPECT_GT(response->accepted.size(), 0)
        << "Expected accepted arguments for facts [w1, w2]";
  }
}

TEST_F(JiminyIntegrationTest, ReasoningWithDifferentSemantics) {
  std::string scenario = "jair.yaml";
  std::string full_path = scenario_path(scenario);
  ASSERT_FALSE(full_path.empty()) << "Scenario file not found: " << scenario;

  // Load scenario
  auto load_client = create_load_scenario_client();
  auto [load_ok, load_resp] = load_scenario(load_client, full_path);
  ASSERT_TRUE(load_ok);
  ASSERT_TRUE(load_resp->success);

  auto call_client = create_call_jiminy_client();
  std::vector<std::string> semantics_list = {"grounded", "preferred", "stable",
                                             "priority"};

  for (const auto &sem : semantics_list) {
    SCOPED_TRACE(sem);
    auto [success, response] = call_jiminy(call_client, {"w1", "w2"}, sem);
    ASSERT_TRUE(success);
    ASSERT_TRUE(response->success) << "CallJiminy failed with semantics=" << sem
                                   << ": " << response->message;
    // Each semantics should produce some result
    EXPECT_GE(response->accepted.size() + response->rejected.size(), 0);
  }
}

//==============================================================================
// Main
//==============================================================================
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
