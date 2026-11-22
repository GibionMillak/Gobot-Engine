# Editor content

Editor content provides bundled resources for the editor and projects. Supported
files are embedded during the editor build and exposed through a virtual
filesystem, so loading them does not require access to the engine source tree.

## Adding and changing content

Place files in the appropriate directory. Discovery is recursive.

| Directory | Supported source formats |
| --- | --- |
| `assets/fonts` | `.ttf`, `.ttc`, `.otf`, `.otc`, `.woff`, `.woff2`, `.pfb`, `.pfm` |
| `assets/icons` | `.svg`, `.png`, `.jpg`, `.jpeg`, `.webp`, `.hdr`, `.exr`, `.dds`, `.ktx`, `.ktx2` |
| `assets/sounds` | `.ogg`, `.mp3`, `.wav` |
| `resources` | `.tres` |

Use lowercase snake_case for asset filenames and directory names. Keep license
and notice files alongside the assets or in an ancestor content directory; they
are associated with the content for export.

Rebuild the editor after adding, removing, or changing content. Bundling does not
require a per-file manifest. Adding a resource makes it available as public
content; its use in the editor theme is configured separately.

Availability depends on the loaders and modules enabled in the build. Hidden
folders, generated files, helper scripts, documentation, and icon metadata are
excluded from the selectable resource list. License and notice files are
embedded as supporting data.

## Paths and loading

Public paths start with `res://.content/`, followed by the file's path relative
to this directory. Names, extensions, and subdirectories are preserved. Load
resources through `ResourceLoader` in C++ or `load()` and `preload()` in GDScript.

Native fonts, textures, and sounds are embedded directly without `.tres`
wrappers. Runtime loaders construct the corresponding resource types from their
embedded data. Serialized `.tres` files use the normal text resource loader and
must use the engine's current resource format.

Resources can reference other bundled content through absolute
`res://.content/` paths or paths relative to their own directory. External
references must resolve to other discovered content files.

## Editor theme integration

Theme configuration lives in `editor/themes`. Public resource paths and theme
item names serve different purposes: a public path loads a bundled resource,
while a theme item name retrieves a resource registered in a generated theme.

All shared editor entries use the `Editor` theme type. Fonts, font sizes, icons,
styleboxes, sounds, colors, and constants remain separate Theme data categories.
Names use snake_case and must be unique within their category. Control-specific
entries use the corresponding control's theme type.

| Content | Registration and configuration |
| --- | --- |
| Fonts | `editor_fonts.cpp` configures the fonts selected for editor use. |
| SVG icons | `editor_icons.cpp` registers discovered icons and configures rendering. |
| OGG sounds | `editor_sounds.cpp` registers discovered clips when Vorbis support is enabled. |
| Styleboxes | Theme resources assign their stylebox definitions to named entries. |

Retrieve registered items with `get_font()`, `get_icon()`, `get_sound()`, or
`get_stylebox()` using their theme item name and type. Control-specific
assignments live in theme resources.
Adding fonts or styleboxes does not automatically select them for editor use;
non-SVG textures and non-OGG audio also remain public content without automatic
theme entries.

SVG icon and OGG sound identifiers come from their paths relative to their asset
folder: remove the extension and join path components with underscores. Renaming
or moving an asset changes its public path and can change its theme identifier;
update affected references and callers in the same change.

SVG icons under `node` and `resource` infer class associations by converting the
filename from snake_case to PascalCase. A companion JSON file with the same stem
can specify an explicit `type` and a `thumbnail` size of `medium` or `big`.
An optional `size_override` pair specifies a fixed display size in pixels for
the registered icon.
Variant type associations require explicit metadata. Keep interface icons outside
class icon directories, and move companion metadata together with its SVG.
The build rejects invalid or orphan metadata and duplicate identifiers or type
associations.

Module icon theme entries depend on the module being enabled. Bundled module
assets can still be public content when the corresponding resource loader is
available. External module assets outside this directory are not part of the
public content namespace.

Theme resources may be shared. Duplicate resources before applying changes that
must remain independent of other users. Editor-specific scaling, colors, font
configuration, and playback settings belong in theme registration or population
code. Theme sound streams are configured for finite, non-looping cues.

## Selecting and authoring editor themes

Editor theme definitions are standalone `.tres` resources. Each definition owns
its appearance parameters, default font, named stylebox entries, and control
assignments; it does not inherit another theme. Asset resources may be shared.

`interface/theme/theme` holds a Theme resource, using the same resource picker
as a Control's Theme property. The bundled Default theme is assigned initially
and is used when the property is empty. The picker offers bundled definitions
and can load another complete editor theme resource. Resource names are display
labels; selection uses the resource itself. System theme following temporarily
uses the bundled Dark or Light definition without replacing the assigned
resource. Selecting a resource manually disables system theme following.
Custom selections are saved as self-contained snapshots in Editor Settings, with
references to bundled content preserved. Their project resources are embedded so
the selection works in other projects and the Hub. Reload the source file to pick
up subsequent changes made outside the editor session.
System accent color is a runtime preference applied to the selected data.

Click the selected theme in Editor Settings or Project Settings to expand its
**Default** group. Open the resource in the Theme editor to access its named
items by type.

The resource supplies base and accent colors, contrast, icon saturation, padding,
border width, and border radius. StyleBox definitions supply colors and focus
outline widths for individual controls.
Each definition contains its complete control assignments, variations, palettes,
and typography. The editor prepares a copy: fonts, icons, and sounds receive
their rendering configuration, and expressions resolve against that copy.
Editor code reads font sizes, syntax colors, touchscreen preferences, and other
editor settings directly and adjusts the prepared theme's items. Platform
adjustments also apply to that copy. The source remains editable data.

