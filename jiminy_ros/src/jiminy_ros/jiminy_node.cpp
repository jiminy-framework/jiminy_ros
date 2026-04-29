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

#include <yaml-cpp/yaml.h>

#include "jiminy_msgs/msg/fact.hpp"
#include "jiminy_msgs/msg/norm.hpp"
#include "jiminy_msgs/msg/semantics.hpp"
#include "jiminy_msgs/srv/call_jiminy.hpp"
#include "jiminy_ros/jiminy_node.hpp"

using namespace jiminy;

/**
 * @brief Convert a Norm structure to a jiminy_msgs::msg::Norm message.
 * @param norm The Norm structure to convert.
 * @return The corresponding jiminy_msgs::msg::Norm message.
 */
inline jiminy_msgs::msg::Norm norm_to_msg(const Norm &norm) {
  jiminy_msgs::msg::Norm norm_msg;
  norm_msg.id = norm.id;
  norm_msg.body = norm.body;
  norm_msg.conclusion = norm.conclusion;
  switch (norm.type) {
  case NormType::CONSTITUTIVE:
    norm_msg.type = jiminy_msgs::msg::Norm::CONSTITUTIVE;
    break;
  case NormType::REGULATIVE:
    norm_msg.type = jiminy_msgs::msg::Norm::REGULATIVE;
    break;
  case NormType::PERMISSIVE:
    norm_msg.type = jiminy_msgs::msg::Norm::PERMISSIVE;
    break;
  }
  norm_msg.stakeholder = norm.stakeholder;
  norm_msg.description = norm.description;
  return norm_msg;
}

