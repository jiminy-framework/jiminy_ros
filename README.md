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
X A graph (`escenario.jpg`) showing nodes and attacks  

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
