# Project Template Guide

ApexRoboticsStudio v0.9 adds `.arstemplate` files so you can package a reusable robot-cell starter with defaults for owner, process family, timing, safety radius, and obstacle offsets.

Typical flow:

```bash
apex_robotics_studio export-demo-template painting_cell.arstemplate
apex_robotics_studio instantiate-template painting_cell.arstemplate instantiated.arsproject InterviewPaintCell
```

Use templates when you want a repeatable engineering starting point instead of copying old projects by hand.
