# Built-in editor sounds

These OGG Vorbis clips are embedded into the editor executable during the build.
They do not require a project import or access to this source directory at runtime.

`EditorSounds::populate_theme()` loads the catalog into the generated editor theme
under the `EditorSounds` type. Names join the folder and filename with an
underscore while preserving existing underscores: `movement/drag_start.ogg` becomes `movement_drag_start`.
All 78 clips are available, including unassigned commerce, reward, and loop clips.
Use `theme->get_sound("editing_undo", "EditorSounds")` to retrieve a catalog entry.

`ThemeClassic::populate_standard_styles()` assigns sounds to control interaction slots, after the catalog has been loaded. Default assignments cover
activation, disabled actions, selection, folding, and dragging.
Hover, focus, typing, caret movement, and continuous value changes remain
silent. A custom editor theme can override the interaction sound slots.

The catalog holds shared stream resources. Each playback creates its own playback
instance. Loop clips default to one-shot playback; a future operation using them
as loops must own its player, stop it when the operation ends, and avoid modifying
the shared catalog resource. The theme sound helper is for finite cues.

Vorbis module support is required for the built-in sounds. Builds without that
module do not register or assign the audio streams.

To add a clip, place an OGG Vorbis file in a category folder. The build generator
discovers `editor/builtin/assets/sounds/*/*.ogg`; do not edit `editor_sounds.gen.h` manually.
