"""Generate the default theme icon tables using the shared asset emitter."""

import json

import asset_builders
import methods


def make_icons(target, source, env):
    records = env["default_theme_icon_records"]
    with (
        methods.generated_wrapper(str(target[0])) as header,
        methods.generated_wrapper(str(target[1])) as output,
    ):
        output.write('#include "default_theme_icons.gen.h"\n\n')
        asset_builders.write_asset_buffers(header, output, records)
        asset_builders.write_table(
            header,
            output,
            "char *const",
            "DEFAULT_THEME_ICONS_SOURCES",
            [record["symbol"] for record in records],
            "nullptr",
        )
        asset_builders.write_table(
            header,
            output,
            "char *const",
            "DEFAULT_THEME_ICONS_NAMES",
            [json.dumps(record["name"]) for record in records],
            '""',
        )
