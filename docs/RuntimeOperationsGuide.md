# Runtime Operations Guide

## Runtime configuration

ApexRoboticsStudio v0.5 adds a runtime configuration file to make execution behavior explicit and reviewable.

Default keys:

- `report_theme`
- `preferred_joint_velocity_limit_rad_per_sec`
- `joint_limit_margin_deg`
- `enable_audit_plugins`
- `generate_segment_summary_files`
- `minimum_log_level`

Export a default file:

```bash
apex_robotics_studio export-runtime-config runtime.ini
```

Run a job with configuration:

```bash
apex_robotics_studio run-job-project demo_workcell_v05.arsproject runtime.ini
```

## Operational best practice

For real teams, the recommended split is:

- project file stores engineering intent and workcell content
- runtime config stores review thresholds and operational toggles
- generated HTML/SVG reports become traceable review artifacts

## Logging

The current logger is intentionally lightweight but already demonstrates:

- log levels
- explicit runtime-controlled filtering
- structured console output that is easy to pipe into CI logs

This is a good base for later integration with:

- spdlog
- ETW / Windows event logging
- syslog / journald
- structured JSON logs for production observability
