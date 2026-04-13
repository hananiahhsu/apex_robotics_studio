# Plugin Architecture

## Goal

Industrial products usually need rule checks that can evolve without rewriting the planning core. ApexRoboticsStudio v0.5 introduces a lightweight audit-plugin architecture for that purpose.

## Main types

- `apex::extension::IProjectAuditPlugin`
- `apex::extension::PluginFinding`
- `apex::extension::ProjectAuditPluginRegistry`

## Built-in plugins

Current built-ins are intentionally simple but product-shaped:

- `JointLimitMarginAudit`
- `ObstacleCoverageAudit`
- `JobStructureAudit`

These plugins return findings with one of three severities:

- `info`
- `warning`
- `error`

## Why this matters in a company

This pattern maps well to real business needs:

- project QA gates
- vertical-industry rule checks
- customer-specific review standards
- deployment-specific safety heuristics

The core planner remains stable while plugins grow around it.

## Next-step evolution

A production team could extend this toward:

- dynamic shared-library plugin loading
- plugin manifests and version negotiation
- customer policy packs
- scripting-based rule plugins
- CI enforcement for project review gates


## v0.6 external audit protocol

This project now supports executable-based external audit plugins in addition to built-in in-process audit rules.

Manifest format:

```ini
[plugin]
name=SampleSafetyAudit
executable=apex_audit_tool_sample
arguments=audit {project} {output}
```

The host replaces `{project}` and `{output}` at runtime, launches the executable, and then parses the findings file written by the plugin.
This model is intentionally simpler to cross-compile and more stable across compiler boundaries than direct C++ shared-library ABI coupling.
