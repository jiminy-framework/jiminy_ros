#!/bin/bash
#
# Run all Jiminy ROS integration tests
# Usage: From repo root: ./test/scripts/run_all_tests.sh
#

set -e

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_ROOT="$( cd "$SCRIPT_DIR/../.." && pwd )"
TEST_DIR="$REPO_ROOT/test/integration"

cd "$REPO_ROOT"

echo ""
echo "=============================================================================="
echo "JIMINY ROS INTEGRATION TEST SUITE"
echo "=============================================================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if sourcing needed
if [ -z "$ROS_DISTRO" ]; then
    echo "Sourcing ROS setup..."
    source install/setup.bash
fi

# Check node is running
echo "Checking for Jiminy ROS node..."
if ! ros2 node list 2>/dev/null | grep -q jiminy; then
    echo -e "${YELLOW}Warning: jiminy node not detected${NC}"
    echo "Starting Jiminy node..."
    timeout 10 ros2 launch jiminy_bringup jiminy.launch.py &
    sleep 3
fi

# Verify service is available
echo "Verifying services are available..."
max_attempts=5
attempt=0
while [ $attempt -lt $max_attempts ]; do
    if ros2 service list 2>/dev/null | grep -q "/load_scenario"; then
        echo -e "${GREEN}✓ /load_scenario service found${NC}"
        break
    fi
    attempt=$((attempt + 1))
    sleep 1
done

if [ $attempt -eq $max_attempts ]; then
    echo -e "${RED}✗ Services not available${NC}"
    exit 1
fi

echo ""
echo "=============================================================================="
echo "TEST 1: Load Scenario Service Integration Tests"
echo "=============================================================================="
echo ""

if [ -f "$TEST_DIR/test_load_scenario_service.py" ]; then
    python3 "$TEST_DIR/test_load_scenario_service.py"
    TEST1_EXIT=$?
else
    echo -e "${RED}✗ test_load_scenario_service.py not found at $TEST_DIR${NC}"
    TEST1_EXIT=1
fi

echo ""
echo "=============================================================================="
echo "TEST 2: Jiminy Reasoning Service Integration Tests"
echo "=============================================================================="
echo ""

if [ -f "$TEST_DIR/test_jiminy_reasoning.py" ]; then
    python3 "$TEST_DIR/test_jiminy_reasoning.py"
    TEST2_EXIT=$?
else
    echo -e "${RED}✗ test_jiminy_reasoning.py not found at $TEST_DIR${NC}"
    TEST2_EXIT=1
fi

echo ""
echo "=============================================================================="
echo "OVERALL RESULTS"
echo "=============================================================================="
echo ""

if [ $TEST1_EXIT -eq 0 ] && [ $TEST2_EXIT -eq 0 ]; then
    echo -e "${GREEN}✓ ALL TEST SUITES PASSED${NC}"
    exit 0
else
    echo -e "${RED}✗ SOME TESTS FAILED${NC}"
    [ $TEST1_EXIT -ne 0 ] && echo "  - Load Scenario tests failed"
    [ $TEST2_EXIT -ne 0 ] && echo "  - Reasoning tests failed"
    exit 1
fi
