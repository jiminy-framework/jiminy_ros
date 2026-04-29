#!/usr/bin/env bash
#
# JIMINY ROS SERVICE TEST - QUICK REFERENCE GUIDE
#
# This guide shows how to run the comprehensive test suite
# for English YAML scenario files in the Jiminy ROS package
#

# ============================================================================
# QUICK START: Run All Tests
# ============================================================================

# Terminal 1: Start the Jiminy node
cd ~/jiminy-folder/jiminy_ros
source install/setup.bash
ros2 launch mini_jiminy_bringup mini_jiminy.launch.py

# Terminal 2: Run all tests
cd ~/jiminy-folder/jiminy_ros
source install/setup.bash
./run_all_tests.sh


# ============================================================================
# INDIVIDUAL TEST EXECUTION
# ============================================================================

# Test 1: Load Scenario Service Tests
# Tests loading of 7 English YAML scenario files
cd ~/jiminy-folder/jiminy_ros
source install/setup.bash
python3 test_load_scenario_service.py

# Test 2: Jiminy Reasoning Service Tests  
# Tests integration with reasoning engine
cd ~/jiminy-folder/jiminy_ros
source install/setup.bash
python3 test_jiminy_reasoning.py


# ============================================================================
# WHAT GETS TESTED
# ============================================================================

# Test Suite 1: Scenario Loading
# - Loads all 7 scenario files
# - Verifies zero parsing errors
# - Checks correct number of norms/facts/priorities loaded
#
# Scenarios:
#    jair.yaml (9 norms, 4 facts, 5 contraries)
#    agrobot.yaml (4 norms, 2 facts, 3 contraries)
#    candy_unified.yaml (12 norms)
#    smoke_alarm.yaml (9 norms, 4 facts)
#    jiminy.yaml (11 norms, 4 facts, 9 contraries)
#    jiminy_extended.yaml (52 norms, 4 facts, 33 contraries)
#    demo_agricola/demo_priority.yaml (2 norms, 1 fact, 2 contraries)

# Test Suite 2: Content Verification
# - Verifies exact counts of loaded elements
# - Norms, facts, contrariness rules, priorities all correct

# Test Suite 3: Priority Resolution
# - Loads priority data correctly
# - Provides contrariness rules for conflict resolution

# Test Suite 4: Scenario Switching
# - Can load jair.yaml → agrobot.yaml → jair.yaml in sequence
# - Verifies hot-reload capability without crashes

# Test Suite 5: Error Handling
# - Handles missing files gracefully
# - Handles invalid paths without crashing
# - Robust to edge cases


# ============================================================================
# EXPECTED OUTPUT
# ============================================================================

# When all tests pass, you should see:
#
# ================================================================================
# JIMINY ROS SERVICE INTEGRATION TEST SUITE
# ================================================================================
#
# TEST SUITE 1: Scenario Loading
#  Load jair.yaml                                     PASS   Loaded 9 norms
#  Load agrobot.yaml                                  PASS   Loaded 4 norms
#  Load candy_unified.yaml                            PASS   Loaded 12 norms
#  Load smoke_alarm.yaml                              PASS   Loaded 9 norms
#  Load jiminy.yaml                                   PASS   Loaded 11 norms
#  Load jiminy_extended.yaml                          PASS   Loaded 52 norms
#  Load demo_agricola/demo_priority.yaml              PASS   Loaded 2 norms
#
# TEST SUITE 2: Scenario Content Verification
#  Content: jair.yaml                                 PASS   Norms: 9, Facts: 4
#  Content: agrobot.yaml                              PASS   Norms: 4, Facts: 2
# [... more tests ...]
#
# TEST SUITE 3: Priority Resolution
#  Priority: Load priorities                          PASS   Loaded 2 priorities
#  Priority: Contrariness rules                       PASS   Found 2 contrariness
#
# TEST SUITE 4: Scenario Switching (Hot-Reload)
#  Switching: Call 1 → jair.yaml                      PASS   Loaded 9 norms
#  Switching: Call 2 → agrobot.yaml                   PASS   Loaded 4 norms
#  Switching: Call 3 → jair.yaml                      PASS   Loaded 9 norms
#  Switching: Reload capability                       PASS   Successfully reloaded
#
# TEST SUITE 5: Error Handling & Edge Cases
#  Error: Non-existent file                           PASS   Service returns grac
#  Error: Empty path                                  PASS   Service handles empt
#  Error: Robustness check                            PASS   Service handles vali
#
# ================================================================================
# TEST SUMMARY
# ================================================================================
#
# Total Tests: 23
#    PASSED: 23
#    FAILED: 0
#   ⊘ SKIPPED: 0
#
# Pass Rate: 100.0%
#
# ================================================================================
#  ALL TESTS PASSED
# ================================================================================


