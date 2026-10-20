"""Shared helpers for generating built-in asset data."""

from pathlib import Path

import methods


def get_asset_name(source, root, prefix=""):
    """Build an asset identifier from its path relative to its asset root."""
    relative_path = Path(str(source)).resolve().relative_to(Path(root).resolve()).with_suffix("")
    return prefix + "_".join(relative_path.parts)


def get_asset_buffer(source):
    return methods.get_buffer(str(source))


def format_asset_buffer(buffer):
    return methods.format_buffer(buffer, 1)


def generated_wrapper(target):
    return methods.generated_wrapper(str(target))
