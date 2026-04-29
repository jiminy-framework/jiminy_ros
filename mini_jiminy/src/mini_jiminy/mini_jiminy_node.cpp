#include <yaml-cpp/yaml.h>

#include "mini_jiminy/mini_jiminy_node.hpp"
#include "mini_jiminy_msgs/msg/fact.hpp"
#include "mini_jiminy_msgs/msg/norm.hpp"
#include "mini_jiminy_msgs/msg/semantics.hpp"
#include "mini_jiminy_msgs/srv/call_jiminy.hpp"

using namespace mini_jiminy;

/**
 * @brief Convert a Norm structure to a mini_jiminy_msgs::msg::Norm message.
 * @param norm The Norm structure to convert.
 * @return The corresponding mini_jiminy_msgs::msg::Norm message.
 */
inline mini_jiminy_msgs::msg::Norm norm_to_msg(const Norm &norm) {
  mini_jiminy_msgs::msg::Norm norm_msg;
  norm_msg.id = norm.id;
  norm_msg.body = norm.body;
  norm_msg.conclusion = norm.conclusion;
  switch (norm.type) {
  case NormType::CONSTITUTIVE:
    norm_msg.type = mini_jiminy_msgs::msg::Norm::CONSTITUTIVE;
    break;
  case NormType::REGULATIVE:
    norm_msg.type = mini_jiminy_msgs::msg::Norm::REGULATIVE;
    break;
  case NormType::PERMISSIVE:
    norm_msg.type = mini_jiminy_msgs::msg::Norm::PERMISSIVE;
    break;
  }
  norm_msg.stakeholder = norm.stakeholder;
  norm_msg.description = norm.description;
  return norm_msg;
}

JiminyNode::JiminyNode() : rclcpp_lifecycle::LifecycleNode("mini_jiminy_node") {
  this->declare_parameter<std::string>("config_file", "config.yaml");
}

rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
JiminyNode::on_configure(const rclcpp_lifecycle::State &) {
  RCLCPP_INFO(this->get_logger(), "[%s] Configuring...", this->get_name());

  this->config_file_ = this->get_parameter("config_file").as_string();

  RCLCPP_INFO(this->get_logger(), "[%s] Configured", this->get_name());

  return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::
      CallbackReturn::SUCCESS;
}

rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
JiminyNode::on_activate(const rclcpp_lifecycle::State &) {
  RCLCPP_INFO(this->get_logger(), "[%s] Activating...", this->get_name());

  this->jiminy_ = load_jiminy(this->config_file_);
  this->jiminy_service_ =
      this->create_service<mini_jiminy_msgs::srv::CallJiminy>(
          "call_jiminy",
          std::bind(&JiminyNode::call_jiminy_service_callback, this,
                    std::placeholders::_1, std::placeholders::_2));
  this->get_scenario_service_ =
      this->create_service<mini_jiminy_msgs::srv::GetScenario>(
          "get_scenario",
          std::bind(&JiminyNode::get_scenario_service_callback, this,
                    std::placeholders::_1, std::placeholders::_2));

  RCLCPP_INFO(this->get_logger(), "[%s] Activated", this->get_name());
  return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::
      CallbackReturn::SUCCESS;
}

rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
JiminyNode::on_deactivate(const rclcpp_lifecycle::State &) {
  RCLCPP_INFO(this->get_logger(), "[%s] Deactivating...", this->get_name());

  this->jiminy_.reset();
  this->jiminy_service_.reset();
  this->get_scenario_service_.reset();

  RCLCPP_INFO(this->get_logger(), "[%s] Deactivated", this->get_name());
  return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::
      CallbackReturn::SUCCESS;
}

rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
JiminyNode::on_cleanup(const rclcpp_lifecycle::State &) {
  RCLCPP_INFO(this->get_logger(), "[%s] Cleaning up...", this->get_name());
  RCLCPP_INFO(this->get_logger(), "[%s] Cleaned up", this->get_name());
  return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::
      CallbackReturn::SUCCESS;
}

rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
JiminyNode::on_shutdown(const rclcpp_lifecycle::State &) {
  RCLCPP_INFO(this->get_logger(), "[%s] Shutting down...", this->get_name());
  RCLCPP_INFO(this->get_logger(), "[%s] Shut down", this->get_name());
  return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::
      CallbackReturn::SUCCESS;
}

std::unique_ptr<Jiminy>
JiminyNode::load_jiminy(const std::string &config_file) {
  RCLCPP_INFO(this->get_logger(), "Loading Jiminy configuration from %s",
              config_file.c_str());

  try {
    YAML::Node config = YAML::LoadFile(config_file);

    // Get description
    std::string description;
    if (config["description"]) {
      description = config["description"].as<std::string>();
    }

    // Parse facts
    std::map<std::string, Fact> facts;
    if (config["context"]) {
      for (const auto &fact_node : config["context"]) {
        Fact fact;
        fact.id = fact_node["id"].as<std::string>();
        fact.description = fact_node["description"].as<std::string>();
        facts[fact.id] = fact;
      }
    }

    // Parse norms
    std::map<std::string, Norm> norms;
    if (config["norms"]) {
      for (const auto &norm_node : config["norms"]) {
        Norm norm;
        norm.id = norm_node["id"].as<std::string>();
        norm.body = norm_node["body"].as<std::vector<std::string>>();
        norm.conclusion = norm_node["conclusion"].as<std::string>();
        std::string type_str = norm_node["type"].as<std::string>();
        if (type_str == "c" || type_str == "constitutive") {
          norm.type = NormType::CONSTITUTIVE;
        } else if (type_str == "r" || type_str == "regulative") {
          norm.type = NormType::REGULATIVE;
        } else if (type_str == "p" || type_str == "permissive") {
          norm.type = NormType::PERMISSIVE;
        } else {
          RCLCPP_WARN(this->get_logger(), "Unknown norm type: %s",
                      type_str.c_str());
          norm.type = NormType::CONSTITUTIVE; // default
        }
        norm.stakeholder = norm_node["stakeholder"].as<std::string>();
        norm.description = norm_node["description"].as<std::string>();
        norms[norm.id] = norm;
      }
    }

    // Parse contrariness
    std::map<std::string, Contrary> contraries;
    if (config["contrariness"]) {
      for (const auto &contrary_pair : config["contrariness"]) {
        std::string id = contrary_pair.first.as<std::string>();
        const auto &contrary_node = contrary_pair.second;
        Contrary contrary;
        contrary.id = id;
        contrary.contraries =
            contrary_node["opposes"].as<std::vector<std::string>>();
        contrary.description = contrary_node["description"].as<std::string>();
        contraries[id] = contrary;
      }
    }

    // Parse priorities
    std::map<std::string, Priority> priorities;
    if (config["priorities"]) {
      for (const auto &priority_pair : config["priorities"]) {
        std::string id = priority_pair.first.as<std::string>();
        const auto &priority_node = priority_pair.second;
        Priority priority;
        priority.id = id;
        priority.value = priority_node["value"].as<int>();
        priority.description = priority_node["description"].as<std::string>();
        priorities[id] = priority;
      }
    }

    return std::make_unique<Jiminy>(description, facts, norms, contraries,
                                    priorities);
  } catch (const YAML::Exception &e) {
    RCLCPP_ERROR(this->get_logger(), "Failed to load YAML config: %s",
                 e.what());
    // Return empty Jiminy instance on error
    return std::make_unique<Jiminy>(
        "", std::map<std::string, Fact>{}, std::map<std::string, Norm>{},
        std::map<std::string, Contrary>{}, std::map<std::string, Priority>{});
  }
}