# ============================================================================
# MANUAL SERVICE CALLS (For Debugging)
# ============================================================================

# Load a scenario manually
ros2 service call /load_scenario mini_jiminy_msgs/srv/LoadScenario \
  "config_file: 'mini_jiminy_bringup/scenarios/jair.yaml'"

# Call reasoning engine with specific facts
ros2 service call /call_jiminy mini_jiminy_msgs/srv/CallJiminy \
  "{'semantics': {'semantics': 'grounded'}, 'facts': ['w1', 'w2']}"

# View available services
ros2 service list

# View service types
ros2 service type /load_scenario
ros2 service type /call_jiminy

# View service schemas
ros2 service type /load_scenario | ros2 interface show mini_jiminy_msgs/srv/LoadScenario


# ============================================================================
# TROUBLESHOOTING
# ============================================================================

# Problem: Service not found
# Solution:
ros2 node list                    # Check Jiminy node is running
ros2 service list | grep jiminy   # Check services are registered
colcon build                      # Rebuild if needed

# Problem: YAML parsing errors
# Solution:
ls -la mini_jiminy_bringup/scenarios/          # Check files exist
file mini_jiminy_bringup/scenarios/*.yaml      # Check file types
python3 -c "import yaml; yaml.safe_load(open('mini_jiminy_bringup/scenarios/jair.yaml'))"

# Problem: Tests timeout
# Solution: Increase timeout in test scripts or check node CPU usage
ros2 node info /mini_jiminy_node

# Problem: Permission denied
# Solution:
chmod +x test_*.py run_all_tests.sh


# ============================================================================
# DEBUGGING INDIVIDUAL TESTS
# ============================================================================

# Add verbose logging to test
export ROS_LOG_DIR=./ros_logs
python3 -u test_load_scenario_service.py

# Check ROS logs
tail -f ~/.ros/log/latest_run/*(N)/log

# Inspect service responses
ros2 service call /load_scenario mini_jiminy_msgs/srv/LoadScenario \
  "config_file: 'mini_jiminy_bringup/scenarios/jair.yaml'" | tee service_response.txt

# Parse response
cat service_response.txt | grep "success="
cat service_response.txt | grep -o "Norm(" | wc -l


# ============================================================================
# CONTINUOUS INTEGRATION
# ============================================================================

# Run tests as part of CI/CD pipeline
(
  cd ~/jiminy-folder/jiminy_ros
  source install/setup.bash
  
  # Start node in background
  timeout 45 ros2 launch mini_jiminy_bringup mini_jiminy.launch.py &
  NODE_PID=$!
  sleep 5
  
  # Run all tests
  python3 test_load_scenario_service.py || exit 1
  python3 test_jiminy_reasoning.py || exit 1
  
  # Cleanup
  kill $NODE_PID 2>/dev/null
  exit 0
)


# ============================================================================
# TEST STATISTICS
# ============================================================================

echo "Test Statistics:"
echo "  - Total test cases: 23"
echo "  - Scenarios tested: 7"
echo "  - Norms tested: 97 total"
echo "  - Expected pass rate: 100%"
echo "  - Average per-test duration: <5 seconds"
echo "  - Full suite duration: ~60 seconds"

# Count scenarios
find mini_jiminy_bringup/scenarios -name "*.yaml" | wc -l

# Count norms
grep "- id:" mini_jiminy_bringup/scenarios/*.yaml | wc -l

# Count test methods
grep "def test_" test_*.py | wc -l
