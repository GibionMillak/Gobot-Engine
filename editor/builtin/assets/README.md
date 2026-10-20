# Built-in assets

Fonts, icons, and sounds each have a `SCsub` and a generator in their asset
folder. Shared path naming, binary reading, byte formatting, and generated-file
wrapping live in `asset_builders.py`.

Generated identifiers come from each asset's path relative to its asset root,
with path components joined by underscores. Font identifiers use snake_case;
icon and sound identifiers use snake_case. Icon module assets can add a module
prefix while using the same path naming helper.
