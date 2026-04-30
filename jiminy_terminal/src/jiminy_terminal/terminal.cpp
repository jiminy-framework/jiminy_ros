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

#include <algorithm>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <memory>
#include <readline/history.h>
#include <readline/readline.h>
#include <regex>
#include <sstream>
#include <stdio.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include "rclcpp/rclcpp.hpp"

#include "jiminy_msgs/srv/call_jiminy.hpp"
#include "jiminy_msgs/srv/get_scenario.hpp"
#include "jiminy_msgs/srv/load_scenario.hpp"
#include "jiminy_terminal/terminal.hpp"

namespace jiminy_terminal {

Terminal *Terminal::current_terminal_ = nullptr;

std::vector<std::string> tokenize(const std::string &text,
                                  const std::string delim) {
  std::vector<std::string> tokens;
  std::string::size_type start = 0;
  std::string::size_type end = text.find_first_of(delim);

  while (end != std::string::npos) {
    std::string token = text.substr(start, end - start);
    if (!token.empty()) {
      tokens.push_back(token);
    }
    start = end + 1;
    end = text.find_first_of(delim, start);
  }

  std::string token = text.substr(start);
  if (!token.empty()) {
    tokens.push_back(token);
  }

  return tokens;
}

void pop_front(std::vector<std::string> &tokens) {
  if (!tokens.empty()) {
    tokens.erase(tokens.begin());
  }
}

// Helper: expand ~ to home directory path
static std::string expand_tilde(const std::string &path) {
  if (!path.empty() && path[0] == '~') {
    const char *home = getenv("HOME");
    if (home) {
      return std::string(home) + path.substr(1);
    }
  }
  return path;
}

// Helper: list files/directories matching a prefix (for "load" completion)
static std::vector<std::string> list_files_matching(const std::string &prefix) {
  std::vector<std::string> results;

  // Expand ~ to home directory
  std::string expanded_prefix = expand_tilde(prefix);

  // Determine directory to search and the remaining pattern
  std::string dir;
  std::string pattern;

  size_t last_slash = expanded_prefix.rfind('/');

  if (last_slash == std::string::npos) {
    // No slash in prefix — search current directory
    dir = ".";
    pattern = expanded_prefix;
  } else if (last_slash == 0 && expanded_prefix.size() == 1) {
    // Prefix is exactly "/" — search root
    dir = "/";
    pattern = "";
  } else if (expanded_prefix.back() == '/') {
    // Prefix ends with '/' — the prefix itself is the directory
    // Strip trailing '/' so we can safely do dir + "/" + name later
    dir = expanded_prefix.substr(0, expanded_prefix.size() - 1);
    pattern = "";
  } else if (last_slash == 0) {
    // Prefix starts with '/' but has no other slash (e.g., "/home")
    dir = "/";
    pattern = expanded_prefix.substr(1);
  } else {
    // Prefix contains '/' in the middle (e.g., "/home/miguel")
    dir = expanded_prefix.substr(0, last_slash);
    pattern = expanded_prefix.substr(last_slash + 1);
  }

  DIR *dp = opendir(dir.c_str());
  if (!dp)
    return results;

  struct dirent *entry;
  while ((entry = readdir(dp)) != nullptr) {
    std::string name = entry->d_name;
    if (name == "." || name == "..")
      continue;
    if (name.size() < pattern.size())
      continue;
    if (name.substr(0, pattern.size()) != pattern)
      continue;

    std::string full_path;
    if (dir == ".") {
      full_path = name;
    } else if (dir == "/") {
      full_path = "/" + name;
    } else {
      full_path = dir + "/" + name;
    }

    struct stat st;
    if (stat(full_path.c_str(), &st) == 0) {
      if (S_ISDIR(st.st_mode)) {
        results.push_back(full_path + "/");
      } else if (S_ISREG(st.st_mode)) {
        results.push_back(full_path);
      }
    }
  }
  closedir(dp);

  std::sort(results.begin(), results.end());
  return results;
}

// LCOV_EXCL_START
char *Terminal::completion_generator(const char *text, int state) {
  static std::vector<std::string> matches;
  static size_t match_index = 0;

  if (state == 0) {
    matches.clear();
    match_index = 0;

    std::string textstr = std::string(text);
    std::string line = rl_line_buffer;
    auto current_text = tokenize(line);
    std::vector<std::string> candidates;

    // Determine which command is being typed
    std::string cmd = current_text.empty() ? "" : current_text[0];

    // Detect whether the command token is "complete" (user typed a space
    // after it or there are more tokens).  If the command is incomplete, we
    // are still typing the command name and should suggest top-level commands.
    bool command_complete = false;
    if (current_text.size() > 1) {
      command_complete = true;
    } else if (current_text.size() == 1 &&
               (!line.empty() && (line.back() == ' ' || line.back() == '\t'))) {
      command_complete = true;
    }

    if (cmd == "load" && command_complete) {
      // File path completion for "load" command.
      // With "/" removed from word break characters, `textstr` is the full
      // path prefix being typed (e.g., "/home/user/scenarios/agrobot").

      // Disable the trailing space that readline appends after completion.
      rl_completion_append_character = '\0';

      auto files = list_files_matching(textstr);
      matches.insert(matches.end(), files.begin(), files.end());

    } else if (cmd == "call" && command_complete) {
      std::vector<std::string> semantics = {"grounded", "preferred", "stable",
                                            "priority"};

      if (current_text.size() <= 2) {
        // User typed "call " or "call grounded " — suggest semantics or facts
        if (current_text.size() == 1) {
          // Only "call" typed — suggest semantics
          candidates = semantics;
        } else {
          // Check if token[1] is a complete semantics match
          std::string second_token = current_text[1];
          bool semantics_complete = false;
          for (const auto &s : semantics) {
            if (s == second_token) {
              semantics_complete = true;
              break;
            }
          }

          if (semantics_complete) {
            // Semantics is complete — suggest facts
            if (current_terminal_->cached_facts_.empty()) {
              if (current_terminal_->get_scenario_client_ &&
                  current_terminal_->get_scenario_client_->wait_for_service(
                      std::chrono::milliseconds(200))) {
                auto req =
                    std::make_shared<jiminy_msgs::srv::GetScenario::Request>();
                auto fut =
                    current_terminal_->get_scenario_client_->async_send_request(
                        req);
                try {
                  auto res = fut.get();
                  for (const auto &fact : res->scenario.context) {
                    current_terminal_->cached_facts_.push_back(fact.id);
                  }
                } catch (...) {
                }
              }
            }
            candidates.insert(candidates.end(),
                              current_terminal_->cached_facts_.begin(),
                              current_terminal_->cached_facts_.end());
          } else {
            // Still completing semantics
            candidates = semantics;
          }
        }
      } else {
        // current_text.size() > 2 — completing additional facts
        if (current_terminal_->cached_facts_.empty()) {
          if (current_terminal_->get_scenario_client_ &&
              current_terminal_->get_scenario_client_->wait_for_service(
                  std::chrono::milliseconds(200))) {
            auto req =
                std::make_shared<jiminy_msgs::srv::GetScenario::Request>();
            auto fut =
                current_terminal_->get_scenario_client_->async_send_request(
                    req);
            try {
              auto res = fut.get();
              for (const auto &fact : res->scenario.context) {
                current_terminal_->cached_facts_.push_back(fact.id);
              }
            } catch (...) {
            }
          }
        }
        // Suggest facts that haven't been used yet in this command
        std::vector<std::string> used_facts(current_text.begin() + 2,
                                            current_text.end());
        for (const auto &f : current_terminal_->cached_facts_) {
          bool already_used = false;
          for (const auto &u : used_facts) {
            if (u == f) {
              already_used = true;
              break;
            }
          }
          if (!already_used) {
            candidates.push_back(f);
          }
        }
      }

    } else {
      // Top-level commands
      candidates = {"help", "load", "get-scenario", "call", "quit", "exit"};

      for (const auto &candidate : candidates) {
        if (candidate.substr(0, textstr.size()) == textstr) {
          matches.push_back(candidate);
        }
      }
      return state < (int)matches.size() ? strdup(matches[state].c_str())
                                         : nullptr;
    }

    // For load/call: filter matches by textstr prefix
    if (!matches.empty()) {
      // File/path matches are already filtered by list_files_matching
      // Nothing extra to do
    } else {
      for (const auto &candidate : candidates) {
        if (candidate.substr(0, textstr.size()) == textstr) {
          matches.push_back(candidate);
        }
      }
    }
  }

  if (match_index < matches.size()) {
    return strdup(matches[match_index++].c_str());
  }
  return nullptr;
}
// LCOV_EXCL_STOP

char **Terminal::completer(const char *text, int /* start */, int /* end */) {
  rl_attempted_completion_over = 1;
  return rl_completion_matches(text, completion_generator);
}

Terminal::Terminal()
    : rclcpp::Node("jiminy_terminal_" + std::to_string(static_cast<int>(
                                            rclcpp::Clock().now().seconds()))) {
  current_terminal_ = this;

  // Create service clients
  call_client_ =
      this->create_client<jiminy_msgs::srv::CallJiminy>("call_jiminy");
  get_scenario_client_ =
      this->create_client<jiminy_msgs::srv::GetScenario>("get_scenario");
  load_scenario_client_ =
      this->create_client<jiminy_msgs::srv::LoadScenario>("load_scenario");

  cached_facts_.clear();

  RCLCPP_INFO(this->get_logger(), "Jiminy Terminal initialized");
}

void Terminal::run_console() {
  rl_bind_key('\t', rl_complete);
  rl_attempted_completion_function = completer;

  // Remove "/" from word break characters so file paths are treated as
  // single words during tab completion
  if (rl_completer_word_break_characters) {
    std::string breaks = rl_completer_word_break_characters;
    breaks.erase(std::remove(breaks.begin(), breaks.end(), '/'), breaks.end());
    rl_completer_word_break_characters = strdup(breaks.c_str());
  }

  std::string command;
  while (true) {
    char *input = readline("\njiminy> ");
    if (!input) {
      break; // EOF
    }

    command = std::string(input);
    free(input);

    if (command.empty()) {
      continue;
    }

    // Add to history
    add_history(command.c_str());

    // Clean command
    clean_command(command);

    // Process command
    std::ostringstream os;
    process_command(command, os);
    std::cout << os.str() << std::flush;
  }

  std::cout << "\nShutting down Jiminy Terminal..." << std::endl;
}

void Terminal::clean_command(std::string &command) {
  // Remove leading/trailing whitespace
  size_t start = command.find_first_not_of(" \t\n\r");
  size_t end = command.find_last_not_of(" \t\n\r");

  if (start == std::string::npos) {
    command = "";
  } else {
    command = command.substr(start, end - start + 1);
  }
}

void Terminal::process_command(std::string &command, std::ostringstream &os) {
  std::vector<std::string> tokens = tokenize(command);

  if (tokens.empty()) {
    return;
  }

  if (tokens[0] == "help") {
    pop_front(tokens);
    this->process_help(tokens, os);
  } else if (tokens[0] == "load") {
    pop_front(tokens);
    this->process_load(tokens, os);
  } else if (tokens[0] == "get-scenario") {
    pop_front(tokens);
    this->process_get_scenario(tokens, os);
  } else if (tokens[0] == "call") {
    pop_front(tokens);
    this->process_call(tokens, os);
  } else if (tokens[0] == "quit" || tokens[0] == "exit") {
    os << "Goodbye!" << std::endl;
    // Signal to exit
    _exit(0);
  } else {
    os << "Unknown command: " << tokens[0] << std::endl;
    os << "Type 'help' for available commands." << std::endl;
  }
}

void Terminal::process_help(std::vector<std::string> & /* command */,
                            std::ostringstream &os) {
  os << "=== Jiminy Terminal Help ===" << std::endl;
  os << std::endl;
  os << "Available commands:" << std::endl;
  os << "  help                  Show this help message" << std::endl;
  os << "  load <file>           Load a scenario from a YAML config file"
     << std::endl;
  os << "  get-scenario          Get the current scenario" << std::endl;
  os << "  call <semantics> [facts...]  Call Jiminy with semantics and "
        "optional facts"
     << std::endl;
  os << "  quit / exit           Exit the terminal" << std::endl;
  os << std::endl;
  os << "=== call command details ===" << std::endl;
  os << std::endl;
  os << "  Syntax:  call <semantics> [fact_id1] [fact_id2] ..." << std::endl;
  os << std::endl;
  os << "  Semantics (choose one):" << std::endl;
  os << "    grounded    - Compute the grounded extension (all unattacked "
        "arguments)"
     << std::endl;
  os << "    preferred   - Compute preferred extensions (maximal conflict-free "
        "sets)"
     << std::endl;
  os << "    stable      - Compute stable extensions (attack all outside "
        "arguments)"
     << std::endl;
  os << "    priority    - Resolve conflicts using priority ordering"
     << std::endl;
  os << std::endl;
  os << "  Facts:" << std::endl;
  os << "    Each fact is a space-separated context ID from the loaded "
        "scenario."
     << std::endl;
  os << "    Use the IDs defined in the YAML 'contexto' section (e.g., w1, "
        "w2, w3)."
     << std::endl;
  os << "    Run 'get-scenario' to see available fact IDs." << std::endl;
  os << "    Press TAB after 'call <semantics> ' to autocomplete available "
        "facts."
     << std::endl;
  os << std::endl;
  os << "  Examples:" << std::endl;
  os << "    call grounded                    # No extra facts" << std::endl;
  os << "    call grounded w1                 # One fact" << std::endl;
  os << "    call grounded w1 w2 w3           # Multiple facts" << std::endl;
  os << "    call preferred w1 w3             # Preferred semantics with 2 "
        "facts"
     << std::endl;
  os << std::endl;
  os << "=== load command details ===" << std::endl;
  os << std::endl;
  os << "  Syntax:  load <path_to_yaml_file>" << std::endl;
  os << "  Press TAB to autocomplete file paths." << std::endl;
  os << std::endl;
  os << "  Examples:" << std::endl;
  os << "    load scenarios/agrobot.yaml" << std::endl;
  os << "    load scenarios/jair.yaml" << std::endl;
  os << std::endl;
  os << "Use TAB for command completion at any time." << std::endl;
}

void Terminal::process_load(std::vector<std::string> &command,
                            std::ostringstream &os) {
  if (command.size() != 1) {
    os << "Usage: load <config_file>" << std::endl;
    os << "  Loads a scenario from a YAML configuration file." << std::endl;
    return;
  }

  std::string config_file = command[0];
  os << "Loading scenario from: " << config_file << "..." << std::endl;

  if (!load_scenario_client_->wait_for_service(std::chrono::seconds(5))) {
    os << "Error: load_scenario service not available" << std::endl;
    return;
  }

  auto request = std::make_shared<jiminy_msgs::srv::LoadScenario::Request>();
  request->config_file = config_file;

  auto future = load_scenario_client_->async_send_request(request);
  try {
    auto result = future.get();
    if (result->success) {
      os << "Scenario loaded successfully!" << std::endl;
      os << "Description: " << result->scenario.description << std::endl;
      os << "Context facts: " << result->scenario.context.size() << std::endl;
      os << "Norms: " << result->scenario.norms.size() << std::endl;
      os << "Contraries: " << result->scenario.contraries.size() << std::endl;
      os << "Priorities: " << result->scenario.priorities.size() << std::endl;
      os << "Base priorities: " << result->scenario.base_priorities.size()
         << std::endl;
      os << "Meta priorities: " << result->scenario.meta_priorities.size()
         << std::endl;

      // Cache fact IDs for autocompletion
      cached_facts_.clear();
      for (const auto &fact : result->scenario.context) {
        cached_facts_.push_back(fact.id);
      }
    } else {
      os << "Failed to load scenario: " << result->message << std::endl;
    }
  } catch (const std::exception &e) {
    os << "Error calling service: " << e.what() << std::endl;
  }
}

void Terminal::process_get_scenario(std::vector<std::string> &command,
                                    std::ostringstream &os) {
  if (!command.empty()) {
    os << "Usage: get-scenario" << std::endl;
    os << "  Gets the current scenario loaded in Jiminy." << std::endl;
    return;
  }

  os << "Fetching current scenario..." << std::endl;

  if (!get_scenario_client_->wait_for_service(std::chrono::seconds(5))) {
    os << "Error: get_scenario service not available" << std::endl;
    return;
  }

  auto request = std::make_shared<jiminy_msgs::srv::GetScenario::Request>();

  auto future = get_scenario_client_->async_send_request(request);
  try {
    auto result = future.get();
    auto scenario = result->scenario;

    // Cache fact IDs for autocompletion
    cached_facts_.clear();
    for (const auto &fact : scenario.context) {
      cached_facts_.push_back(fact.id);
    }

    os << "=== Current Scenario ===" << std::endl;
    os << "Description: " << scenario.description << std::endl;
    os << std::endl;

    // Context
    os << "Context (" << scenario.context.size() << "):" << std::endl;
    for (const auto &fact : scenario.context) {
      os << "  - " << fact.id << ": " << fact.description << std::endl;
    }
    os << std::endl;

    // Norms
    os << "Norms (" << scenario.norms.size() << "):" << std::endl;
    for (const auto &norm : scenario.norms) {
      std::string type_str = "unknown";
      switch (norm.type) {
      case jiminy_msgs::msg::Norm::CONSTITUTIVE:
        type_str = "constitutive";
        break;
      case jiminy_msgs::msg::Norm::REGULATIVE:
        type_str = "regulative";
        break;
      case jiminy_msgs::msg::Norm::PERMISSIVE:
        type_str = "permissive";
        break;
      }
      os << "  - [" << norm.id << "] (" << type_str << "): ";
      os << "conclusion=" << norm.conclusion;
      if (!norm.stakeholder.empty()) {
        os << ", stakeholder=" << norm.stakeholder;
      }
      os << std::endl;
      if (!norm.description.empty()) {
        os << "    " << norm.description << std::endl;
      }
    }
    os << std::endl;

    // Contraries
    os << "Contraries (" << scenario.contraries.size() << "):" << std::endl;
    for (const auto &contrary : scenario.contraries) {
      os << "  - " << contrary.id << ": [";
      for (size_t i = 0; i < contrary.contraries.size(); i++) {
        os << contrary.contraries[i];
        if (i < contrary.contraries.size() - 1)
          os << ", ";
      }
      os << "]" << std::endl;
    }
    os << std::endl;

    // Priorities
    os << "Priorities (" << scenario.priorities.size() << "):" << std::endl;
    for (const auto &priority : scenario.priorities) {
      os << "  - " << priority.id << ": value=" << priority.value;
      if (!priority.description.empty()) {
        os << " (" << priority.description << ")";
      }
      os << std::endl;
    }
  } catch (const std::exception &e) {
    os << "Error calling service: " << e.what() << std::endl;
  }
}

void Terminal::process_call(std::vector<std::string> &command,
                            std::ostringstream &os) {
  if (command.empty()) {
    os << "Usage: call <semantics> [fact_id1] [fact_id2] ..." << std::endl;
    os << std::endl;
    os << "  Semantics: grounded, preferred, stable, priority" << std::endl;
    os << "  Facts: space-separated context IDs (e.g., w1, w2, w3)"
       << std::endl;
    os << "  Run 'get-scenario' to see available fact IDs." << std::endl;
    os << "  Press TAB after 'call <semantics> ' for autocomplete."
       << std::endl;
    return;
  }

  std::string semantics = command[0];
  std::vector<std::string> facts(command.begin() + 1, command.end());

  os << "Calling Jiminy with semantics: " << semantics;
  if (!facts.empty()) {
    os << ", facts: [";
    for (size_t i = 0; i < facts.size(); i++) {
      os << facts[i];
      if (i < facts.size() - 1)
        os << ", ";
    }
    os << "]";
  }
  os << "..." << std::endl;

  if (!call_client_->wait_for_service(std::chrono::seconds(5))) {
    os << "Error: call_jiminy service not available" << std::endl;
    return;
  }

  auto request = std::make_shared<jiminy_msgs::srv::CallJiminy::Request>();
  request->semantics.semantics = semantics;
  request->facts = facts;

  auto future = call_client_->async_send_request(request);
  try {
    auto result = future.get();

    if (result->success) {
      os << "Call successful!" << std::endl;
      if (!result->message.empty()) {
        os << "Message: " << result->message << std::endl;
      }

      os << std::endl;
      os << "========================================" << std::endl;
      os << "  Accepted norms (" << result->accepted.size()
         << "):" << std::endl;
      os << "========================================" << std::endl;
      for (const auto &norm : result->accepted) {
        std::string type_str = "unknown";
        switch (norm.type) {
        case jiminy_msgs::msg::Norm::CONSTITUTIVE:
          type_str = "constitutive";
          break;
        case jiminy_msgs::msg::Norm::REGULATIVE:
          type_str = "regulative";
          break;
        case jiminy_msgs::msg::Norm::PERMISSIVE:
          type_str = "permissive";
          break;
        }
        os << "  [ACCEPTED] " << norm.id << " (" << type_str << ")"
           << std::endl;
        os << "    Conclusion:   " << norm.conclusion << std::endl;
        if (!norm.stakeholder.empty()) {
          os << "    Stakeholder:  " << norm.stakeholder << std::endl;
        }
        if (!norm.body.empty()) {
          os << "    Body:         [";
          for (size_t i = 0; i < norm.body.size(); i++) {
            os << norm.body[i];
            if (i < norm.body.size() - 1)
              os << ", ";
          }
          os << "]" << std::endl;
        }
        if (!norm.description.empty()) {
          os << "    Description:  " << norm.description << std::endl;
        }
        os << std::endl;
      }

      os << "========================================" << std::endl;
      os << "  Rejected norms (" << result->rejected.size()
         << "):" << std::endl;
      os << "========================================" << std::endl;
      for (const auto &norm : result->rejected) {
        std::string type_str = "unknown";
        switch (norm.type) {
        case jiminy_msgs::msg::Norm::CONSTITUTIVE:
          type_str = "constitutive";
          break;
        case jiminy_msgs::msg::Norm::REGULATIVE:
          type_str = "regulative";
          break;
        case jiminy_msgs::msg::Norm::PERMISSIVE:
          type_str = "permissive";
          break;
        }
        os << "  [REJECTED] " << norm.id << " (" << type_str << ")"
           << std::endl;
        os << "    Conclusion:   " << norm.conclusion << std::endl;
        if (!norm.stakeholder.empty()) {
          os << "    Stakeholder:  " << norm.stakeholder << std::endl;
        }
        if (!norm.body.empty()) {
          os << "    Body:         [";
          for (size_t i = 0; i < norm.body.size(); i++) {
            os << norm.body[i];
            if (i < norm.body.size() - 1)
              os << ", ";
          }
          os << "]" << std::endl;
        }
        if (!norm.description.empty()) {
          os << "    Description:  " << norm.description << std::endl;
        }
        os << std::endl;
      }

      // Summary
      os << "----------------------------------------" << std::endl;
      os << "Summary: " << result->accepted.size() << " accepted, "
         << result->rejected.size() << " rejected" << std::endl;
      os << "----------------------------------------" << std::endl;
    } else {
      os << "Call failed: " << result->message << std::endl;
    }
  } catch (const std::exception &e) {
    os << "Error calling service: " << e.what() << std::endl;
  }
}

} // namespace jiminy_terminal
