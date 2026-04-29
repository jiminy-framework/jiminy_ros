#!/bin/bash
#
# Convenience wrapper to run Jiminy tests from repository root
# Forwards all arguments to test/scripts/run_all_tests.sh
#

exec "$(dirname "$0")/scripts/run_all_tests.sh" "$@"
