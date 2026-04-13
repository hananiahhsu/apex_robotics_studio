# Recipe And Patch Guide

ApexRoboticsStudio v0.8 adds two product-facing configuration layers that are common in real industrial software.

## 1. Process recipe (`.arsrecipe`)
A recipe standardizes a process family across many cells or many project variants.

Typical contents:

- process family
- owner
- default segment sample count
- duration scale
- link safety radius
- release-gate limits
- process-tag prefix

Example:

```ini
[recipe]
name=Automotive Painting Standard
process_family=painting
owner=manufacturing.engineering
notes=Standard spray-cell defaults
process_tag_prefix=paint

[motion]
default_sample_count=20
segment_duration_scale=1.15
link_safety_radius=0.09

[quality_gate]
max_peak_tcp_speed_mps=1.4
max_average_tcp_speed_mps=0.9
preferred_joint_velocity_limit_rad_per_sec=1.2
max_warning_findings=1
require_collision_free=true
```

Use:

```bash
apex_robotics_studio export-demo-recipe painting_standard.arsrecipe
apex_robotics_studio apply-recipe demo_workcell_v08.arsproject painting_standard.arsrecipe recipe_applied.arsproject
```

## 2. Project patch (`.arspatch`)
A patch applies a localized engineering change without rebuilding the whole project by hand.

Typical use cases:

- move a fixture
- add a scanner stand or fence
- remove an obsolete obstacle
- tighten sample count / duration / safety radius
- change project owner metadata

Example:

```ini
[patch]
name=Shift Fixture And Tighten Motion
notes=Moves a fixture and tightens motion sampling for validation

[metadata]
owner=robotics.integration

[motion]
sample_count=22
duration_seconds=5.0
link_safety_radius=0.10

[translate_obstacle]
name=Fixture_A
delta=0.06,-0.04,0.0

[add_obstacle]
name=ScannerStand
min=0.15,0.55,-0.10
max=0.28,0.68,0.85
```

Use:

```bash
apex_robotics_studio export-demo-patch shift_fixture.arspatch
apex_robotics_studio apply-project-patch demo_workcell_v08.arsproject shift_fixture.arspatch patched_project.arsproject
```

## Why this matters in interviews

This lets you describe the system as more than a simulator:

- recipes provide process-level reuse
- patches provide engineering-change management
- both keep the core project format stable while supporting controlled variation
