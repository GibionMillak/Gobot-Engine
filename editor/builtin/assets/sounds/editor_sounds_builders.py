"""Functions used to generate source files during build time"""

import os

import methods


def make_sounds_header(target, source, env):
    sound_entries = []
    sound_names = set()
    with methods.generated_wrapper(str(target[0])) as file:
        file.write("""\
struct EditorSoundData {
	const char *name;
	const unsigned char *data;
	int size;
};

""")
        for src in map(str, source):
            category = os.path.basename(os.path.dirname(src))
            name = f"{category}_{os.path.splitext(os.path.basename(src))[0]}"
            if name in sound_names:
                raise ValueError(f"Duplicate built-in sound name: {name}")
            sound_names.add(name)
            buffer = methods.get_buffer(src)
            file.write(f"""\
inline constexpr unsigned char SOUND_{name.upper()}[] = {{
	{methods.format_buffer(buffer, 1)}
}};

""")
            sound_entries.append(f'{{ "{name}", SOUND_{name.upper()}, {len(buffer)} }}')
        entries = ",\n\t".join(sound_entries)
        file.write(f"""\
inline constexpr EditorSoundData EDITOR_SOUND_DATA[] = {{
	{entries}
}};
""")
