#include <gtest/gtest.h>

#include <chrono>
#include <fstream>
#include <memory>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/state.hpp>

#include "mini_jiminy/mini_jiminy_node.hpp"
#include "mini_jiminy_msgs/msg/semantics.hpp"
#include "mini_jiminy_msgs/srv/call_jiminy.hpp"
#include "mini_jiminy_msgs/srv/get_scenario.hpp"

using namespace std::chrono_literals;

//==============================================================================
// Test Fixture
//==============================================================================
class JiminyNodeTest : public ::testing::Test {
protected:
  void SetUp() override {
    rclcpp::init(0, nullptr);

    // Create a temporary YAML config file for testing
    create_test_config_file();
  }

  void TearDown() override {
    // Clean up the temporary config file
    std::remove(test_config_path_.c_str());
    rclcpp::shutdown();
  }

  void create_test_config_file() {
    test_config_path_ = "/tmp/test_jiminy_config.yaml";

    std::ofstream file(test_config_path_);
    file << R"(
description: "Test scenario for JiminyNode"

context:
  - id: "w1"
    description: "World fact 1"
  - id: "w2"
    description: "World fact 2"

norms:
  - id: "n1"
    body: ["w1"]
    conclusion: "i1"
    type: "c"
    stakeholder: "S1"
    description: "Constitutive norm 1"
  - id: "n2"
    body: ["i1"]
    conclusion: "d1"
    type: "r"
    stakeholder: "S1"
    description: "Regulative norm 1"
  - id: "n3"
    body: ["w2"]
    conclusion: "d2"
    type: "r"
    stakeholder: "S2"
    description: "Regulative norm 2"

contrariness:
  d1:
    opposes: ["d2"]
    description: "d1 opposes d2"
  d2:
    opposes: ["d1"]
    description: "d2 opposes d1"

priorities:
  d1:
    value: 10
    description: "High priority for d1"
  d2:
    value: 5
    description: "Lower priority for d2"
)";
    file.close();
  }

  std::string test_config_path_;
};

//==============================================================================
// Parameter Tests
//==============================================================================
TEST_F(JiminyNodeTest, DefaultConfigFileParameter) {
  auto node = std::make_shared<mini_jiminy::JiminyNode>();

  auto config_file = node->get_parameter("config_file").as_string();
  EXPECT_EQ(config_file, "config.yaml");
}

TEST_F(JiminyNodeTest, SetConfigFileParameter) {
  auto node = std::make_shared<mini_jiminy::JiminyNode>();

  node->set_parameter(rclcpp::Parameter("config_file", test_config_path_));

  auto config_file = node->get_parameter("config_file").as_string();
  EXPECT_EQ(config_file, test_config_path_);
}

//==============================================================================
// Service Tests
//==============================================================================
class JiminyNodeServiceTest : public JiminyNodeTest {
protected:
  void SetUp() override {
    JiminyNodeTest::SetUp();

    // Create and activate the node
    node_ = std::make_shared<mini_jiminy::JiminyNode>();
    node_->set_parameter(rclcpp::Parameter("config_file", test_config_path_));
    node_->configure();
    node_->activate();

    // Create a client node for testing
    client_node_ = rclcpp::Node::make_shared("test_client_node");
  }

  void TearDown() override {
    node_->deactivate();
    node_.reset();
    client_node_.reset();
    JiminyNodeTest::TearDown();
  }

  std::shared_ptr<mini_jiminy::JiminyNode> node_;
  std::shared_ptr<rclcpp::Node> client_node_;
};

TEST_F(JiminyNodeServiceTest, CallJiminyServiceExists) {
  auto client = client_node_->create_client<mini_jiminy_msgs::srv::CallJiminy>(
      "call_jiminy");

  // Wait for the service to be available
  EXPECT_TRUE(client->wait_for_service(2s));
}

TEST_F(JiminyNodeServiceTest, GetScenarioServiceExists) {
  auto client = client_node_->create_client<mini_jiminy_msgs::srv::GetScenario>(
      "get_scenario");

  // Wait for the service to be available
  EXPECT_TRUE(client->wait_for_service(2s));
}

