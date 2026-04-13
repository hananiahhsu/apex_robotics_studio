#!/usr/bin/env python3
import json
from pathlib import Path

WORKSPACE_ROOT = Path(__file__).resolve().parent
required = [
    ('apexpainterv16_imported_project_description', WORKSPACE_ROOT / 'src' / 'apexpainterv16_imported_project_description'),
    ('apexpainterv16_imported_project_bringup', WORKSPACE_ROOT / 'src' / 'apexpainterv16_imported_project_bringup'),
    ('apexpainterv16_imported_project_moveit_config', WORKSPACE_ROOT / 'src' / 'apexpainterv16_imported_project_moveit_config'),
    ('apexpainterv16_imported_project_moveit_task_demo', WORKSPACE_ROOT / 'src' / 'apexpainterv16_imported_project_moveit_task_demo'),
]

missing = [name for name, path in required if not path.exists()]
manifest = WORKSPACE_ROOT / 'workspace_manifest.json'
print('Apex ROS2 Workspace Doctor')
print('Workspace:', WORKSPACE_ROOT)
print('Manifest :', manifest)
if missing:
    print('Missing packages:')
    for item in missing:
        print('  -', item)
else:
    print('All generated packages are present.')
if manifest.exists():
    print('Manifest summary:')
    data = json.loads(manifest.read_text(encoding='utf-8'))
    print(json.dumps(data, indent=2))
else:
    print('workspace_manifest.json is missing.')
raise SystemExit(1 if missing else 0)
