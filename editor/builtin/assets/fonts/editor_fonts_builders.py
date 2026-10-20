"""Functions used to generate source files during build time"""

from pathlib import Path

import asset_builders


def make_fonts_header(target, source, env):
    font_root = Path(env["editor_asset_root"])
    font_names = set()
    with asset_builders.generated_wrapper(target[0]) as file:
        for src in map(str, source):
            # Saving uncompressed, since FreeType will reference from memory pointer.
            name = asset_builders.get_asset_name(src, font_root)
            if name in font_names:
                raise ValueError(f"Duplicate built-in font name: {name}")
            font_names.add(name)
            buffer = asset_builders.get_asset_buffer(src)

            file.write(f"""\
inline constexpr int _font_{name}_size = {len(buffer)};
inline constexpr unsigned char _font_{name}[] = {{
	{asset_builders.format_asset_buffer(buffer)}
}};

""")