TEST_F(JiminyNodeServiceTest, CallJiminyWithValidFacts) {
  auto client = client_node_->create_client<mini_jiminy_msgs::srv::CallJiminy>(
      "call_jiminy");

  ASSERT_TRUE(client->wait_for_service(2s));

  auto request = std::make_shared<mini_jiminy_msgs::srv::CallJiminy::Request>();
  request->facts = {"w1"};
  request->semantics.semantics = mini_jiminy_msgs::msg::Semantics::PRIORITY;

  auto future = client->async_send_request(request);

  // Spin until we get a response
  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(node_->get_node_base_interface());
  executor.add_node(client_node_);

  auto status = executor.spin_until_future_complete(future, 5s);
  ASSERT_EQ(status, rclcpp::FutureReturnCode::SUCCESS);

  auto response = future.get();
  EXPECT_TRUE(response->success);
  EXPECT_EQ(response->message, "Jiminy processing completed successfully.");
}

TEST_F(JiminyNodeServiceTest, CallJiminyWithInvalidFact) {
  auto client = client_node_->create_client<mini_jiminy_msgs::srv::CallJiminy>(
      "call_jiminy");

  ASSERT_TRUE(client->wait_for_service(2s));

  auto request = std::make_shared<mini_jiminy_msgs::srv::CallJiminy::Request>();
  request->facts = {"nonexistent_fact"};
  request->semantics.semantics = mini_jiminy_msgs::msg::Semantics::PRIORITY;

  auto future = client->async_send_request(request);

  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(node_->get_node_base_interface());
  executor.add_node(client_node_);

  auto status = executor.spin_until_future_complete(future, 5s);
  ASSERT_EQ(status, rclcpp::FutureReturnCode::SUCCESS);

  auto response = future.get();
  EXPECT_FALSE(response->success);
  EXPECT_TRUE(response->message.find("not found") != std::string::npos);
}

TEST_F(JiminyNodeServiceTest, CallJiminyWithDifferentSemantics) {
  auto client = client_node_->create_client<mini_jiminy_msgs::srv::CallJiminy>(
      "call_jiminy");

  ASSERT_TRUE(client->wait_for_service(2s));

  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(node_->get_node_base_interface());
  executor.add_node(client_node_);

  // Test each semantics type
  std::vector<std::string> semantics_types = {
      mini_jiminy_msgs::msg::Semantics::GROUNDED,
      mini_jiminy_msgs::msg::Semantics::PREFERRED,
      mini_jiminy_msgs::msg::Semantics::STABLE,
      mini_jiminy_msgs::msg::Semantics::PRIORITY};

  for (const auto &semantics : semantics_types) {
    auto request =
        std::make_shared<mini_jiminy_msgs::srv::CallJiminy::Request>();
    request->facts = {"w1"};
    request->semantics.semantics = semantics;

    auto future = client->async_send_request(request);
    auto status = executor.spin_until_future_complete(future, 5s);

    ASSERT_EQ(status, rclcpp::FutureReturnCode::SUCCESS)
        << "Failed for semantics: " << semantics;

    auto response = future.get();
    EXPECT_TRUE(response->success) << "Failed for semantics: " << semantics;
  }
}

TEST_F(JiminyNodeServiceTest, GetScenarioReturnsCompleteScenario) {
  auto client = client_node_->create_client<mini_jiminy_msgs::srv::GetScenario>(
      "get_scenario");

  ASSERT_TRUE(client->wait_for_service(2s));

  auto request =
      std::make_shared<mini_jiminy_msgs::srv::GetScenario::Request>();

  auto future = client->async_send_request(request);

  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(node_->get_node_base_interface());
  executor.add_node(client_node_);

  auto status = executor.spin_until_future_complete(future, 5s);
  ASSERT_EQ(status, rclcpp::FutureReturnCode::SUCCESS);

  auto response = future.get();

  // Check the scenario is populated
  EXPECT_EQ(response->scenario.description, "Test scenario for JiminyNode");
  EXPECT_EQ(response->scenario.context.size(), 2);
  EXPECT_EQ(response->scenario.norms.size(), 3);
  EXPECT_EQ(response->scenario.contraries.size(), 2);
  EXPECT_EQ(response->scenario.priorities.size(), 2);
}

