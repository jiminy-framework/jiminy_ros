# Jiminy Advisor — Minimal Normative Reasoning Engine for Autonomous Robots  
*A lightweight implementation of the moral and normative reasoning framework introduced in Jiminy .*

---

##  Overview

**Jiminy Advisor** is a compact, transparent, and research-friendly engine that implements a *prioritized normative argumentation system* for autonomous robots.  
It follows the formal model introduced in the Jiminy framework:

- **Brute facts** → context observations (`w₁, w₂, ...`)
- **Institutional facts** → derived via *constitutive norms* (`i₁, i₂, ...`)
- **Obligations** → produced by *regulative norms* (`d₁, d₂, ...`)
- **Permissions** → produced by *permissive norms* (`p₁, ...`)
- **Contrariness relations** → define conflicts between conclusions
- **Priority ordering** → resolves normative dilemmas

This repository provides:

- A **C++ engine** to detach norms, construct arguments, detect conflicts, and compute a *prioritized extension*.
- An **explanation generator** for debugging and transparency.
- A **YAML format** that mirrors the formal definitions from the paper.
- A **graph generator** for argument and attack structures.
- Optional conceptual integration with **MERLIN2** (hybrid cognitive architecture for ROS 2 robots).

---


## YAML Scenario Format

Each scenario mirrors the formal structure of the Jiminy framework:

- `contexto`: **brute facts** (observations)
- `normas`: **constitutive**, **regulative**, and **permissive** norms
- `contrariedades`: **contrariness relation**
- `prioridades`: **priority ordering**

Example:

```yaml
contexto:
  - id: w1
    descripcion: "The child is eating candy."
  - id: w2
    descripcion: "Parents have said candy is forbidden before lunch."
  - id: w3
    descripcion: "No physical risk is present."

normas:
  - id: H1
    stakeholder: "Home"
    tipo: "c"
    descripcion: "Eating candy before lunch is classified as rule violation."
    cuerpo: ["w1", "w2"]
    conclusion: "i1"

  - id: H2
    stakeholder: "Home"
    tipo: "r"
    descripcion: "Remind the child of the rule."
    cuerpo: ["i1"]
    conclusion: "d1"
```

---

## ⚙️ Running an Example

```bash
ros2 launch mini_jiminy_bringup mini_jiminy.launch.py
```

```bash
ros2 service call /call_jiminy mini_jiminy_msgs/srv/CallJiminy "{'semantics': {'semantics': 'grounded'}, 'facts': ['w1', 'w5']}"
```

The engine prints:

- Context analysis  
- Generated arguments  
- Conflict detection  
- Priority evaluation  
- Accepted vs. rejected arguments  
- Final moral recommendation  
- Optional narrative explanation  
- A graph (`escenario.jpg`) showing nodes and attacks  

---

## 🔄 Runtime YAML Configuration (Dynamic Scenario Loading)

### Overview

The Jiminy ROS node supports **dynamic scenario loading at runtime** via the `/load_scenario` service, eliminating the need to restart the node when switching between different normative reasoning scenarios.

This feature enables:
- **Hot-swapping scenarios** without interrupting the node
- **Support for base_priorities and meta_priorities** (dynamic priority computation)
- **Compatibility with jair.yaml** and other scenario formats
- **Real-time reconfiguration** for multi-scenario robotic applications

### The `/load_scenario` Service

**Service Type**: `mini_jiminy_msgs/LoadScenario`

**Request**:
```
string config_file    # Path to the YAML scenario file
```

**Response**:
```
bool success                                    # Whether the load succeeded
string message                                  # Success or error description
ScenarioFull scenario                           # Loaded scenario details (if success=true)
```

**The ScenarioFull message includes**:
- `contexto`: Array of context (brute fact) objects
- `normas`: Array of norm objects
- `contrariedades`: Array of contrariness relations
- `prioridades`: Array of priority rules (static priorities)
- `base_priorities`: Array of stakeholder base priorities
- `meta_priorities`: Array of meta-priority escalation rules

### Example: Switching Scenarios at Runtime

#### 1. Using `ros2 service call` (Command Line)

Start the Jiminy node:
```bash
ros2 launch mini_jiminy_bringup mini_jiminy.launch.py
```

