# Approval Workflow Guide

ApexRoboticsStudio v1.0 adds `.arsapproval` files to model real delivery sign-off.

## Why this matters

Industrial robotics projects usually need explicit approvals before release. A workstation project can be technically valid but still not be cleared for production until the responsible roles sign the current revision.

## File type

- `.arsapproval`: role-based approval workflow document

## Typical flow

1. Export a template.
2. Each role signs the current project fingerprint.
3. Verify the document against the current project revision.
4. Publish the HTML report into the delivery dossier or project review package.

## Commands

```bash
apex_robotics_studio export-approval-template demo.arsapproval
apex_robotics_studio sign-project demo.arsproject demo.arsapproval robotics_lead alice approved "Lead sign-off"
apex_robotics_studio sign-project demo.arsproject demo.arsapproval process_owner bob approved "Process sign-off"
apex_robotics_studio sign-project demo.arsproject demo.arsapproval qa carol approved "QA sign-off"
apex_robotics_studio verify-approval demo.arsproject demo.arsapproval approval_report.html
```

## Output

The verification step produces an HTML report with:

- current project fingerprint
- overall approval decision
- per-role approval status
- signer, timestamp, and notes

## Best practice

Treat approval files as revision-specific artifacts. If the project changes, verify again and expect previous signatures to become stale for the new fingerprint.
