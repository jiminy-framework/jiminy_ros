# Jiminy ROS - jair.yaml Compatibility Fix

## Summary

The ROS version of Jiminy has been updated to support scenario files that use `base_priorities` and `meta_priorities` instead of static priority declarations. This enables compatibility with jair.yaml and other scenarios that follow the same pattern.

## What Changed

### 1. Header File Updates (`jiminy_ros/jiminy.hpp`)

- Added `MetaPriority` struct to represent dynamic priority rules:
  ```cpp
  struct MetaPriority {
    std::string if_condition;  // fact that triggers this rule
    std::string stakeholder;
    int value;
    std::string description;
  };
  ```
- Updated `Jiminy::Jiminy()` constructor to accept:
  - `base_priorities`: map of stakeholder IDs to base priority values
  - `meta_priorities`: vector of dynamic priority escalation rules
- Added new methods:
  - `derive_stakeholder_priorities()`: Compute stakeholder priorities from base + meta rules
  - `derive_conclusion_priorities()`: Map stakeholder priorities to conclusion priorities

### 2. YAML Loader Updates (`jiminy_node.cpp`)

- Now parses `base_priorities` from YAML files
- Now parses `meta_priorities` array from YAML files
- **Auto-computation**: When static `priorities` field is empty but `base_priorities` exists:
  - For each norm, assigns its conclusion the base priority of the supporting stakeholder
  - If multiple norms support the same conclusion, takes the maximum priority
  - This enables scenarios to work without explicit priority declarations

### 3. Implementation Files (`jiminy.cpp`)

- Updated constructor implementation to store base_priorities and meta_priorities
- Implemented priority derivation methods

## How It Works

### Scenario File Structure

A scenario file can now use this pattern:

```yaml
base_priorities:
  Law: 3
  Household: 2
  Manuf: 1

meta_priorities:
  - if: i_threat
    stakeholder: Law
    value: 5
    description: "threat escalates legal authority"

norms:
  - id: r1
    body: [w1]
    conclusion: d1
    stakeholder: Law
    type: r

priorities: {} # Empty - will be automatically computed
```

When loaded, the ROS engine will:

1. Detect empty `priorities` field
2. Find that norm r1 supports conclusion d1 via stakeholder Law
3. Assign d1 priority value 3 (Law's base priority)
4. Store base_priorities and meta_priorities for potential future runtime refinement

## Testing

### With jair.yaml

A test scenario `jair.yaml` has been copied to:

```
jiminy_ros/src/jiminy_bringup/scenarios/jair.yaml
```

This scenario uses the new features:

- Defines base stakeholder priorities
- Includes meta-priority escalation rules
- Uses empty static priorities field
- Now fully compatible with the ROS engine

### Building

```bash
cd jiminy_ros
colcon build
```

## Backwards Compatibility

**Fully backwards compatible**

- Scenarios with explicit static `priorities` declarations continue to work unchanged
- The auto-computation only triggers when `priorities` is empty AND `base_priorities` is non-empty
- If both are empty, the system behaves as before

## Future Enhancements

The `meta_priorities` are now loaded and stored but not yet used for runtime dynamic priority refinement. Future versions could implement:

- Dynamic priority adjustment during extension computation based on accepted conclusions
- Two-phase "jiminy" semantics matching the Python version
- Runtime re-evaluation of priorities when context facts change
