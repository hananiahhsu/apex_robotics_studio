# ApexRoboticsStudio Architecture

## 1. Product positioning

ApexRoboticsStudio is positioned as a **robot workstation software foundation** rather than an isolated algorithm demo. The software boundary is intentionally chosen so it can grow into:

- Robot offline programming software
- Robot workcell simulation software
- Digital twin runtime platform
- Equipment upper-computer software
- Robot process planning software

## 2. Layered architecture

```text
+-----------------------------------------------------------+
| App Layer                                                 |
| CLI today / future Qt or ImGui workstation shell          |
+-----------------------------------------------------------+
| Planning Layer                                            |
| Trajectory planning, sampling, collision tagging          |
+-----------------------------------------------------------+
| Workcell Layer                                            |
| Obstacle model, environment query, collision reports      |
+-----------------------------------------------------------+
| Core Layer                                                |
| Robot model, math types, joint state, TCP computation     |
+-----------------------------------------------------------+
| Platform / Delivery Layer                                 |
| CMake, presets, toolchains, CI, install package, scripts  |
+-----------------------------------------------------------+
```

## 3. Business-facing modules

### 3.1 Core
Responsible for stable robot domain abstractions:

- Robot metadata
- Joint definitions
- Joint state dimension validation
- Joint limit validation
- Forward kinematics result
- TCP pose computation

### 3.2 Workcell
Responsible for modeling the robot execution environment:

- Obstacles
- Environment bounds
- Collision checks
- Collision report generation

### 3.3 Planning
Responsible for task-level simulation output:

- Linear joint interpolation
- Time sampling
- TCP path extraction
- Per-sample collision tagging

### 3.4 Delivery / Tooling
Responsible for turning source code into a real company asset:

- Reproducible host builds
- Cross builds for new targets
- Installable package exports
- CI verification
- Dependency strategy scaffolding

## 4. Commercial-grade delivery decisions

### 4.1 Checked-in presets
`CMakePresets.json` is committed and describes all approved build entry points. That avoids ad-hoc shell knowledge and makes CI, local development, and onboarding align.

### 4.2 Toolchain isolation
Cross-compilation details live in `cmake/toolchains/` so that target logic is versioned and reviewable.

### 4.3 Relocatable install package
The project now installs export targets and generates `ApexRoboticsStudioConfig.cmake` / `ApexRoboticsStudioConfigVersion.cmake`, so downstream products can consume the SDK using `find_package()`.

### 4.4 vcpkg growth path
The current code has no third-party libraries, but `vcpkg/triplets/` is already prepared so future additions such as `fmt`, `Eigen`, `urdfdom`, or serialization libraries can be brought in without redesigning the build topology.

### 4.5 Cross target selection
Two cross targets are selected because they are common in actual business:

- **Linux x64 -> Linux ARM64**: edge controllers, industrial gateways, robot cell appliances.
- **Linux x64 -> Windows x64**: Windows desktop engineering workstation delivery from Linux CI.

## 5. Recommended next architectural evolution

### Phase A: product usability
- Add scene and task persistence
- Add configuration files for robot families
- Add structured logging and diagnostics

### Phase B: robotics interoperability
- Add URDF import
- Add ROS 2 topic/service bridge
- Add workcell frame tree model

### Phase C: workstation shell
- Add Qt/ImGui desktop UI
- Add scene tree, property editor, target point editor
- Add playback timeline and collision report panel

### Phase D: commercial platformization
- Add installers and package signing
- Add SBOM and license scanning
- Add integration tests across supported targets
- Add binary cache and dependency mirror strategy
