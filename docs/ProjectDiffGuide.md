# Project Diff Guide

ApexRoboticsStudio v0.8 adds project-to-project differencing.

## Why it exists

In real robot workstation projects, engineers need to answer questions like:

- What changed between the last approved cell and this new variant?
- Which fixture moved?
- Did segment timing or sampling change?
- Did the project owner or process family change?

## What is compared

The current diff engine compares:

- project metadata
- motion timing and safety radius
- obstacle set and bounds
- job segments and tags

## Command

```bash
apex_robotics_studio diff-project base.arsproject candidate.arsproject project_diff.txt
```

## Result shape

The text report uses three kinds:

- `ADDED`
- `REMOVED`
- `MODIFIED`

This makes the output easy to discuss in design reviews and code reviews.
