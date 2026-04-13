# Job Programming Guide

## Why this module exists

Industrial robot software rarely executes a single start-to-goal move. Real projects define:

- named waypoints such as Home, Approach, ProcessStart, ProcessEnd, Retreat
- multiple motion segments connecting those waypoints
- process tags that identify the engineering intent of each segment

ApexRoboticsStudio v0.5 adds this layer through the `apex::job` and `apex::simulation` modules.

## Project-file model

Schema version 2 adds three new section types:

- `[job]`
- `[waypoint]`
- `[segment]`

Example:

```ini
[job]
name=Door Panel Paint Cycle
link_safety_radius=0.080000

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

## Runtime flow

1. Load `StudioProject`
2. Build `SerialRobotModel` and `CollisionWorld`
3. Resolve named waypoints
4. Plan each segment with `JointTrajectoryPlanner`
5. Analyze each segment with `TrajectoryAnalyzer`
6. Aggregate segment results into one `JobSimulationResult`
7. Emit SVG, HTML, and console summaries

## Interview talking points

This design lets you explain:

- how a robotics workstation separates product semantics from pure motion planning
- why named waypoints are friendlier to application engineers than raw vectors
- how multi-step jobs support paint, weld, glue, palletizing, or inspection scenarios
- why segment-level analysis is needed for production debugging and cycle-time review
