# Apex Robotics Studio v0.6 Deployment and Extensibility

## New capabilities in v0.6

This version adds three product-facing subsystems:

1. **External audit tool protocol**
   - Plugin integration now supports executable-based audit tools through `.arsplugin` manifests.
   - This avoids fragile C++ ABI coupling across shared library boundaries.
   - Each plugin receives a project file and writes a findings file using a stable text contract.

2. **Bundle packaging**
   - A project, runtime config, plugins, and generated reports can now be assembled into a deployable bundle folder.
   - Bundle layout:
     - `bundle.arsbundle`
     - `project/workcell.arsproject`
     - `config/runtime.ini`
     - `plugins/`
     - `reports/`

3. **Batch validation**
   - Multiple projects can be validated in one batch run.
   - The batch runner aggregates success/failure, path metrics, collision state, and plugin findings.

## Why executable plugins?

For commercial software, out-of-process integrations are often easier to version, test, and cross-compile than direct C++ shared-library ABI coupling.
This project therefore supports a stable manifest-driven plugin protocol while still keeping the core SDK usable in-process.

## Plugin manifest

```ini
[plugin]
name=SampleSafetyAudit
executable=apex_audit_tool_sample
arguments=audit {project} {output}
```

## Findings protocol

Each plugin writes one line per finding:

```text
warning|No safety fence style obstacle detected in workcell.
info|Job contains four or more segments and looks suitable for interview demonstration.
```

## Bundle workflow

```bash
apex_robotics_studio bundle-project demo.arsproject runtime.ini out/bundles/demo_bundle samples/plugins
apex_robotics_studio run-bundle out/bundles/demo_bundle
```

## Batch workflow

```bash
apex_robotics_studio export-demo-batch samples/batches/generated_demo_batch.arsbatch
apex_robotics_studio run-batch samples/batches/generated_demo_batch.arsbatch
apex_robotics_studio report-batch samples/batches/generated_demo_batch.arsbatch out/reports/demo_batch.html
```
