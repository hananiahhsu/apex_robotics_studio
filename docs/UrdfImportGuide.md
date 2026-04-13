# URDF Import Guide

## Supported scope

The built-in importer is intentionally scoped for the interview product prototype:

- serial revolute joints
- continuous joints
- fixed joints are skipped
- link length is derived from the joint `origin xyz`

It is sufficient for building a workstation-style demo pipeline without introducing a heavy XML dependency stack.

## Example

```bash
./out/build/linux-release/apex_robotics_studio import-urdf samples/urdf/demo_painter.urdf imported_demo.arsproject
./out/build/linux-release/apex_robotics_studio run-project imported_demo.arsproject
./out/build/linux-release/apex_robotics_studio render-project-svg imported_demo.arsproject imported_demo.svg
./out/build/linux-release/apex_robotics_studio report-project imported_demo.arsproject imported_demo.html
```

## Extension path

In a production team, the next steps would be:

- switch to a full XML/URDF stack such as `urdfdom`
- add inertial, visual, and collision geometry loading
- resolve parent/child link graphs formally
- support mesh packages and package URI resolution
- map imported frames into a richer scene graph
