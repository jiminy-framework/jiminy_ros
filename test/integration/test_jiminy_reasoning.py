#!/usr/bin/env python3
"""
ROS Integration Tests for Jiminy /call_jiminy Service
Tests that loaded scenarios work correctly with the reasoning engine.
"""

import sys
import subprocess
import time
from dataclasses import dataclass
from typing import Dict, List, Tuple


@dataclass
class ReasoningResult:
    """Stores reasoning test result"""
    scenario: str
    facts: List[str]
    status: str  # "PASS", "FAIL"
    message: str
    num_arguments: int = 0
    num_decisions: int = 0


class JiminyReasoningTester:
    """Test harness for Jiminy reasoning service"""
    
    def __init__(self):
        self.results: List[ReasoningResult] = []
        self.test_cases = {
            'jair.yaml': [
                {'facts': ['w1', 'w2'], 'name': 'Manufacturer and data collection'},
                {'facts': ['w1', 'w2', 'w3'], 'name': 'With threat detection'},
            ],
            'agrobot.yaml': [
                {'facts': ['w1'], 'name': 'Open gate detected'},
                {'facts': ['w1', 'w2'], 'name': 'Gate open, machinery absent'},
            ],
            'smoke_alarm.yaml': [
                {'facts': ['w1'], 'name': 'Smoke detected'},
                {'facts': ['w1', 'w2'], 'name': 'Smoke with child present'},
            ],
        }
    
    def call_jiminy_service(self, facts: List[str]) -> Tuple[bool, Dict]:
        """
        Call /call_jiminy ROS service
        Returns: (success, data_dict)
        """
        try:
            # Format facts as ROS list
            facts_str = ', '.join([f"'{f}'" for f in facts])
            
            result = subprocess.run(
                f'ros2 service call /call_jiminy mini_jiminy_msgs/srv/CallJiminy '
                f'"{{\'semantics\': {{\'semantics\': \'grounded\'}}, \'facts\': [{facts_str}]}}" 2>&1',
                shell=True, capture_output=True, text=True, timeout=8
            )
            
            success = 'response:' in result.stdout and not '[ERROR]' in result.stdout
            
            data = {
                'raw_output': result.stdout,
                'arguments_found': 'arguments=' in result.stdout,
                'decisions_found': 'decisions=' in result.stdout,
            }
            
            return success, data
            
        except subprocess.TimeoutExpired:
            return False, {'error': 'Service call timeout'}
        except Exception as e:
            return False, {'error': str(e)}
    
    def load_scenario(self, scenario_file: str) -> bool:
        """Load a scenario before running reasoning tests"""
        try:
            import os
            workspace_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
            scenario_path = f"{workspace_root}/mini_jiminy_bringup/scenarios/{scenario_file}"
            
            result = subprocess.run(
                f'ros2 service call /load_scenario mini_jiminy_msgs/srv/LoadScenario '
                f'"config_file: \'{scenario_path}\'" 2>&1',
                shell=True, capture_output=True, text=True, timeout=8
            )
            
            return 'success=True' in result.stdout
            
        except Exception:
            return False
    
    def run_all_tests(self):
        """Execute all reasoning tests"""
        print("\n" + "="*80)
        print("JIMINY REASONING SERVICE INTEGRATION TEST SUITE")
        print("="*80 + "\n")
        
        for scenario_file, test_cases in self.test_cases.items():
            print(f"\nTesting Scenario: {scenario_file}")
            print("-" * 80)
            
            # Load scenario
            if not self.load_scenario(scenario_file):
                print(f"✗ Failed to load {scenario_file}")
                continue
            
            print(f"✓ Loaded {scenario_file}")
            time.sleep(0.2)
            
            # Run reasoning tests
            for test_case in test_cases:
                facts = test_case['facts']
                test_name = test_case.get('name', 'Test')
                
                success, data = self.call_jiminy_service(facts)
                
                result = ReasoningResult(
                    scenario=scenario_file,
                    facts=facts,
                    status="PASS" if success else "FAIL",
                    message=test_name,
                    num_arguments=data.get('raw_output', '').count('Argument(') if success else 0,
                )
                
                self.results.append(result)
                
                status_symbol = "✓" if success else "✗"
                args_count = result.num_arguments
                print(f"{status_symbol} {test_name:40} facts: {facts} (args: {args_count})")
        
        # Summary
        self.print_summary()
    
    def print_summary(self):
        """Print test summary"""
        print("\n" + "="*80)
        print("REASONING TEST SUMMARY")
        print("="*80 + "\n")
        
        if not self.results:
            print("No tests were executed")
            return
        
        pass_count = sum(1 for r in self.results if r.status == "PASS")
        fail_count = sum(1 for r in self.results if r.status == "FAIL")
        total = len(self.results)
        
        print(f"Total Reasoning Tests: {total}")
        print(f"  ✓ PASSED: {pass_count}")
        print(f"  ✗ FAILED: {fail_count}")
        
        pass_rate = (pass_count / total * 100) if total > 0 else 0
        print(f"\nPass Rate: {pass_rate:.1f}%")
        
        if fail_count > 0:
            print("\nFailed Tests:")
            for result in self.results:
                if result.status == "FAIL":
                    print(f"  - {result.scenario} with facts {result.facts}: {result.message}")
        
        print("\n" + "="*80)
        if fail_count == 0 and pass_count > 0:
            print("✓ ALL REASONING TESTS PASSED")
        else:
            print(f"✗ {fail_count} TEST(S) FAILED")
        print("="*80 + "\n")


# Pytest-discoverable test functions for colcon test
# These wrap the tester methods so pytest can discover them

# Create a global tester instance for pytest
_tester = JiminyReasoningTester()


def test_jiminy_reasoning_all():
    """Test Jiminy reasoning across all scenarios"""
    _tester.run_all_tests()
    # Assert all tests passed
    assert all(r.status == "PASS" for r in _tester.results), f"Some reasoning tests failed: {[(r.scenario, r.facts) for r in _tester.results if r.status != 'PASS']}"


def main():
    """Main test entry point"""
    tester = JiminyReasoningTester()
    
    try:
        tester.run_all_tests()
        sys.exit(0 if all(r.status == "PASS" for r in tester.results) else 1)
    except KeyboardInterrupt:
        print("\n\nTests interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"\nFatal error: {str(e)}")
        sys.exit(2)


if __name__ == '__main__':
    main()
