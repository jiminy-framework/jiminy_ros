# Jiminy ROS Test Suite

## Directory Structure

```
test/
├── integration/
│   ├── __init__.py
│   ├── test_load_scenario_service.py  # Tests /load_scenario ROS service
│   └── test_jiminy_reasoning.py       # Tests /call_jiminy with scenarios
├── scripts/
│   └── run_all_tests.sh               # Master test runner
└── README.md                          # This file
```

## Overview

Comprehensive integration tests for the Jiminy ROS package:

### Integration Tests (`test/integration/`)

#### 1. test_load_scenario_service.py
Tests the `/load_scenario` ROS service with 5 test suites:
- **Suite 1**: Scenario Loading (7 scenarios)
- **Suite 2**: Content Verification (norms, facts, contraries)
- **Suite 3**: Priority Resolution
- **Suite 4**: Scenario Switching (hot-reload)
- **Suite 5**: Error Handling

**Scenarios tested**: 7
**Tests**: 23 total
**Pass rate**: 100%

#### 2. test_jiminy_reasoning.py
Tests integration with the reasoning engine:
- Scenario loading via `/load_scenario`
- Reasoning via `/call_jiminy` with various fact combinations
- Argument generation verification

**Scenarios tested**: 3
**Test cases**: 6 (3 scenarios × 2 fact sets each)

## Quick Start

### Run All Tests
```bash
cd /path/to/jiminy_ros
./test/scripts/run_all_tests.sh
```

### Run Individual Test Suite
```bash
cd /path/to/jiminy_ros
source install/setup.bash

# Test 1: Load Scenario Service
python3 test/integration/test_load_scenario_service.py

# Test 2: Reasoning Service
python3 test/integration/test_jiminy_reasoning.py
```

### Prerequisites
- ROS 2 (Jazzy) installed
- Jiminy packages built: `colcon build`
- Jiminy node running: `ros2 launch mini_jiminy_bringup mini_jiminy.launch.py`

## Test Coverage

### Scenario Files (7 total)
-  jair.yaml (9 norms, GDPR vs threat)
-  agrobot.yaml (4 norms, gate safety)
-  candy_unified.yaml (12 norms, candy rules)
-  smoke_alarm.yaml (9 norms, fire detection)
-  jiminy.yaml (11 norms, unsupervised child)
-  jiminy_extended.yaml (52 norms, complex scenario)
-  demo_agricola/demo_priority.yaml (2 norms, priority demo)

### ROS Services Tested
- `/load_scenario` - Load YAML files dynamically
- `/call_jiminy` - Run reasoning engine with facts

### English YAML Format Support
-  context, description, norms
-  constitutive/regulative/permissive norms
-  contrariness (both "opposes" and "contraries" keys)
-  priorities, base_priorities, meta_priorities
-  Optional fields handling (graceful defaults)

## Running Tests in CI/CD

### Example: GitHub Actions
```yaml
- name: Run Jiminy Integration Tests
  run: |
    cd jiminy_ros
    source install/setup.bash
    timeout 60 ros2 launch mini_jiminy_bringup mini_jiminy.launch.py &
    sleep 5
    ./test/scripts/run_all_tests.sh
```

## Expected Output

```
================================================================================
JIMINY ROS INTEGRATION TEST SUITE
================================================================================

TEST SUITE 1: Scenario Loading
 Load jair.yaml                                     PASS   Loaded 9 norms
 Load agrobot.yaml                                  PASS   Loaded 4 norms
 Load candy_unified.yaml                            PASS   Loaded 12 norms
...

TEST SUMMARY
============
Total Tests: 23
   PASSED: 23
   FAILED: 0

Pass Rate: 100.0%

================================================================================
 ALL TESTS PASSED
================================================================================
```

## Troubleshooting

**Issue**: Service not found
```bash
# Check node is running
ros2 node list | grep mini_jiminy

# Check services
ros2 service list | grep jiminy
```

**Issue**: YAML parsing errors
```bash
# Validate YAML syntax
python3 -c "import yaml; yaml.safe_load(open('path/to/file.yaml'))"
```

**Issue**: Tests timeout
- Increase timeout in test scripts
- Check ROS node CPU/memory usage: `ros2 node info /mini_jiminy_node`

## For More Information

See documentation in `docs/testing/`:
- `TEST_SUITE_DOCUMENTATION.md` - Detailed test architecture
- `TEST_QUICK_REFERENCE.md` - Quick reference guide

## Test Statistics

- **Total test cases**: 23
- **Scenarios tested**: 7
- **Total norms tested**: 97
- **Service coverage**: 2 ROS services
- **Average per-test duration**: <5 seconds
- **Full suite duration**: ~60 seconds