In another terminal, load the **jair.yaml** scenario (with base priorities):
```bash
ros2 service call /load_scenario mini_jiminy_msgs/srv/LoadScenario \
  "config_file: 'src/mini_jiminy_bringup/scenarios/jair.yaml'"
```

**Expected Response**:
```
result: 
  success: true
  message: 'Scenario loaded successfully: jair.yaml'
  scenario: [scenario details with base_priorities and derived priorities]
```

Switch to **agrobot.yaml** scenario:
```bash
ros2 service call /load_scenario mini_jiminy_msgs/srv/LoadScenario \
  "config_file: 'src/mini_jiminy_bringup/scenarios/agrobot.yaml'"
```

#### 2. Using Python Client

```python
import rclpy
from mini_jiminy_msgs.srv import LoadScenario

def main():
    rclpy.init()
    node = rclpy.create_node('scenario_loader')
    
    client = node.create_client(LoadScenario, '/load_scenario')
    while not client.wait_for_service(timeout_sec=1.0):
        print('Service not available, waiting...')
    
    # Load jair.yaml scenario
    request = LoadScenario.Request()
    request.config_file = 'src/mini_jiminy_bringup/scenarios/jair.yaml'
    
    future = client.call_async(request)
    rclpy.spin_until_future_complete(node, future)
    
    if future.result() is not None:
        result = future.result()
        print(f"Success: {result.success}")
        print(f"Message: {result.message}")
        print(f"Loaded scenario with {len(result.scenario.normas)} norms")
    else:
        print("Service call failed")
    
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
```

#### 3. Using C++ Client

```cpp
#include "rclcpp/rclcpp.hpp"
#include "mini_jiminy_msgs/srv/load_scenario.hpp"

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<rclcpp::Node>("scenario_loader");
  
  auto client = node->create_client<mini_jiminy_msgs::srv::LoadScenario>("/load_scenario");
  
  while (!client->wait_for_service(std::chrono::seconds(1))) {
    if (!rclcpp::ok()) return 0;
    RCLCPP_INFO(node->get_logger(), "Waiting for service...");
  }
  
  auto request = std::make_shared<mini_jiminy_msgs::srv::LoadScenario::Request>();
  request->config_file = "src/mini_jiminy_bringup/scenarios/jair.yaml";
  
  auto result = client->async_send_request(request);
  if (rclcpp::spin_until_future_complete(node, result) == 
      rclcpp::executor::FutureReturnCode::SUCCESS) 
  {
    auto response = result.get();
    RCLCPP_INFO(node->get_logger(), "Scenario loaded: %s", response->message.c_str());
  }
  
  rclcpp::shutdown();
  return 0;
}
```

### Scenario Format: Static vs. Dynamic Priorities

#### Static Priorities (Traditional Format)
```yaml
contexto:
  - id: w1
    descripcion: "Observation 1"

normas:
  - id: H1
    stakeholder: "Home"
    tipo: "r"
    cuerpo: ["w1"]
    conclusion: "d1"

prioridades:
  - [superior, "d1", "d2"]  # d1 beats d2
```

#### Dynamic Priorities with Base and Meta Rules
```yaml
contexto:
  - id: w1
    descripcion: "Observation 1"

normas:
  - id: H1
    stakeholder: "Home"
    tipo: "r"
    cuerpo: ["w1"]
    conclusion: "d1"

base_priorities:
  - stakeholder: "Home"
    value: 80
  - stakeholder: "Street"
    value: 50

meta_priorities:
  - if_condition: "emergency_present"
    stakeholder: "Emergency"
    value: 100
    description: "Emergency stakeholder gains priority when danger detected"
```

The engine **automatically computes conclusion-level priorities** from stakeholder base priorities and applies meta-priority rules.

### Workflow Example: Multi-Scenario Reasoning

