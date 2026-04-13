# Integration Export Guide

ApexRoboticsStudio v1.0 adds an integration package exporter for downstream systems.

## Why this matters

Real robotics deployments often need machine-readable outputs for MES, QA dashboards, traceability systems, or enterprise review tooling.

## Command

```bash
apex_robotics_studio export-integration-package demo.arsproject integration_dir runtime.ini plugins demo.arsapproval
```

## Output artifacts

The exporter writes:

- `summary.json`
- `segments.csv`
- `findings.csv`
- `obstacles.csv`
- `README.txt`

## Included metrics

- project fingerprint
- gate status
- approval decision
- collision-free state
- segment count
- obstacle count
- finding count
- total duration
- total path length

## Best practice

Use the integration package as the machine-readable complement to the workstation dashboard, release gate HTML, and delivery dossier.
