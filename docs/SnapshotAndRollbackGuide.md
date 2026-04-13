# Snapshot and Rollback Guide

ApexRoboticsStudio v1.0 adds project snapshot support so you can archive a validated revision and restore it later.

## Why this matters

Robot workstation projects change quickly during commissioning. A team often needs a frozen, reviewable revision that can be restored if a later edit introduces regressions.

## Command flow

```bash
apex_robotics_studio create-project-snapshot demo.arsproject snapshot_dir runtime.ini demo.arsapproval
apex_robotics_studio restore-project-snapshot snapshot_dir restored.arsproject restored_runtime.ini restored.arsapproval
```

## Snapshot contents

A snapshot directory contains:

- `project/`
- `config/`
- `governance/`
- `snapshot_manifest.json`
- `index.html`

## Best practice

Create a snapshot after release-gate evaluation and approval verification. That gives you a stable rollback point for deployment, customer acceptance, or regression triage.
