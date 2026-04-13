# ApexRoboticsStudio

ApexRoboticsStudio is a cross-platform C++ robotics workstation interview project designed to demonstrate product-oriented engineering rather than isolated algorithm demos.

Current release: **v1.3.0**.

## Main capabilities

- serial robot model and forward kinematics
- workcell obstacle modeling and collision evaluation
- joint-space trajectory generation
- schema-versioned `.arsproject` files
- multi-step robot job programming with named waypoints and segments
- built-in audit plugin registry for project review findings
- runtime configuration and logging
- URDF / Xacro import for serial robots
- mesh resource capture from URDF/Xacro visual/collision tags
- trajectory quality analysis
- SVG workcell export for single motions and whole jobs
- HTML project report generation for engineering review and interviews
- cross-platform and cross-compilation ready CMake build
- SDK-style install/export for downstream consumers
- process recipes (`.arsrecipe`) for reusable manufacturing standards
- project patches (`.arspatch`) for localized engineering changes
- release-gate evaluation with HTML sign-off reports
- project-to-project diff reports for review workflows
- project templates (`.arstemplate`) for repeatable engineering starters
- baseline-vs-candidate regression analysis with HTML reports
- delivery dossiers that gather project, config, reports, and session evidence
- ROS 2 / MoveIt workspace skeleton export
- optional Qt Widgets workstation frontend with backend execution hooks

## Quick start

### Linux

```bash
./scripts/build_linux.sh
./out/build/linux-release/apex_robotics_studio export-demo-project demo_workcell_v05.arsproject
./out/build/linux-release/apex_robotics_studio export-runtime-config runtime.ini
./out/build/linux-release/apex_robotics_studio inspect-project demo_workcell_v05.arsproject
./out/build/linux-release/apex_robotics_studio run-job-project demo_workcell_v05.arsproject runtime.ini
./out/build/linux-release/apex_robotics_studio render-project-svg demo_workcell_v05.arsproject demo.svg
./out/build/linux-release/apex_robotics_studio report-job-project demo_workcell_v05.arsproject demo_job.html runtime.ini
```

### Robot description import example

```bash
./out/build/linux-release/apex_robotics_studio import-robot-description samples/xacro/demo_painter_cell.xacro imported_demo.arsproject
./out/build/linux-release/apex_robotics_studio inspect-project imported_demo.arsproject
./out/build/linux-release/apex_robotics_studio export-ros2-workspace imported_demo.arsproject generated_ws
```

## Product positioning

Use this project as a robotics workstation / offline programming / digital-twin product prototype. It demonstrates:

- C++ system design with modular SDK-style libraries
- CMake, packaging, and cross-compilation discipline
- offline programming and execution-review workflows
- plugin-based engineering review gates
- configuration, reporting, and customer-facing deliverables


## v0.6 highlights

- executable-based external audit plugin protocol (`.arsplugin`)
- deployable project bundles (`bundle.arsbundle`)
- batch validation manifests (`.arsbatch`)
- sample external audit tool for interview demos
- stronger release/deployment docs for product-style delivery


## v0.7 Workstation additions

- Project catalog / resource tree export
- Execution session recording (`session_summary.json`, `session_events.jsonl`, `session_summary.html`)
- Workstation dashboard HTML export
- Batch dashboard HTML export
- Optional Qt desktop workbench skeleton via `ARS_ENABLE_QT_FRONTEND`


## v0.8 Platform additions

- process recipe loader and applicator
- project patch loader and scene editor
- release-gate evaluator with PASS/HOLD/FAIL
- project diff engine for change reviews
- metadata and quality-gate fields in schema v3 `.arsproject`


## v0.9 Platform additions

- reusable project templates and template instantiation
- baseline/candidate regression analysis for engineering changes
- delivery dossier generation for customer or internal handoff
- serializer string APIs for in-memory project transformations


## v1.0 Governance, Snapshot, and Integration

This version adds three product-grade capabilities that are common in real robotics workstation deployments:

- governance approvals with role-based sign-off files (`.arsapproval`)
- project snapshots and restore workflows for traceability and rollback
- machine-readable integration packages for downstream systems

New CLI commands:

```bash
apex_robotics_studio export-approval-template demo.arsapproval
apex_robotics_studio sign-project demo.arsproject demo.arsapproval robotics_lead alice approved
apex_robotics_studio verify-approval demo.arsproject demo.arsapproval approval_report.html
apex_robotics_studio create-project-snapshot demo.arsproject snapshot_dir runtime.ini demo.arsapproval
apex_robotics_studio restore-project-snapshot snapshot_dir restored.arsproject restored_runtime.ini restored.arsapproval
apex_robotics_studio export-integration-package demo.arsproject integration_dir runtime.ini plugins demo.arsapproval
```


## v1.1.0 Enhancements

- Automation flow orchestration via `.arsflow`
- Parameter sweep analysis via `.arssweep`
- Structured audit trail export for review and governance


## v1.2 Robot Description / ROS 2 / Qt additions

- Xacro-aware robot description import with a limited built-in xacro expander and optional external `xacro` tool usage
- mesh URI capture into schema v4 `.arsproject` files
- generated ROS 2 `ament_cmake` bringup workspace skeletons
- generated MoveIt package skeletons with `MoveGroupInterface` and `PlanningSceneInterface` smoke-test code
- Qt Widgets workstation frontend upgraded from preview shell to a real backend-connected workbench


## v1.3 Robot Description / ROS 2 / Qt depth upgrade

- Added deeper `xacro` handling with `$(arg ...)`, `$(find ...)`, package root discovery, and mesh URI resolution.
- Added mesh resource persistence for resolved paths and package names in `.arsproject` schema v5.
- Added ROS 2 / MoveIt workspace export with a generated `*_description` package, copied mesh assets, URI rewriting, display launch, bringup package, and MoveIt smoke-test sources.
- Upgraded the optional Qt workbench source to support save-project, SVG export, session export, dashboard export, and ROS 2 workspace export actions from the GUI source tree.


## v1.7 highlights

- `inspect-robot-description <project.arsproject> [output.{html|json}]`
- ROS2 workspace export now includes `workspace_doctor.py`, `dependency_catalog.md`, `resource_manifest.json`, and `full_stack_demo.launch.py`
- Qt workbench source now includes a robot-description inspection export action and richer artifact state text
