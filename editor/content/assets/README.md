# Editor assets

Fonts, icons, and sounds keep their `SCsub` and builder in their own folders.
The parent content build discovers visible source files once; each asset builder
selects its inputs from that list. Hidden folders, Python caches, and generated
files are excluded consistently. External module icon folders are discovered
separately using the same rules.

`asset_builders.py` owns supported extensions, file selection, path validation,
identifier collision checks, and generated buffer/table emission. Font extensions
also generate the runtime content loader's extension table. SCons tracks these
helpers as build dependencies.

Asset path components must use lowercase snake_case. Theme identifiers join path
components with underscores. C++ buffer symbols use uppercase names with an
`EDITOR_FONT_`, `EDITOR_ICON_`, or `EDITOR_SOUND_` prefix. Duplicate resulting
symbols are rejected with both source paths.

Each builder generates declarations in a `.gen.h` and storage in a `.gen.cpp`.
The public content index references named buffers and their declared sizes;
it does not reconstruct symbol names or assume icon array indices. SVG text
buffers use normalized LF line endings and retain a terminating null byte that
is excluded from their virtual file size.

Editor theme configuration lives in `editor/themes`. Adding supported files makes
them discoverable as public content; automatic theme use has separate rules:
SVG icons receive theme entries, OGG clips receive sound entries when Vorbis is
enabled, and editor font choices are configured in `editor_fonts.cpp`.

The root content README documents public paths, theme integration, UIDs,
read-only behavior, and export.
