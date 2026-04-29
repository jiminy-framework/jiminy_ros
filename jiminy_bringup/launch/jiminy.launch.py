# MIT License
#
# Copyright (c) 2026 Jiminy Framework
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import os
from launch_ros.actions import Node
from launch.actions import OpaqueFunction, DeclareLaunchArgument
from launch import LaunchDescription, LaunchContext
from launch.substitutions import LaunchConfiguration
from ament_index_python import get_package_share_directory


def generate_launch_description():

    # Creata a run_jiminy opaque function
    def run_jiminy(context: LaunchContext, config_file):
        config_file = str(context.perform_substitution(config_file))
        config_file = os.path.join(
            get_package_share_directory("jiminy_bringup"),
            "scenarios",
            config_file,
        )

        return [
            Node(
                package="jiminy_ros",
                executable="jiminy_node",
                output="both",
                parameters=[{"config_file": config_file}],
            )
        ]

    config_file = LaunchConfiguration("config_file")
    config_file_cmd = DeclareLaunchArgument(
        "config_file",
        default_value="candy_unified.yaml",
        description="Config file to use",
    )

    ld = LaunchDescription()
    ld.add_action(config_file_cmd)
    ld.add_action(
        OpaqueFunction(function=run_jiminy, args=[config_file]),
    )
    return ld
