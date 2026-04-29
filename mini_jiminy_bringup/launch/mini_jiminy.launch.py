import os
from launch_ros.actions import Node
from launch.actions import OpaqueFunction, DeclareLaunchArgument
from launch import LaunchDescription, LaunchContext
from launch.substitutions import LaunchConfiguration
from ament_index_python import get_package_share_directory


def generate_launch_description():

    # Creata a run_mini_jiminy opaque function
    def run_mini_jiminy(context: LaunchContext, config_file):
        config_file = str(context.perform_substitution(config_file))
        config_file = os.path.join(
            get_package_share_directory("mini_jiminy_bringup"),
            "scenarios",
            config_file,
        )

        return [
            Node(
                package="mini_jiminy",
                executable="mini_jiminy_node",
                output="both",
                parameters=[{"config_file": config_file}],
            )
        ]

    config_file = LaunchConfiguration("config_file")
    config_file_cmd = DeclareLaunchArgument(
        "config_file",
        default_value="escenario_dulces_antes_de_comer.yaml",
        description="Config file to use",
    )

    ld = LaunchDescription()
    ld.add_action(config_file_cmd)
    ld.add_action(
        OpaqueFunction(function=run_mini_jiminy, args=[config_file]),
    )
    return ld
