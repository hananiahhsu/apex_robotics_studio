# Release Gate Guide

ApexRoboticsStudio v0.8 adds a release-gate evaluator for project sign-off.

## What it evaluates

The gate combines:

- job simulation output
- plugin findings
- project quality-gate thresholds

The evaluator currently checks:

- collision-free requirement
- error finding count
- warning finding count
- peak TCP speed
- average TCP speed

## Status model

- `PASS`: ready for the next stage
- `HOLD`: technically runnable, but requires review
- `FAIL`: blocked from release

## Project-side configuration

Projects can now embed a `[quality_gate]` section:

```ini
[quality_gate]
require_collision_free=true
max_error_findings=0
max_warning_findings=2
max_peak_tcp_speed_mps=1.8
max_average_tcp_speed_mps=1.2
preferred_joint_velocity_limit_rad_per_sec=1.5
```

## Command

```bash
apex_robotics_studio gate-project demo_workcell_v08.arsproject release_gate.html runtime_v08.ini plugins
```

The HTML report is suitable for:

- interview demos
- internal review meetings
- manufacturing readiness review
- release checklist handoff
