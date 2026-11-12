"""Functions used to generate source files during build time"""

import json
import re
from pathlib import Path

import asset_builders
import methods


# See also `scene/theme/icons/default_theme_icons_builders.py`.
def make_icons(target, source, env):
    icons_names = []
    icons_med = []
    icons_big = []
    type_entries = []
    types = {}
    records = env["editor_asset_records"]
    icon_paths = {Path(record["path"]) for record in records}
    icon_metadata = {}
    for item in env["editor_icon_metadata"]:
        metadata_path = Path(item)
        asset_path = metadata_path.with_suffix(".svg")
        if asset_path not in icon_paths:
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

    for idx, record in enumerate(records):
        asset_path = Path(record["path"])
        path = record["path"]
        relative_path = record["relative_path"]
        name = record["name"]
        metadata = icon_metadata.get(asset_path, {})
        type_name = metadata.get("type")
        if type_name is None and relative_path.startswith(("node/", "resource/")):
            type_name = _class_name_from_icon_stem(asset_path.stem)
        if type_name is not None:
            if type_name in types:
                raise ValueError(f"Duplicate icon type '{type_name}': {types[type_name]} and {path}")
            types[type_name] = path
            type_entries.append(f"{{ {json.dumps(type_name)}, {json.dumps(name)} }}")
        icons_names.append(json.dumps(name))

        if metadata.get("thumbnail") == "medium":
            icons_med.append(str(idx))
        elif metadata.get("thumbnail") == "big":
            icons_big.append(str(idx))

    with (
        methods.generated_wrapper(str(target[0])) as header,
        methods.generated_wrapper(str(target[1])) as output,
    ):
        output.write('#include "editor_icons.gen.h"\n\n')
        header.write("""\
struct EditorTypeIcon {
\tconst char *type_name;
\tconst char *icon_name;
};

""")
        asset_builders.write_asset_buffers(header, output, records)
        tables = (
            ("char *const", "EDITOR_ICONS_SOURCES", [record["symbol"] for record in records], "nullptr"),
            ("char *const", "EDITOR_ICONS_NAMES", icons_names, '""'),
            ("EditorTypeIcon", "EDITOR_TYPE_ICONS", type_entries, '{ "", "" }'),
            ("int", "EDITOR_MD_THUMBS_INDICES", icons_med, "0"),
            ("int", "EDITOR_BG_THUMBS_INDICES", icons_big, "0"),
        )
        for element_type, name, rows, empty_row in tables:
            asset_builders.write_table(header, output, element_type, name, rows, empty_row)


def _class_name_from_icon_stem(p_stem):
    pascal_case_parts = []
    for part in p_stem.split("_"):
        if re.fullmatch(r"\d+[a-z]", part):
            pascal_case_parts.append(part[:-1] + part[-1].upper())
        else:
            pascal_case_parts.append(part[:1].upper() + part[1:])
    return "".join(pascal_case_parts)