TEST_F(JiminyNodeServiceTest, CallJiminyReturnsAcceptedAndRejected) {
  auto client = client_node_->create_client<mini_jiminy_msgs::srv::CallJiminy>(
      "call_jiminy");

  ASSERT_TRUE(client->wait_for_service(2s));

  // Provide both w1 and w2 to trigger conflicting norms
  auto request = std::make_shared<mini_jiminy_msgs::srv::CallJiminy::Request>();
  request->facts = {"w1", "w2"};
  request->semantics.semantics = mini_jiminy_msgs::msg::Semantics::PRIORITY;

  auto future = client->async_send_request(request);

  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(node_->get_node_base_interface());
  executor.add_node(client_node_);

  auto status = executor.spin_until_future_complete(future, 5s);
  ASSERT_EQ(status, rclcpp::FutureReturnCode::SUCCESS);

  auto response = future.get();
  EXPECT_TRUE(response->success);

  // With priority semantics, d1 (priority 10) should beat d2 (priority 5)
  // Check that there are some accepted and rejected norms
  size_t total_norms = response->accepted.size() + response->rejected.size();
  EXPECT_GT(total_norms, 0);
}

//==============================================================================
// YAML Loading Tests
//==============================================================================
class JiminyNodeYamlTest : public ::testing::Test {
protected:
  void SetUp() override { rclcpp::init(0, nullptr); }

  void TearDown() override { rclcpp::shutdown(); }

  std::string create_yaml_file(const std::string &content) {
    static int file_counter = 0;
    std::string path =
        "/tmp/test_yaml_" + std::to_string(file_counter++) + ".yaml";
    std::ofstream file(path);
    file << content;
    file.close();
    temp_files_.push_back(path);
    return path;
  }

  void cleanup_temp_files() {
    for (const auto &path : temp_files_) {
      std::remove(path.c_str());
    }
    temp_files_.clear();
  }

  std::vector<std::string> temp_files_;
};

TEST_F(JiminyNodeYamlTest, LoadYamlWithAllNormTypes) {
  std::string yaml = R"(
description: "All norm types test"

context:
  - id: "w"
    description: "World fact"

norms:
  - id: "c1"
    body: ["w"]
    conclusion: "i1"
    type: "constitutive"
    stakeholder: "S"
    description: "Constitutive"
  - id: "r1"
    body: ["w"]
    conclusion: "d1"
    type: "regulative"
    stakeholder: "S"
    description: "Regulative"
  - id: "p1"
    body: ["w"]
    conclusion: "p1"
    type: "permissive"
    stakeholder: "S"
    description: "Permissive"
)";

  auto path = create_yaml_file(yaml);
  auto node = std::make_shared<mini_jiminy::JiminyNode>();
  node->set_parameter(rclcpp::Parameter("config_file", path));
  node->configure();
  node->activate();

  // Create client to get scenario
  auto client_node = rclcpp::Node::make_shared("test_client");
  auto client = client_node->create_client<mini_jiminy_msgs::srv::GetScenario>(
      "get_scenario");

  ASSERT_TRUE(client->wait_for_service(2s));

  auto request =
      std::make_shared<mini_jiminy_msgs::srv::GetScenario::Request>();
  auto future = client->async_send_request(request);

  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(node->get_node_base_interface());
  executor.add_node(client_node);

  executor.spin_until_future_complete(future, 5s);

  auto response = future.get();
  EXPECT_EQ(response->scenario.norms.size(), 3);

  node->deactivate();
  cleanup_temp_files();
}

