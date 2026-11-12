"""Generate declarations and shared storage for bundled fonts."""

import asset_builders
import methods


def make_fonts(target, source, env):
    with (
        methods.generated_wrapper(str(target[0])) as header,
        methods.generated_wrapper(str(target[1])) as output,
    ):
        output.write('#include "editor_fonts.gen.h"\n\n')
        asset_builders.write_asset_buffers(header, output, env["editor_asset_records"])
