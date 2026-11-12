# Editor content

Every supported asset in the content folders is discovered recursively at editor
build time. No per-file manifest or registration step is required. Rebuild the
editor after adding, removing, or changing content.

| Directory | Source formats |
| --- | --- |
| `assets/fonts` | `.ttf`, `.ttc`, `.otf`, `.otc`, `.woff`, `.woff2`, `.pfb`, `.pfm` |
| `assets/icons` | `.svg`, `.png`, `.jpg`, `.jpeg`, `.webp`, `.hdr`, `.exr`, `.dds`, `.ktx`, `.ktx2` |
| `assets/sounds` | `.ogg`, `.mp3`, `.wav` |
| `resources` | `.tres` |

Availability depends on the loaders enabled in the engine build. Helper scripts,
C++ sources, icon metadata JSON, generated files, and documentation are excluded
from the selectable resource list. License, copyright, copying, and notice files
are embedded as supporting data.

## Paths and loading

Files retain their names, extensions, and paths below `res://.content/`. Examples:

- `res://.content/assets/icons/action/add.svg`
- `res://.content/assets/fonts/inter/regular.woff2`
- `res://.content/assets/sounds/commerce/add_to_cart.ogg`
- `res://.content/resources/styleboxes/panel.tres`

Native assets are embedded directly, without `.tres` wrappers. The runtime content
loader constructs `FontFile`, `DPITexture`, `ImageTexture`, and audio stream
resources from those files. Existing DDS/KTX loaders handle their native formats.
Vorbis and MP3 modules register their content decoders when enabled; scene and
editor theme code do not include their decoder implementation headers. The same
loaders are registered in editor builds and export templates. Serialized
`.tres` resources use the normal text resource loader.

Native resources can reference other content with absolute `res://.content/`
paths or paths relative to their own directory. The Basic theme references the
original Inter font and a rounded StyleBox.
External references must resolve to other discovered content files.

## Build data and identity

The root `SCsub` discovers source files once and invokes the builders in each
asset folder. Shared discovery, extension rules, identifier validation, and buffer
emission live in `assets/asset_builders.py`. Builders stay beside their assets.

`content_builders.py` generates `editor_content_resources.gen.h` and `.gen.cpp`:
paths, numeric UIDs, dependency-aware preview hashes, supporting dependencies,
and storage for content that does not
already have an editor asset buffer. Asset builders also generate header/source
pairs. Headers contain declarations and small constants; source files own the
buffers and tables. The public content index and editor theme share those buffers.
Do not edit generated files manually.

Editor-specific font configuration, icon rendering, and sound assignment live in
`editor/themes/editor_fonts.cpp`, `editor_icons.cpp`, and `editor_sounds.cpp`.
Resource types and selectable paths are cached after scene loaders and modules
have initialized. Preview hashes include transitive content dependencies, so
changing a referenced asset invalidates cached previews after rebuilding.

UIDs for native assets are derived from their public paths and stored in generated
`.uid` sidecars in the virtual filesystem. Source `.tres` UIDs are preserved; a
path-derived UID is inserted into embedded copies of resources without a UID.
Explicit UIDs must use valid engine UID syntax and fit its supported numeric range.
Duplicate numeric UIDs are rejected at build time. Keep public paths stable when
modifying content.

License and notice files in an asset's directory or ancestor content directories
are automatically associated with it for export.

## Browsing and editing

Enable **Editor Content** in Quick Load. Results respect the requested resource
type and support search, history, and previews. Editor content results have an
engine logo badge; addon results have a plugin badge. Hover a result to see its
source and full path. Badges share the active layout's content margins.

The item counter reports displayed results. The editor setting
`filesystem/quick_open_dialog/max_results` limits the displayed list (100 by
default); searching still considers eligible content beyond that limit.

The shipped originals and their subresources are read-only. Use **Make Unique**
to customize an assigned resource, or **Save As** to save a separate project copy.
Clearing a property only removes the reference. The reserved virtual directory
cannot be overwritten through file access or resource saving.

## Export

Exports include referenced editor content and its transitive resource, UID, and
license dependencies. Files retain their original virtual paths in the project
pack. Runtime loading does not require the editor or its embedded index.

For dynamically constructed paths, use the export preset's include filter, such
as `.content/assets/icons/action/*.svg` or `.content/*`. Unreferenced content is
otherwise omitted, including with **Export all resources**. Exclusion filters
narrow these optional include matches. Content referenced by retained project
resources or project settings is always included, even when it matches an
exclusion. Dependencies of optional resources that remain included are required
as well, so excluding a dependency does not produce a broken resource.
