# Apex Robotics Studio Batch Operations Guide

## Purpose

Batch operations make the project look and behave more like a delivery tool used by robotics applications teams:

- validate multiple workcells before release
- compare projects under one runtime policy
- generate a single review report for leads or customers

## Batch manifest format

```ini
[batch]
schema_version=1
name=Demo Production Validation

[entry]
name=primary_demo
project=../projects/demo_workcell_v05.arsproject
runtime=../../config/runtime_v05.ini
plugin_directory=../plugins
```

## Commands

### Export a demo batch

```bash
apex_robotics_studio export-demo-batch out/demo_batch.arsbatch
```

### Run a batch

```bash
apex_robotics_studio run-batch out/demo_batch.arsbatch
```

### Save an HTML report

```bash
apex_robotics_studio report-batch out/demo_batch.arsbatch out/demo_batch.html
```
