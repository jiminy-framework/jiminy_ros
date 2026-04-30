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

#include <memory>
#include <thread>

#include "rclcpp/rclcpp.hpp"

#include "jiminy_terminal/terminal.hpp"

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  auto terminal_node = std::make_shared<jiminy_terminal::Terminal>();

  rclcpp::executors::SingleThreadedExecutor exe;
  exe.add_node(terminal_node->get_node_base_interface());

  bool finish = false;
  std::thread t([&]() {
    while (!finish) {
      exe.spin_some();
    }
  });

  terminal_node->run_console();

  finish = true;
  t.join();

  rclcpp::shutdown();
  return 0;
}
