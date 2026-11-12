"""Discover and embed editor content using its original filenames and formats."""

import hashlib
import json
import posixpath
import re
from pathlib import Path

import asset_builders
import methods


def discover_content(p_files, p_root):
    root = Path(p_root).resolve()
    formats = {
        "assets/fonts/": asset_builders.FONT_EXTENSIONS,
        "assets/icons/": asset_builders.TEXTURE_EXTENSIONS,
        "assets/sounds/": asset_builders.AUDIO_EXTENSIONS,
        "resources/": {".tres"},
    }
    resources = []
    support = []
    for path in p_files:
        relative = path.relative_to(root).as_posix()
        if any(
            relative.startswith(directory) and path.suffix.lower() in extensions
            for directory, extensions in formats.items()
        ):
            resources.append(path)
        elif path.name.upper().startswith(("LICENSE", "COPYING", "COPYRIGHT", "NOTICE")):
            support.append(path)
    return resources, support


UID_ALPHABET = "abcdefghijklmnopqrstuvwxy012345678"
UID_MASK = 0x7FFFFFFFFFFFFFFF


def _resource_uid(p_path):
    value = int.from_bytes(hashlib.sha256(p_path.encode("utf-8")).digest()[:8], "little") & UID_MASK
    text = ""
    while value or not text:
        text = UID_ALPHABET[value % len(UID_ALPHABET)] + text
        value //= len(UID_ALPHABET)
    return "uid://" + text


def _uid_id(p_uid):
    if not re.fullmatch(r"uid://[a-y0-8]{1,13}", p_uid):
        raise ValueError(f"Invalid editor content UID: {p_uid}")
    value = 0
    for character in p_uid[6:]:
        value = value * len(UID_ALPHABET) + UID_ALPHABET.index(character)
    if value > UID_MASK:
        raise ValueError(f"Editor content UID exceeds the supported range: {p_uid}")
    return value