```bash
# Start Jiminy
ros2 launch mini_jiminy_bringup mini_jiminy.launch.py

# --- Scenario 1: Agricultural Robot (Agrobot) ---
ros2 service call /load_scenario mini_jiminy_msgs/srv/LoadScenario \
  "config_file: 'src/mini_jiminy_bringup/scenarios/agrobot.yaml'"

# Run reasoning on loaded scenario
ros2 service call /call_jiminy mini_jiminy_msgs/srv/CallJiminy \
  "{'semantics': {'semantics': 'grounded'}, 'facts': ['w1', 'w2']}"

# --- Scenario 2: Jair (Multi-stakeholder with Meta-priorities) ---
ros2 service call /load_scenario mini_jiminy_msgs/srv/LoadScenario \
  "config_file: 'src/mini_jiminy_bringup/scenarios/jair.yaml'"

# Run reasoning on the new scenario (no node restart required)
ros2 service call /call_jiminy mini_jiminy_msgs/srv/CallJiminy \
  "{'semantics': {'semantics': 'grounded'}, 'facts': ['w3', 'w4', 'w5']}"

# --- Scenario 3: Smoke Alarm ---
ros2 service call /load_scenario mini_jiminy_msgs/srv/LoadScenario \
  "config_file: 'src/mini_jiminy_bringup/scenarios/smoke_alarm.yaml'"
```

### Error Handling

If the service encounters an error (e.g., invalid YAML file), it responds with:

```
success: false
message: "Error: Could not load YAML file - [details about the error]"
scenario: [null/empty]
```

**The previous scenario remains active** in case of load failure, ensuring graceful degradation.

### Performance Considerations

- **Load time**: Typically 10-50ms depending on scenario complexity
- **Memory**: Old scenario is released immediately after new one loads
- **No interruption**: The `/call_jiminy` service can be called while scenarios are loading (new calls use the current scenario)

### For More Details

See the detailed documentation files:
- [RUNTIME_YAML_SERVICE.md](../../RUNTIME_YAML_SERVICE.md) — Complete API reference and advanced usage
- [RUNTIME_YAML_SUMMARY.md](../../RUNTIME_YAML_SUMMARY.md) — Implementation architecture and design decisions
- [load_scenario_example.sh](../../examples/load_scenario_example.sh) — Working shell script with all examples

---

## Theoretical Basis

Jiminy Advisor implements the following reasoning steps:

1. **Detachment**  
   Norms whose preconditions match the current context generate arguments.

2. **Argumentation Framework**  
   Attack relations are defined by contrariness between conclusions.

3. **Prioritized Extension**  
   Conflicts are resolved using a priority order `≻`:
   > *the highest-priority conclusion survives, others are defeated.*

4. **Explanation Layer**  
   Not part of formal semantics but crucial for transparency and debugging.

The implementation corresponds directly to normative argumentation semantics used in the original Jiminy papers.

---

## Integration with MERLIN2 (Conceptual)

Jiminy can be used as a **moral gating mechanism** for MERLIN2 robotic behaviors:

- Use Jiminy *before executing* a YASMIN state machine action.
- Use the engine to filter *which PDDL actions* may be executed.
- Add “ethical preconditions” in the Executive Layer via Jiminy conclusions.

Planned future developments:

- Ethical validation at the Planning Layer (post-PDDL filtering)
- Dynamic institutional fact generation from robot perception
- Integration with KANT (symbolic knowledge base)

---

##  Current Features

- ✔ Norm detachment  
- ✔ Institutional, regulative, and permissive norms  
- ✔ Dung-style attack relation  
- ✔ Priority-based defeat resolution  
- ✔ Human-readable explanations  
- ✔ YAML-based scenarios  
- ✔ Graph visualization (NetworkX)  
- ✔ Clean Python API  

---


##  License

This project is released under the **MIT License**, enabling open academic and industrial reuse.

---

##  Acknowledgments

This work is inspired by:

- **Jiminy Advisor** — normative reasoning framework for moral-sensitive robotics.  

If you use this implementation (Jiminy-Lite) in research or teaching materials, please cite:

```markdown
The Jiminy Advisor: Moral Agreements among Stakeholders Based on Norms and Argumentation
Bei Shui Liao, Pere Pardo, Marija Slavković, Leendert van der Torre
Journal of Artificial Intelligence Research, Vol. 77, pp. 737–792
Published: July 11, 2023
DOI: https://doi.org/10.1613/jair.1.14368
```

---
