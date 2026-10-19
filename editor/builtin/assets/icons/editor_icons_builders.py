"""Functions used to generate source files during build time"""

import json
import re
from pathlib import Path

import methods


# See also `scene/theme/icons/default_theme_icons_builders.py`.
def make_editor_icons_action(target, source, env):
    icons_names = []
    icons_raw = []
    icons_med = []
    icons_big = []
    type_entries = []
    identifiers = {}
    types = {}
    roots = [(Path(path).resolve(), prefix) for path, prefix in env["editor_icon_roots"]]
    icon_paths = sorted(Path(str(item)).resolve() for item in source if str(item).endswith(".svg"))
    icon_path_set = set(icon_paths)
    icon_metadata = {}
    for item in source:
        if not str(item).endswith(".json"):
            continue
        metadata_path = Path(str(item)).resolve()
        asset_path = metadata_path.with_suffix(".svg")
        if asset_path not in icon_path_set:
            raise ValueError(f"Icon metadata has no matching SVG: {metadata_path}")
        try:
            metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
        except json.JSONDecodeError as error:
            raise ValueError(f"Invalid icon metadata JSON: {metadata_path}: {error}") from error
        if not isinstance(metadata, dict) or set(metadata) - {"type", "thumbnail"}:
            raise ValueError(f"Invalid icon metadata: {metadata_path}")
        if "thumbnail" in metadata and metadata["thumbnail"] not in ("medium", "big"):
            raise ValueError(f"Invalid thumbnail size: {metadata_path}")
        if "type" in metadata and (not isinstance(metadata["type"], str) or not metadata["type"]):
            raise ValueError(f"Invalid icon type association: {metadata_path}")
        icon_metadata[asset_path] = metadata

    for idx, asset_path in enumerate(icon_paths):
        path = str(asset_path)
        icon_root, prefix = next((root, prefix) for root, prefix in roots if asset_path.is_relative_to(root))
        relative_path = asset_path.relative_to(icon_root).as_posix()
        stem_path = relative_path.removesuffix(".svg")
        for part in stem_path.split("/"):
            if not re.fullmatch(r"[a-z0-9]+(?:_[a-z0-9]+)*", part):
                raise ValueError(f"Icon paths must use snake_case: {path}")
        name = prefix + stem_path.replace("/", "_")
        if name in identifiers:
            raise ValueError(f"Duplicate icon identifier '{name}': {identifiers[name]} and {path}")
        identifiers[name] = path
        metadata = icon_metadata.get(asset_path, {})
        if "type" in metadata:
            type_name = metadata["type"]
            if type_name in types:
                raise ValueError(f"Duplicate icon type '{type_name}': {types[type_name]} and {path}")
            types[type_name] = path
            type_entries.append(f"{{ {json.dumps(type_name)}, {json.dumps(name)} }}")
        with open(path, encoding="utf-8", newline="\n") as file:
            icons_raw.append(methods.to_raw_cstring(file.read()))

        icons_names.append(f'"{name}"')

        if metadata.get("thumbnail") == "medium":
            icons_med.append(str(idx))
        elif metadata.get("thumbnail") == "big":
            icons_big.append(str(idx))

    icons_names_str = ",\n\t".join(icons_names)
    icons_raw_str = ",\n\t".join(icons_raw)
    types_str = ",\n\t".join(type_entries)

    with methods.generated_wrapper(str(target[0])) as file:
        file.write(f"""\
inline constexpr int editor_icons_count = {len(icons_names)};
inline constexpr const char *editor_icons_sources[] = {{
	{icons_raw_str}
}};

inline constexpr const char *editor_icons_names[] = {{
	{icons_names_str}
}};

struct EditorTypeIcon {{
	const char *type_name;
	const char *icon_name;
}};

inline constexpr EditorTypeIcon EDITOR_TYPE_ICONS[] = {{
	{types_str}
}};

inline constexpr int editor_md_thumbs_count = {len(icons_med)};
inline constexpr int editor_md_thumbs_indices[] = {{ {", ".join(icons_med)} }};

inline constexpr int editor_bg_thumbs_count = {len(icons_big)};
inline constexpr int editor_bg_thumbs_indices[] = {{ {", ".join(icons_big)} }};
""")