def make_content_resources(target, source, env):
    root = Path(env["editor_content_root"])
    resources = list(map(Path, env["editor_content_resources"]))
    support = list(map(Path, env["editor_content_support"]))
    format_header = Path(env["resource_text_format_header"]).read_text(encoding="utf-8")
    format_match = re.search(r"\bFORMAT_VERSION\s*=\s*(\d+)", format_header)
    if not format_match:
        raise ValueError("Could not determine the engine's text resource format version")
    format_version = int(format_match.group(1))
    shared_buffers = env["editor_content_shared_buffers"]
    entries = {}
    seen_uids = {}
    includes = set()
    licenses = {}
    for path in support:
        public_path = "res://.content/" + path.relative_to(root).as_posix()
        licenses.setdefault(path.parent, []).append(public_path)
        entries[public_path] = (-1, path.read_bytes(), None, [])
    for path in resources:
        public_path = "res://.content/" + path.relative_to(root).as_posix()
        shared = shared_buffers.get(str(path))
        data = None if shared else path.read_bytes()
        uid = _resource_uid(public_path)
        dependencies = []
        if path.suffix.lower() == ".tres":
            text = data.decode("utf-8")
            header = re.match(r'\[gd_resource\b[^\n]*\]', text)
            if not header:
                raise ValueError(f"Invalid editor content resource header: {path}")
            source_format = re.search(r"\bformat\s*=\s*(\d+)", header.group(0))
            if not source_format or int(source_format.group(1)) != format_version:
                raise ValueError(f"Expected resource format {format_version}: {path}")
            source_uid = re.search(r'\buid="([^"\n]*)"', header.group(0))
            if source_uid:
                uid = source_uid.group(1)
            else:
                text = text[: header.end() - 1] + f' uid="{uid}"' + text[header.end() - 1 :]
                data = text.encode("utf-8")
        else:
            uid_path = public_path + ".uid"
            entries[uid_path] = (-1, (uid + "\n").encode("ascii"), None, [])
            dependencies.append(uid_path)
        try:
            uid_id = _uid_id(uid)
        except ValueError as error:
            raise ValueError(f"{path}: {error}") from error
        if uid_id in seen_uids:
            raise ValueError(f"Duplicate editor content UID '{uid}': {seen_uids[uid_id]} and {path}")
        seen_uids[uid_id] = path
        directory = path.parent
        while directory.is_relative_to(root):
            dependencies.extend(licenses.get(directory, []))
            directory = directory.parent
        if shared:
            includes.add(shared["header"])
        entries[public_path] = (uid_id, data, shared, dependencies)
    uid_paths = {uid: public_path for public_path, (uid, _, _, _) in entries.items() if uid >= 0}
    resource_dependencies = {}
    for public_path, (uid, data, _, _) in entries.items():
        if uid >= 0 and public_path.lower().endswith(".tres"):
            for dependency in re.findall(r'\[ext_resource\b[^\n]*\bpath="([^"]+)"', data.decode("utf-8")):
                if "://" not in dependency:
                    parent = public_path.removeprefix("res://").rsplit("/", 1)[0]
                    dependency = "res://" + posixpath.normpath(posixpath.join(parent, dependency))
                if dependency.startswith("uid://"):
                    dependency = uid_paths.get(_uid_id(dependency), dependency)
                if dependency not in entries:
                    raise ValueError(f"Unknown dependency '{dependency}' in '{public_path}'")
                resource_dependencies.setdefault(public_path, []).append(dependency)
    file_hashes = {
        path: hashlib.sha256(data if data is not None else Path(shared["path"]).read_bytes()).digest()
        for path, (_, data, shared, _) in entries.items()
    }
    preview_hashes = {}
    for path, (uid, _, _, _) in entries.items():
        if uid < 0:
            continue
        visited = set()
        pending = [path]
        while pending:
            dependency = pending.pop()
            if dependency in visited:
                continue
            visited.add(dependency)
            pending.extend(entries[dependency][3])
            pending.extend(resource_dependencies.get(dependency, []))
        digest = hashlib.sha256()
        for dependency in sorted(visited):
            digest.update(dependency.encode("utf-8") + b"\0" + file_hashes[dependency])
        preview_hashes[path] = digest.hexdigest()
    with (
        methods.generated_wrapper(str(target[0])) as header,
        methods.generated_wrapper(str(target[1])) as output,
    ):
        header.write('#include "core/io/resource_uid.h"\n\n')
        header.write("""\
struct EditorContentResourceData {
\tconst char *path;
\tResourceUID::ID uid;
\tconst char *preview_hash;
\tconst unsigned char *data;
\tuint64_t size;
\tconst char *const *dependencies;
\tint dependency_count;
};

""")
        output.write('#include "editor_content_resources.gen.h"\n\n')
        for include in sorted(includes):
            output.write(f'#include "{include}"\n')
        output.write("\n")
        rows = []
        for index, (public_path, (uid, data, shared, dependencies)) in enumerate(sorted(entries.items())):
            if shared:
                buffer_name = shared["symbol"]
                size = buffer_name + "_SIZE"
                if public_path.lower().endswith(".svg"):
                    buffer_name = f"reinterpret_cast<const unsigned char *>({buffer_name})"
            else:
                buffer_name = f"EDITOR_CONTENT_BUFFER_{index}"
                asset_builders.write_buffer(output, buffer_name, data)
                size = str(len(data))
            dependency_name = "nullptr"
            if dependencies:
                dependency_name = f"EDITOR_CONTENT_DEPENDENCIES_{index}"
                output.write(f"static constexpr const char *{dependency_name}[] = {{\n\t")
                output.write(",\n\t".join(json.dumps(item) for item in dependencies) + "\n};\n\n")
            rows.append(
                f'{{ {json.dumps(public_path)}, {uid}, "{preview_hashes.get(public_path, "")}", '
                f"{buffer_name}, {size}, {dependency_name}, {len(dependencies)} }}"
            )
        asset_builders.write_table(
            header,
            output,
            "EditorContentResourceData",
            "EDITOR_CONTENT_RESOURCE_DATA",
            rows,
            '{ "", -1, "", nullptr, 0, nullptr, 0 }',
        )
