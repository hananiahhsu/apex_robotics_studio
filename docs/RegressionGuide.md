# Project Regression Guide

ApexRoboticsStudio v0.9 adds candidate-vs-baseline regression analysis.

Typical flow:

```bash
apex_robotics_studio regress-project baseline.arsproject candidate.arsproject regression.html runtime.ini
```

The report compares gate status, collisions, findings, cycle time, and TCP path length.
