# Project File Format

ApexRoboticsStudio stores workspace data in a human-readable `.arsproject` text format.

## Supported schema versions

- **Schema 1**: project, joints, obstacles, and a single motion request
- **Schema 2**: schema 1 plus a multi-step robot job with waypoints and segments

## Core sections

### `[project]`
- `schema_version`
- `project_name`
- `robot_name`

### `[joint]`
- `name`
- `link_length`
- `min_angle_deg`
- `max_angle_deg`

### `[obstacle]`
- `name`
- `min`
- `max`

### `[motion]`
- `start_angles_deg`
- `goal_angles_deg`
- `sample_count`
- `duration_seconds`
- `link_safety_radius`

## Schema 2 additions

### `[job]`
- `name`
- `link_safety_radius`

### `[waypoint]`
- `name`
- `angles_deg`

### `[segment]`
- `name`
- `start_waypoint`
- `goal_waypoint`
- `sample_count`
- `duration_seconds`
- `process_tag`

## Example

```ini
[project]
schema_version=2
project_name=Door Panel Paint Demo
robot_name=Apex Painter 6A

[joint]
name=J1
link_length=0.45
min_angle_deg=-180
max_angle_deg=180

[motion]
start_angles_deg=0,10,20,-15,5,0
goal_angles_deg=35,25,-30,15,-20,10
sample_count=16
duration_seconds=4.0
link_safety_radius=0.08

[job]
name=Door Panel Paint Cycle
link_safety_radius=0.08

[waypoint]
name=Home
angles_deg=0,15,20,-15,0,0

[segment]
name=MoveToApproach
start_waypoint=Home
goal_waypoint=Approach
sample_count=12
duration_seconds=2.0
process_tag=approach
```

## Design rationale

This format is intentionally simple:

- easy to diff in Git
- easy to generate from scripts
- easy to explain in interviews
- still expressive enough for workstation demos and offline programming workflows

## Schema v3 additions

Version 0.8 introduces schema version 3 for product-level metadata and quality gates.

### `[metadata]`
```ini
[metadata]
cell_name=Cell-07-Paint
process_family=painting
owner=hananiah
notes=Interview-grade robot workstation scenario
```

### `[quality_gate]`
```ini
[quality_gate]
require_collision_free=true
max_error_findings=0
max_warning_findings=2
max_peak_tcp_speed_mps=1.8
max_average_tcp_speed_mps=1.2
preferred_joint_velocity_limit_rad_per_sec=1.5
```

These sections let the same project file carry:
- engineering ownership
- process-family classification
- release-readiness thresholds
- gate evaluation defaults
