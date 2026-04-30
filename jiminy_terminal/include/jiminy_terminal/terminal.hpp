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

#ifndef JIMINY_TERMINAL__TERMINAL_HPP_
#define JIMINY_TERMINAL__TERMINAL_HPP_

#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"

#include "jiminy_msgs/srv/call_jiminy.hpp"
#include "jiminy_msgs/srv/get_scenario.hpp"
#include "jiminy_msgs/srv/load_scenario.hpp"

namespace jiminy_terminal {

std::vector<std::string> tokenize(const std::string &text,
                                  const std::string delim = " ");
void pop_front(std::vector<std::string> &tokens);

class Terminal : public rclcpp::Node {
public:
  Terminal();

  virtual void run_console();

protected:
  virtual void clean_command(std::string &command);
  virtual void process_command(std::string &command, std::ostringstream &os);

  virtual void process_help(std::vector<std::string> &command,
                            std::ostringstream &os);
  virtual void process_load(std::vector<std::string> &command,
                            std::ostringstream &os);
  virtual void process_get_scenario(std::vector<std::string> &command,
                                    std::ostringstream &os);
  virtual void process_call(std::vector<std::string> &command,
                            std::ostringstream &os);

private:
  static char *completion_generator(const char *text, int state);
  static char **completer(const char *text, int start, int end);
  static Terminal *current_terminal_;

  rclcpp::Client<jiminy_msgs::srv::CallJiminy>::SharedPtr call_client_;
  rclcpp::Client<jiminy_msgs::srv::GetScenario>::SharedPtr get_scenario_client_;
  rclcpp::Client<jiminy_msgs::srv::LoadScenario>::SharedPtr
      load_scenario_client_;

  // Cached facts from the loaded scenario for autocompletion
  std::vector<std::string> cached_facts_;
};

} // namespace jiminy_terminal

#endif // JIMINY_TERMINAL__TERMINAL_HPP_
