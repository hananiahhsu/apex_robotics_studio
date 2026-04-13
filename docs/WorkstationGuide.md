# Apex Robotics Workstation Guide

## What this round adds

Version 0.7 pushes the project from a backend-heavy prototype toward a real workstation product path.
The main additions are:

- Project catalog / resource tree generation
- Execution session recording
- Interactive workstation dashboard HTML export
- Batch dashboard export
- Optional Qt desktop workbench skeleton

## New CLI commands

```bash
apex_robotics_studio project-tree demo_workcell_v07.arsproject
apex_robotics_studio record-job-session demo_workcell_v07.arsproject apex_session runtime_v07.ini plugins
apex_robotics_studio dashboard-project demo_workcell_v07.arsproject workstation_dashboard.html runtime_v07.ini plugins
apex_robotics_studio dashboard-batch demo_validation.arsbatch batch_dashboard.html
```

## Product positioning

These commands are not just helpers. Together they model a realistic industrial workflow:

1. Inspect project structure and resources
2. Execute a robot job simulation
3. Persist an execution session for audit / traceability
4. Export a customer-facing workstation dashboard
5. Repeat the same pattern for batch validation

## Session output

`record-job-session` writes:

- `session_summary.json`
- `session_events.jsonl`
- `session_summary.html`

This is intentionally aligned with how companies typically separate:

- machine-readable output
- append-friendly event logs
- human-readable summaries

## Dashboard output

`dashboard-project` produces a single HTML file with:

- overview cards
- project catalog table
- embedded SVG workcell view
- segment execution table
- plugin findings
- execution session events

This is designed for interview demos, internal design reviews, and customer walk-throughs.
