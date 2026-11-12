"""Discovery, validation, and buffer generation shared by editor content builders."""

import re
from pathlib import Path

import methods


FONT_EXTENSIONS = {".ttf", ".ttc", ".otf", ".otc", ".woff", ".woff2", ".pfb", ".pfm"}
TEXTURE_EXTENSIONS = {".svg", ".png", ".jpg", ".jpeg", ".webp", ".hdr", ".exr", ".dds", ".ktx", ".ktx2"}
AUDIO_EXTENSIONS = {".ogg", ".mp3", ".wav"}


def discover_files(p_root):
    """Discover visible source files in a deterministic order."""
    root = Path(p_root).resolve()
    return [
        path
        for path in sorted(root.rglob("*"))
        if path.is_file()
        and not any(part.startswith(".") or part == "__pycache__" for part in path.relative_to(root).parts)
        and ".gen." not in path.name
    ]


def select_files(p_files, p_root, p_extensions):
    root = Path(p_root).resolve()
    return [
        path
        for path in map(Path, p_files)
        if path.is_relative_to(root) and path.suffix.lower() in p_extensions
    ]


def get_asset_records(p_files, p_root, p_symbol_prefix, p_name_prefix="", p_flat_names=False):
    """Resolve identifiers once for both theme data and the public content index."""
    root = Path(p_root).resolve()
    records = []
    identifiers = {}
    for path in sorted(map(Path, p_files)):
        relative = path.resolve().relative_to(root).with_suffix("")
        if any(not re.fullmatch(r"[a-z0-9]+(?:_[a-z0-9]+)*", part) for part in relative.parts):
            raise ValueError(f"Asset paths must use lowercase snake_case: {path}")
        name = p_name_prefix + (relative.name if p_flat_names else "_".join(relative.parts))
        symbol = p_symbol_prefix + name.upper()
        if symbol in identifiers:
            raise ValueError(f"Duplicate asset identifier '{name}': {identifiers[symbol]} and {path}")
        identifiers[symbol] = path
        records.append({"path": str(path.resolve()), "name": name, "symbol": symbol})
    return records


def register_buffers(r_env, p_records, p_header):
    for record in p_records:
        path = record["path"]
        if path in r_env["editor_content_shared_buffers"]:
            raise ValueError(f"Editor asset buffer registered twice: {path}")
        r_env["editor_content_shared_buffers"][path] = {**record, "header": p_header}


def write_buffer(r_source, p_symbol, p_data, p_text=False):
    """Define storage and return its element type, array bound, and logical byte size."""
    if p_text:
        text = p_data.decode("utf-8").replace("\r\n", "\n").replace("\r", "\n")
        data = text.encode("utf-8")
        value = methods.to_raw_cstring(text)
        # Parentheses around concatenated literals are valid for pointers, not array initialization.
        if value.startswith("("):
            value = value[1:-1]
        element_type = "char"
        storage_size = len(data) + 1
    else:
        data = p_data
        value = "{\n\t" + methods.format_buffer(data or b"\0", 1) + "\n}"
        element_type = "unsigned char"
        storage_size = max(len(data), 1)
    r_source.write(f"const {element_type} {p_symbol}[{storage_size}] = {value};\n\n")
    return element_type, storage_size, len(data)


def write_asset_buffers(r_header, r_source, p_records):
    for record in p_records:
        path = Path(record["path"])
        element_type, storage_size, size = write_buffer(
            r_source,
            record["symbol"],
            path.read_bytes(),
            p_text=path.suffix.lower() == ".svg",
        )
        symbol = record["symbol"]
        r_header.write(f"inline constexpr int {symbol}_SIZE = {size};\n")
        r_header.write(f"extern const {element_type} {symbol}[{storage_size}];\n\n")


def write_table(r_header, r_source, p_type, p_name, p_rows, p_empty_row):
    r_header.write(f"inline constexpr int {p_name}_COUNT = {len(p_rows)};\n")
    r_header.write(f"extern const {p_type} {p_name}[{max(len(p_rows), 1)}];\n\n")
    r_source.write(f"const {p_type} {p_name}[{max(len(p_rows), 1)}] = {{\n\t")
    r_source.write(",\n\t".join(p_rows or [p_empty_row]) + "\n};\n\n")


def make_font_extensions_header(target, source, env):
    with methods.generated_wrapper(str(target[0])) as output:
        extensions = ", ".join(f'"{extension[1:]}"' for extension in sorted(FONT_EXTENSIONS))
        output.write(f"inline constexpr const char *CONTENT_FONT_EXTENSIONS[] = {{ {extensions} }};\n")
