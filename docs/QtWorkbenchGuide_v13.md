# Qt Workbench Guide v1.3

The Qt frontend remains optional and is controlled by the existing Qt workbench build path.

## v1.3 source-level improvements

The workbench source now wires these actions into the main window implementation:

- open `.arsproject`
- import `URDF` / `Xacro`
- save project as `.arsproject`
- run job analysis
- export SVG preview
- export dashboard HTML
- export session artifacts
- export ROS 2 / MoveIt workspace

## Current validation boundary

The current container does **not** include a Qt SDK, so the Qt executable was **not** compiled here.

What is validated in this version is the backend chain that the Qt workbench calls:

- project serialization
- robot description import
- project catalog generation
- simulation
- dashboard generation
- ROS 2 workspace export

## Intended desktop workflow

1. Import a `URDF` or `Xacro`
2. Inspect the generated project tree
3. Run job analysis
4. Export SVG/dashboard/session artifacts
5. Export a ROS 2 / MoveIt workspace skeleton for downstream robotics integration
