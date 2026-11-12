/**************************************************************************/
/*  editor_content_resources.cpp                                          */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "editor_content_resources.h"

#include "editor_content_resources.gen.h"

#include "core/error/error_macros.h"
#include "core/io/file_access_content.h"
#include "core/io/resource_loader.h"

HashMap<String, int> EditorContentResources::_path_indices;
Vector<String> EditorContentResources::_paths;
Vector<StringName> EditorContentResources::_resource_types;

void EditorContentResources::initialize() {
	for (int i = 0; i < EDITOR_CONTENT_RESOURCE_DATA_COUNT; i++) {
		const EditorContentResourceData &entry = EDITOR_CONTENT_RESOURCE_DATA[i];
		FileAccessContent::register_file(entry.path, entry.data, entry.size);
		_path_indices.insert(entry.path, i);
	}
	register_uids();
	_initialize_metadata();
}

void EditorContentResources::finalize() {
	FileAccessContent::cleanup();
	_path_indices.clear();
	_paths.clear();
	_resource_types.clear();
}

void EditorContentResources::register_uids() {
	ResourceUID *resource_uid = ResourceUID::get_singleton();
	for (int i = 0; i < EDITOR_CONTENT_RESOURCE_DATA_COUNT; i++) {
		const EditorContentResourceData &entry = EDITOR_CONTENT_RESOURCE_DATA[i];
		if (entry.uid == ResourceUID::INVALID_ID) {
			continue;
		}
		const ResourceUID::ID uid = entry.uid;
		if (resource_uid->has_id(uid)) {
			ERR_CONTINUE_MSG(resource_uid->get_id_path(uid) != entry.path, vformat("Editor content resource UID collision for '%s'.", entry.path));
		} else {
			resource_uid->add_id(uid, entry.path);
		}
	}
}

const Vector<String> &EditorContentResources::get_paths() {
	return _paths;
}

StringName EditorContentResources::get_resource_type(const String &p_path) {
	const int index = _find_path(p_path);
	if (index < 0) {
		return StringName();
	}
	return _resource_types[index];
}

ResourceUID::ID EditorContentResources::get_resource_uid(const String &p_path) {
	const int index = _find_path(p_path);
	if (index < 0) {
		return ResourceUID::INVALID_ID;
	}
	return EDITOR_CONTENT_RESOURCE_DATA[index].uid;
}

String EditorContentResources::get_preview_hash(const String &p_path) {
	const int index = _find_path(p_path);
	return index >= 0 ? String(EDITOR_CONTENT_RESOURCE_DATA[index].preview_hash) : String();
}

Vector<String> EditorContentResources::get_extra_dependencies(const String &p_path) {
	Vector<String> dependencies;
	const int index = _find_path(p_path);
	if (index >= 0) {
		const EditorContentResourceData &entry = EDITOR_CONTENT_RESOURCE_DATA[index];
		for (int i = 0; i < entry.dependency_count; i++) {
			dependencies.push_back(entry.dependencies[i]);
		}
	}
	return dependencies;
}

void EditorContentResources::_initialize_metadata() {
	_resource_types.resize(EDITOR_CONTENT_RESOURCE_DATA_COUNT);
	for (int i = 0; i < EDITOR_CONTENT_RESOURCE_DATA_COUNT; i++) {
		const EditorContentResourceData &entry = EDITOR_CONTENT_RESOURCE_DATA[i];
		if (entry.uid == ResourceUID::INVALID_ID) {
			continue;
		}
		const StringName type = ResourceLoader::get_resource_type(entry.path);
		_resource_types.write[i] = type;
		if (type != StringName()) {
			_paths.push_back(entry.path);
		}
	}
}

int EditorContentResources::_find_path(const String &p_path) {
	const int *index = _path_indices.getptr(p_path);
	return index ? *index : -1;
}
