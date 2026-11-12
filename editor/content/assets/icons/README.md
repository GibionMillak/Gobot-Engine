# Editor icons

SVG assets use lowercase snake_case filenames in category subfolders. An icon's
identifier is its path relative to this directory, without `.svg`, with `/`
replaced by `_`. For example, `action/copy.svg` becomes `action_copy` and
`node/3d/mesh_instance_3d.svg` becomes `node_3d_mesh_instance_3d`.
UI code uses these identifiers in the `EditorIcons` theme catalog.

Node icons are grouped under `node`, with subfolders for `2d`, `3d`, `control`,
and `viewport`. Resource class icons are grouped under `resource`. Other
categories hold Variant types, actions, interface graphics, and editor tools.
Moving or renaming an SVG changes its identifier; update its callers in the same
change.

Class icon associations are inferred from the filename under `node` or
`resource`, converting snake_case to PascalCase. Optional JSON metadata
records an explicit `type` for names that do not convert directly, such as
`HTTPRequest`, `CPUParticles2D`, and `AABB`, and/or `thumbnail` sizing
(`medium` or `big`). Both files share a stem. For example,
`node/2d/cpu_particles_2d.json` can contain:

```json
{
  "type": "CPUParticles2D"
}
```

Variant type associations remain explicit in JSON metadata. UI icons belong in
their own categories so they are not associated with engine classes.
`editor_get_type_icon_name()` resolves a type to its asset identifier,
returning an empty name for types without an associated icon. Move or rename an
icon's optional JSON file along with its SVG.

Bundled module icons live under `module/<module_name>`. They receive editor theme
entries only when that module is enabled. Their SVG files remain available as
public editor content whenever the SVG loader is enabled, even if the module
they depict is disabled. External module icon directories are used only for
enabled modules and receive theme identifiers prefixed by `module_<module_name>_`;
assets outside `editor/content` are not part of the public content namespace.

The build recursively discovers SVG and JSON files. It rejects orphan metadata,
invalid metadata, non-snake_case asset paths, duplicate identifiers, and duplicate
type associations. Duplicate errors identify both source paths.

`SCsub` invokes `editor_icons_builders.py` to generate `editor_icons.gen.h`
declarations and `editor_icons.gen.cpp` storage here. Do not edit generated files
manually. `editor/themes/editor_icons.cpp` configures icon rendering, theme
registration, thumbnail sizes, and type-name lookup.

Other supported texture formats are exposed as public content through the parent
content builder; SVG-specific metadata and theme registration apply only to SVGs.
