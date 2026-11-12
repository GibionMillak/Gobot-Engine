"""Generate the default theme's embedded font storage."""

from pathlib import Path

import asset_builders
import methods


def make_fonts(target, source, env):
    paths = [Path(str(item)).resolve() for item in source]
    records = asset_builders.get_asset_records(paths, paths[0].parent, "DEFAULT_FONT_")
    with (
        methods.generated_wrapper(str(target[0])) as header,
        methods.generated_wrapper(str(target[1])) as output,
    ):
        output.write('#include "default_font.gen.h"\n\n#ifdef BROTLI_ENABLED\n\n')
        asset_builders.write_asset_buffers(header, output, records)
        output.write("#endif\n")