TEST_F(JiminyNodeYamlTest, LoadYamlWithShortNormTypes) {
  std::string yaml = R"yaml(
description: "Short norm types test"

context:
  - id: "w"
    description: "World fact"

norms:
  - id: "c1"
    body: ["w"]
    conclusion: "i1"
    type: "c"
    stakeholder: "S"
    description: "Constitutive (short)"
  - id: "r1"
    body: ["w"]
    conclusion: "d1"
    type: "r"
    stakeholder: "S"
    description: "Regulative (short)"
  - id: "p1"
    body: ["w"]
    conclusion: "p1"
    type: "p"
    stakeholder: "S"
    description: "Permissive (short)"
)yaml";

  auto path = create_yaml_file(yaml);
  auto node = std::make_shared<mini_jiminy::JiminyNode>();
  node->set_parameter(rclcpp::Parameter("config_file", path));
  node->configure();
  node->activate();

  auto client_node = rclcpp::Node::make_shared("test_client");
  auto client = client_node->create_client<mini_jiminy_msgs::srv::GetScenario>(
      "get_scenario");

  ASSERT_TRUE(client->wait_for_service(2s));

  auto request =
      std::make_shared<mini_jiminy_msgs::srv::GetScenario::Request>();
  auto future = client->async_send_request(request);

  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(node->get_node_base_interface());
  executor.add_node(client_node);

  executor.spin_until_future_complete(future, 5s);

  auto response = future.get();
  EXPECT_EQ(response->scenario.norms.size(), 3);

  node->deactivate();
  cleanup_temp_files();
}

TEST_F(JiminyNodeYamlTest, LoadYamlWithEmptyOptionalFields) {
  std::string yaml = R"(
description: "Minimal scenario"

context:
  - id: "w"
    description: "World fact"

norms:
  - id: "n1"
    body: ["w"]
    conclusion: "d1"
    type: "r"
    stakeholder: "S"
    description: "Single norm"
)";

  auto path = create_yaml_file(yaml);
  auto node = std::make_shared<mini_jiminy::JiminyNode>();
  node->set_parameter(rclcpp::Parameter("config_file", path));
  node->configure();
  node->activate();

  auto client_node = rclcpp::Node::make_shared("test_client");
  auto client = client_node->create_client<mini_jiminy_msgs::srv::GetScenario>(
      "get_scenario");

  ASSERT_TRUE(client->wait_for_service(2s));

  auto request =
      std::make_shared<mini_jiminy_msgs::srv::GetScenario::Request>();
  auto future = client->async_send_request(request);

  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(node->get_node_base_interface());
  executor.add_node(client_node);

  executor.spin_until_future_complete(future, 5s);

  auto response = future.get();
  EXPECT_TRUE(response->scenario.contraries.empty());
  EXPECT_TRUE(response->scenario.priorities.empty());

  node->deactivate();
  cleanup_temp_files();
}

TEST_F(JiminyNodeYamlTest, LoadInvalidYamlFileReturnsEmptyJiminy) {
  // Create an invalid YAML file that doesn't exist
  auto node = std::make_shared<mini_jiminy::JiminyNode>();
  node->set_parameter(
      rclcpp::Parameter("config_file", "/tmp/nonexistent_file.yaml"));
  node->configure();
  node->activate();

  auto client_node = rclcpp::Node::make_shared("test_client");
  auto client = client_node->create_client<mini_jiminy_msgs::srv::GetScenario>(
      "get_scenario");

  ASSERT_TRUE(client->wait_for_service(2s));

  auto request =
      std::make_shared<mini_jiminy_msgs::srv::GetScenario::Request>();
  auto future = client->async_send_request(request);

  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(node->get_node_base_interface());
  executor.add_node(client_node);

  executor.spin_until_future_complete(future, 5s);

  auto response = future.get();

  // Should return empty scenario due to file not found
  EXPECT_TRUE(response->scenario.context.empty());
  EXPECT_TRUE(response->scenario.norms.empty());

  node->deactivate();
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
