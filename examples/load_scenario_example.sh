#!/bin/bash
# Example: Loading jair.yaml at runtime using the load_scenario service

# Make sure the mini_jiminy node is running first:
# ros2 launch mini_jiminy_bringup mini_jiminy.launch.py

# Load jair.yaml at runtime
echo "Loading jair.yaml scenario at runtime..."
ros2 service call /load_scenario mini_jiminy_msgs/srv/LoadScenario \
  "config_file: 'src/mini_jiminy_bringup/scenarios/jair.yaml'"

# Check that it was loaded successfully
echo ""
echo "Getting the current scenario..."
ros2 service call /get_scenario mini_jiminy_msgs/srv/GetScenario ""

# Now you can call jiminy with facts from the jair scenario
echo ""
echo "Calling jiminy with jair facts..."
ros2 service call /call_jiminy mini_jiminy_msgs/srv/CallJiminy \
  "facts: ['w1', 'w2', 'w3', 'w4']
   semantics:
     semantics: 'priority'"