void JiminyNode::call_jiminy_service_callback(
    const std::shared_ptr<mini_jiminy_msgs::srv::CallJiminy::Request> request,
    std::shared_ptr<mini_jiminy_msgs::srv::CallJiminy::Response> response) {
  RCLCPP_INFO(this->get_logger(), "Received Jiminy service request");

  if (!this->jiminy_) {
    RCLCPP_ERROR(this->get_logger(),
                 "Jiminy instance is not initialized. Cannot process request.");
    response->success = false;
    response->message = "Jiminy instance is not initialized.";
    return;
  }

  // Convert request config to Jiminy facts
  std::map<std::string, Fact> facts;
  for (const auto &fact_str : request->facts) {
    try {
      facts[fact_str] = this->jiminy_->get_fact(fact_str);
    } catch (const std::out_of_range &e) {
      RCLCPP_WARN(this->get_logger(), "Fact '%s' not found in Jiminy instance.",
                  fact_str.c_str());
      response->success = false;
      response->message =
          "Fact '" + fact_str + "' not found in Jiminy instance.";
      return;
    }
  }

  // Generate arguments
  auto arguments = this->jiminy_->generate_arguments(facts);

  // Compute extension based on requested semantics
  Semantics semantics;
  if (request->semantics.semantics ==
      mini_jiminy_msgs::msg::Semantics::GROUNDED) {
    semantics = Semantics::GROUNDED;
  } else if (request->semantics.semantics ==
             mini_jiminy_msgs::msg::Semantics::PREFERRED) {
    semantics = Semantics::PREFERRED;
  } else if (request->semantics.semantics ==
             mini_jiminy_msgs::msg::Semantics::STABLE) {
    semantics = Semantics::STABLE;
  } else if (request->semantics.semantics ==
             mini_jiminy_msgs::msg::Semantics::PRIORITY) {
    semantics = Semantics::PRIORITY;
  } else {
    RCLCPP_WARN(this->get_logger(), "Unknown semantics: %s, using PRIORITY",
                request->semantics.semantics.c_str());
    semantics = Semantics::PRIORITY;
  }

  auto [accepted, rejected] =
      this->jiminy_->compute_extension(arguments, semantics);

  // Populate response
  for (const auto &norm : accepted) {
    response->accepted.push_back(norm_to_msg(norm));
  }

  for (const auto &norm : rejected) {
    response->rejected.push_back(norm_to_msg(norm));
  }

  response->success = true;
  response->message = "Jiminy processing completed successfully.";
  RCLCPP_INFO(this->get_logger(),
              "Jiminy service request processed successfully");
}

void JiminyNode::get_scenario_service_callback(
    const std::shared_ptr<mini_jiminy_msgs::srv::GetScenario::Request> request,
    std::shared_ptr<mini_jiminy_msgs::srv::GetScenario::Response> response) {

  (void)request;

  RCLCPP_INFO(this->get_logger(), "Received GetScenario service request");

  if (!this->jiminy_) {
    RCLCPP_ERROR(this->get_logger(),
                 "Jiminy instance is not initialized. Cannot process request.");
    return;
  }

  // Set scenario description
  response->scenario.description = this->jiminy_->get_description();

  // Populate response scenario
  // Facts
  for (const auto &[id, fact] : this->jiminy_->get_facts()) {
    mini_jiminy_msgs::msg::Fact fact_msg;
    fact_msg.id = fact.id;
    fact_msg.description = fact.description;
    response->scenario.context.push_back(fact_msg);
  }

  // Norms
  for (const auto &[id, norm] : this->jiminy_->get_norms()) {
    response->scenario.norms.push_back(norm_to_msg(norm));
  }

  // Contrariness
  for (const auto &[id, contrary] : this->jiminy_->get_contraries()) {
    mini_jiminy_msgs::msg::Contrary contrary_msg;
    contrary_msg.id = contrary.id;
    contrary_msg.contraries = contrary.contraries;
    contrary_msg.description = contrary.description;
    response->scenario.contraries.push_back(contrary_msg);
  }

  // Priorities
  for (const auto &[id, priority] : this->jiminy_->get_priorities()) {
    mini_jiminy_msgs::msg::Priority priority_msg;
    priority_msg.id = priority.id;
    priority_msg.value = priority.value;
    priority_msg.description = priority.description;
    response->scenario.priorities.push_back(priority_msg);
  }

  RCLCPP_INFO(this->get_logger(),
              "GetScenario service request processed successfully");
}