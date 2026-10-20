"""Functions used to generate source files during build time"""

from pathlib import Path

import asset_builders


def make_sounds_header(target, source, env):
    sound_entries = []
    sound_names = set()
    sound_root = Path(env["editor_asset_root"])
    with asset_builders.generated_wrapper(target[0]) as file:
        file.write("""\
struct EditorSoundData {
	const char *name;
	const unsigned char *data;
	int size;
};

""")
        for src in map(str, source):
            name = asset_builders.get_asset_name(src, sound_root)
            if name in sound_names:
                raise ValueError(f"Duplicate built-in sound name: {name}")
            sound_names.add(name)
            buffer = asset_builders.get_asset_buffer(src)
            file.write(f"""\
inline constexpr unsigned char SOUND_{name.upper()}[] = {{
	{asset_builders.format_asset_buffer(buffer)}
}};

""")
            sound_entries.append(f'{{ "{name}", SOUND_{name.upper()}, {len(buffer)} }}')
        entries = ",\n\t".join(sound_entries)
        file.write(f"""\
inline constexpr EditorSoundData EDITOR_SOUND_DATA[] = {{
	{entries}
}};
""")