Theme properties can be referenced directly in formulas. Fonts, icons,
styleboxes, and sounds are assigned to normal named Theme entries. Those
references declare the resource dependencies and work before editor preparation.
The editor configures the runtime copy's fonts, icons, and sounds under `Editor`
while preserving the source resources. Text-editor and visual-shader palette
colors also live under `Editor`, with descriptive prefixes separating them from
the general editor palette.
No theme inherits another theme or relies on a C++ control-style generator.

## Theme properties and styleboxes

Store reusable StyleBox definitions in `resources/styleboxes` and reference
them from theme resources. A shared StyleBox resolves its expressions against
the Theme using it, so each theme can supply its own defaults and named colors.

Theme defaults are grouped under `Default` in the Inspector. Their serialized
names use the `default_` prefix, which is omitted from the Inspector labels.
For example, `default_font_family` appears as Font Family and `default_padding`
appears as Padding.

Theme resources can define global padding, border width, and border radius.
Unspecified StyleBox padding, border widths, and corner radii inherit those
values. An explicit value, including zero, overrides inheritance. Author
background, border, and shadow colors and focus outline widths on the StyleBox.
Theme-wide base and accent colors, contrast, and icon saturation are available
to formulas.

Write stylebox expressions as quoted strings, such as
`padding_left = "theme.default_padding * 2 + 1"` or `border_color = "theme.default_accent_color"`.
Use `theme.Type.colors.name`, `theme.Type.constants.name`, or the corresponding
`fonts`, `font_sizes`, `icons`, `styles`, and `sounds` categories for named items.
References resolve against the prepared theme, including its runtime color
configuration. Theme resources do not declare editor-setting paths or platform
flags.
Formulas use the engine's `Expression` arithmetic, constructors, indexing, and
constant resource methods. Color references can be multiplied component by
component by `Color(r, g, b, a)`; multiplying by `Color(1, 1, 1, 0.5)` halves opacity.
Numeric literals are fixed pixels. Inherited and expression-based dimensions
use the theme's base scale once; integer dimensions are rounded.
Use `pixels(...)` for a formula that already computes pixel coordinates, such as
`pixels(round(theme.default_padding * theme.default_base_scale))`. The resource
saver preserves expressions as quoted strings. Use `condition ? when_true : when_false`
to evaluate only the selected branch.
Corner detail also accepts numeric expressions: the result scales with the theme,
is rounded, and is clamped to the native range of 1 to 20.

Loading a stylebox exposes its definition. `Theme.get_stylebox()` resolves it
against that theme and caches an independent instance when necessary. Use
`Theme.get_stylebox_definition()` to edit the source. Theme or source changes
invalidate cached instances. Controls resolve StyleBox values against their
assigned themes in nearest-first order, then the themes in the active context.
Use **Edit as Theme Expression** in a numeric property's context menu to switch
to a formula, and **Revert Value** to restore inheritance.
Invalid syntax, missing parameters, and incompatible values report errors.
Theme item expressions are resolved and cached too. Use
`Theme.get_theme_item_definition()` to preserve their source during editing or
copying; value getters return the prepared result. Cyclic references report an
error instead of recursively resolving forever.

## Build data and identity

The root `SCsub` discovers content and invokes the asset builders.
`assets/asset_builders.py` provides shared discovery, format, naming, validation,
and buffer generation. `content_builders.py` generates the public content index,
UIDs, dependency information, and preview hashes. The index and editor theme
reuse asset buffers. Do not edit generated `.gen.h` or `.gen.cpp` files manually.

Native asset UIDs are derived from public paths and exposed through generated
virtual `.uid` sidecars. Source `.tres` UIDs are preserved; embedded copies
without a UID receive a path-derived one. Explicit UIDs must be valid and unique;
the build rejects duplicate numeric UIDs. Keep public paths stable where possible.

Resource types and selectable paths are cached after loaders and modules
initialize. Preview hashes include transitive content dependencies, so rebuilding
after changing a dependency also invalidates affected previews.

## Browsing and editing

Enable **Editor Content** in Quick Load to browse bundled resources. Results
respect the requested resource type and support search, history, and previews.
Hover a result to inspect its source and full path.

Bundled originals and their subresources are read-only. Use **Make Unique** to
customize an assigned resource, or **Save As** to create a project copy. Clearing
a property removes only its reference. The reserved virtual directory cannot be
overwritten through file access or resource saving.

## Export

Every export template embeds `resources/themes/default.tres` and its
dependencies. It is the initial selection in Project Settings under
**GUI → Theme → Theme** (`gui/theme/theme`). The engine applies the project's
default scale and font settings to its copy. If another theme is selected, it
takes priority and Default fills in missing items.

Exports include referenced editor content and its transitive resource, UID, and
license dependencies. Files retain their virtual paths in the project pack.
Runtime loading does not require the editor or its embedded index, but does
require the corresponding resource loaders in the export template.

For dynamically constructed paths, use the export preset's include filter to
include the required content paths, such as `.content/*`. Unreferenced content is
otherwise omitted, including with **Export all resources**. Exclusion filters
narrow optional include matches. Content referenced by retained project resources
or project settings is always included, even when it matches an exclusion.
Dependencies of retained optional resources are required as well, so excluding
a dependency does not produce a broken resource.