JiminyNode::JiminyNode() : rclcpp_lifecycle::LifecycleNode("jiminy_node") {
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
  this->jiminy_service_ = this->create_service<jiminy_msgs::srv::CallJiminy>(
      "call_jiminy", std::bind(&JiminyNode::call_jiminy_service_callback, this,
                               std::placeholders::_1, std::placeholders::_2));
  this->get_scenario_service_ =
      this->create_service<jiminy_msgs::srv::GetScenario>(
          "get_scenario",
          std::bind(&JiminyNode::get_scenario_service_callback, this,
                    std::placeholders::_1, std::placeholders::_2));
  this->load_scenario_service_ =
      this->create_service<jiminy_msgs::srv::LoadScenario>(
          "load_scenario",
          std::bind(&JiminyNode::load_scenario_service_callback, this,
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
  this->load_scenario_service_.reset();

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

/**
 * @brief Helper to parse a single norm node into a Norm structure.
 */
inline Norm parse_norm_node(const YAML::Node &node, NormType type) {
  Norm norm;
  norm.id = node["id"].as<std::string>();
  norm.body = node["body"].as<std::vector<std::string>>();
  norm.conclusion = node["conclusion"].as<std::string>();
  norm.type = type;
  norm.stakeholder = node["stakeholder"].as<std::string>();
  norm.description = node["description"].as<std::string>();
  return norm;
}

/**
 * @brief Parse norms from a YAML section with a given type.
 */
inline void parse_norms_section(const YAML::Node &section,
                                std::map<std::string, Norm> &norms,
                                NormType type) {
  if (!section)
    return;
  for (const auto &node : section) {
    norms[node["id"].as<std::string>()] = parse_norm_node(node, type);
  }
}

/**
 * @brief Parse old-format norms with type field.
 */
inline void parse_old_norms_section(const YAML::Node &section,
                                    std::map<std::string, Norm> &norms) {
  if (!section)
    return;
  for (const auto &node : section) {
    Norm norm;
    norm.id = node["id"].as<std::string>();
    norm.body = node["body"].as<std::vector<std::string>>();
    norm.conclusion = node["conclusion"].as<std::string>();
    std::string type_str = node["type"].as<std::string>();
    if (type_str == "c" || type_str == "constitutive") {
      norm.type = NormType::CONSTITUTIVE;
    } else if (type_str == "r" || type_str == "regulative") {
      norm.type = NormType::REGULATIVE;
    } else if (type_str == "p" || type_str == "permissive") {
      norm.type = NormType::PERMISSIVE;
    } else {
      RCLCPP_WARN(norms.empty() ? rclcpp::get_logger("jiminy")
                                : rclcpp::get_logger("jiminy"),
                  "Unknown norm type: %s", type_str.c_str());
      norm.type = NormType::CONSTITUTIVE;
    }
    norm.stakeholder = node["stakeholder"].as<std::string>();
    norm.description = node["description"].as<std::string>();
    norms[norm.id] = norm;
  }
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

    // Parse norms (support multiple formats)
    std::map<std::string, Norm> norms;

    // Old format: "norms" section with type field
    parse_old_norms_section(config["norms"], norms);

    // New format: separate sections with implicit types
    parse_norms_section(config["constitutive_norms"], norms,
                        NormType::CONSTITUTIVE);
    parse_norms_section(config["regulative_norms"], norms,
                        NormType::REGULATIVE);
    parse_norms_section(config["permissions"], norms, NormType::PERMISSIVE);

    // Parse contrariness
    std::map<std::string, Contrary> contraries;
    if (config["contrariness"]) {
      for (const auto &contrary_pair : config["contrariness"]) {
        std::string id = contrary_pair.first.as<std::string>();
        const auto &contrary_node = contrary_pair.second;
        Contrary contrary;
        contrary.id = id;
        // Support both "opposes" and "contraries" keys for compatibility
        if (contrary_node["opposes"]) {
          contrary.contraries =
              contrary_node["opposes"].as<std::vector<std::string>>();
        } else if (contrary_node["contraries"]) {
          contrary.contraries =
              contrary_node["contraries"].as<std::vector<std::string>>();
        }
        // Handle optional description field
        if (contrary_node["description"]) {
          contrary.description = contrary_node["description"].as<std::string>();
        } else {
          contrary.description = "";
        }
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
        // Handle optional value and description fields
        if (priority_node["value"]) {
          priority.value = priority_node["value"].as<int>();
        } else {
          priority.value = 0;
        }
        if (priority_node["description"]) {
          priority.description = priority_node["description"].as<std::string>();
        } else {
          priority.description = "";
        }
        priorities[id] = priority;
      }
    }

    // Parse base priorities (stakeholder authority levels)
    std::map<std::string, int> base_priorities;
    if (config["base_priorities"]) {
      for (const auto &bp_pair : config["base_priorities"]) {
        std::string stakeholder = bp_pair.first.as<std::string>();
        int value = bp_pair.second.as<int>();
        base_priorities[stakeholder] = value;
      }
    }

    // If priorities is empty but base_priorities exists, compute default
    // priorities by assigning each conclusion the base priority of its
    // supporting stakeholder
    if (priorities.empty() && !base_priorities.empty()) {
      for (const auto &[norm_id, norm] : norms) {
        int stakeholder_priority = base_priorities.count(norm.stakeholder)
                                       ? base_priorities[norm.stakeholder]
                                       : 1;
        if (priorities.count(norm.conclusion) == 0) {
          priorities[norm.conclusion] = {norm.conclusion, stakeholder_priority,
                                         "Derived from stakeholder " +
                                             norm.stakeholder};
        } else {
          // Keep the maximum priority if multiple norms support same conclusion
          priorities[norm.conclusion].value =
              std::max(priorities[norm.conclusion].value, stakeholder_priority);
        }
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
    const std::shared_ptr<jiminy_msgs::srv::CallJiminy::Request> request,
    std::shared_ptr<jiminy_msgs::srv::CallJiminy::Response> response) {
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
  if (request->semantics.semantics == jiminy_msgs::msg::Semantics::GROUNDED) {
    semantics = Semantics::GROUNDED;
  } else if (request->semantics.semantics ==
             jiminy_msgs::msg::Semantics::PREFERRED) {
    semantics = Semantics::PREFERRED;
  } else if (request->semantics.semantics ==
             jiminy_msgs::msg::Semantics::STABLE) {
    semantics = Semantics::STABLE;
  } else if (request->semantics.semantics ==
             jiminy_msgs::msg::Semantics::PRIORITY) {
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
    const std::shared_ptr<jiminy_msgs::srv::GetScenario::Request> request,
    std::shared_ptr<jiminy_msgs::srv::GetScenario::Response> response) {

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
    jiminy_msgs::msg::Fact fact_msg;
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
    jiminy_msgs::msg::Contrary contrary_msg;
    contrary_msg.id = contrary.id;
    contrary_msg.contraries = contrary.contraries;
    contrary_msg.description = contrary.description;
    response->scenario.contraries.push_back(contrary_msg);
  }

  // Priorities
  for (const auto &[id, priority] : this->jiminy_->get_priorities()) {
    jiminy_msgs::msg::Priority priority_msg;
    priority_msg.id = priority.id;
    priority_msg.value = priority.value;
    priority_msg.description = priority.description;
    response->scenario.priorities.push_back(priority_msg);
  }

  RCLCPP_INFO(this->get_logger(),
              "GetScenario service request processed successfully");
}

void JiminyNode::load_scenario_service_callback(
    const std::shared_ptr<jiminy_msgs::srv::LoadScenario::Request> request,
    std::shared_ptr<jiminy_msgs::srv::LoadScenario::Response> response) {

  RCLCPP_INFO(this->get_logger(),
              "Received LoadScenario service request for: %s",
              request->config_file.c_str());

  try {
    // Attempt to load the new scenario
    auto new_jiminy = load_jiminy(request->config_file);

    if (!new_jiminy) {
      response->success = false;
      response->message = "Failed to load Jiminy instance from file";
      RCLCPP_ERROR(this->get_logger(), "%s", response->message.c_str());
      return;
    }

    // Replace the current instance
    this->jiminy_ = std::move(new_jiminy);
    this->config_file_ = request->config_file;

    // Populate response scenario
    response->scenario.description = this->jiminy_->get_description();

    // Facts
    for (const auto &[id, fact] : this->jiminy_->get_facts()) {
      jiminy_msgs::msg::Fact fact_msg;
      fact_msg.id = fact.id;
      fact_msg.description = fact.description;
      response->scenario.context.push_back(fact_msg);
    }

    // Norms
    for (const auto &[id, norm] : this->jiminy_->get_norms()) {
      response->scenario.norms.push_back(norm_to_msg(norm));
    }

    // Contraries
    for (const auto &[id, contrary] : this->jiminy_->get_contraries()) {
      jiminy_msgs::msg::Contrary contrary_msg;
      contrary_msg.id = contrary.id;
      contrary_msg.contraries = contrary.contraries;
      contrary_msg.description = contrary.description;
      response->scenario.contraries.push_back(contrary_msg);
    }

    // Priorities
    for (const auto &[id, priority] : this->jiminy_->get_priorities()) {
      jiminy_msgs::msg::Priority priority_msg;
      priority_msg.id = priority.id;
      priority_msg.value = priority.value;
      priority_msg.description = priority.description;
      response->scenario.priorities.push_back(priority_msg);
    }

    response->success = true;
    response->message =
        "Scenario loaded successfully from " + request->config_file;
    RCLCPP_INFO(this->get_logger(),
                "LoadScenario service request processed successfully");

  } catch (const YAML::Exception &e) {
    response->success = false;
    response->message = std::string("YAML parsing error: ") + e.what();
    RCLCPP_ERROR(this->get_logger(), "%s", response->message.c_str());

  } catch (const std::exception &e) {
    response->success = false;
    response->message = std::string("Error: ") + e.what();
    RCLCPP_ERROR(this->get_logger(), "%s", response->message.c_str());
  }
}