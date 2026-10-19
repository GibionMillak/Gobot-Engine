# Built-in editor icons

SVG assets use lowercase snake_case filenames in category subfolders. An icon's
identifier is its path relative to this directory, without `.svg`, with `/`
replaced by `_`. For example, `action/copy.svg` becomes `action_copy` and
`node/3d/mesh_instance_3d.svg` becomes `node_3d_mesh_instance_3d`.
UI code uses these identifiers in the `EditorIcons` theme catalog.

Categories describe the represented concept. Node icons are grouped under
`node`, including `node/2d`, `node/3d`, `node/control`, and `node/viewport`.
Resources, Variant types, actions, interface graphics, and editor tools have
separate categories. Moving or renaming an SVG changes its identifier; update
its callers in the same change.

Optional JSON metadata records an exact engine class or Variant `type`
association and/or `thumbnail` sizing (`medium` or `big`). Both files share a
stem. For example, `node/3d/multi_mesh_instance_3d.json` contains:

```json
{
  "type": "MultiMeshInstance3D"
}
```

Type names are explicit because filenames cannot reliably reconstruct names
such as `HTTPRequest`, `CPUParticles2D`, and `AABB`, and UI icons must not be
mistaken for class icons. `editor_get_type_icon_name()` resolves a type to its
asset identifier, returning an empty name for types without an associated icon.
Icons without type or thumbnail metadata need only an SVG. Move or rename an
icon's optional JSON file along with its SVG.

Built-in module icons live under `module/<module_name>` and are embedded only
when that module is enabled. External module icon directories follow the same
conventions, with identifiers prefixed by `module_<module_name>_`.

The build recursively discovers SVG and JSON files. It rejects orphan metadata,
invalid metadata, non-snake_case asset paths, duplicate identifiers, and duplicate
type associations. Duplicate errors identify both source paths. `not_in_use`
directories are excluded from discovery.

`SCsub` invokes `editor_icons_builders.py` to generate `editor_icons.gen.h` here.
Do not edit the generated header manually. `editor_icons.cpp` configures icon
rendering, theme registration, thumbnail sizes, and type-name lookup.
