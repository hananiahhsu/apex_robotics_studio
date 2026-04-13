# Parameter Sweep Guide

`*.arssweep` defines a matrix of sample counts, duration scales, and safety radii.

Outputs:
- `sweep_summary.csv`
- `sweep_summary.html`
- generated variant projects
- `audit/` trail

CLI:
```bash
apex_robotics_studio export-demo-sweep demo_sweep.arssweep
apex_robotics_studio run-sweep demo_sweep.arssweep
```
