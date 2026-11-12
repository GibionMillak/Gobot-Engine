"""Generate the editor theme sound table and shared asset buffers."""

import json

import asset_builders
import methods


def make_sounds(target, source, env):
    records = env["editor_asset_records"]
    with (
        methods.generated_wrapper(str(target[0])) as header,
        methods.generated_wrapper(str(target[1])) as output,
    ):
        output.write('#include "editor_sounds.gen.h"\n\n')
        header.write("""\
struct EditorSoundData {
\tconst char *name;
\tconst char *path;
};

""")
        asset_builders.write_asset_buffers(header, output, records)
        rows = [f'{{ {json.dumps(record["name"])}, {json.dumps(record["public_path"])} }}' for record in records]
        asset_builders.write_table(header, output, "EditorSoundData", "EDITOR_SOUND_DATA", rows, '{ "", "" }')
