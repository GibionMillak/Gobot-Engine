/**************************************************************************/
/*  file_access_content.cpp                                               */
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

#include "file_access_content.h"

#include "core/config/project_settings.h"
#include "core/error/error_macros.h"
#include "core/io/resource_uid.h"

HashMap<String, FileAccessContent::EmbeddedFile> FileAccessContent::_files;

bool FileAccessContent::is_content_path(const String &p_path) {
	return !_normalize_path(p_path).is_empty();
}

void FileAccessContent::register_file(const String &p_path, const uint8_t *p_data, uint64_t p_size) {
	const String path = _normalize_path(p_path);
	ERR_FAIL_COND(path.is_empty());
	ERR_FAIL_NULL(p_data);
	ERR_FAIL_COND(_files.has(path));
	_files.insert(path, { p_data, p_size });
}

void FileAccessContent::cleanup() {
	_files.clear();
}

bool FileAccessContent::has_file(const String &p_path) {
	return !_files.is_empty() && _files.has(_normalize_path(p_path));
}

String FileAccessContent::get_path() const {
	return _path;
}

String FileAccessContent::get_path_absolute() const {
	return _path;
}

bool FileAccessContent::store_buffer(const uint8_t *p_src, uint64_t p_length) {
	return false;
}

bool FileAccessContent::file_exists(const String &p_path) {
	return has_file(p_path);
}

void FileAccessContent::close() {
	FileAccessMemory::open_custom(nullptr, 0);
	_path.clear();
}

Error FileAccessContent::open_internal(const String &p_path, int p_mode_flags) {
	close();
	if (p_mode_flags != READ) {
		return ERR_UNAUTHORIZED;
	}
	const String path = _normalize_path(p_path);
	const EmbeddedFile *file = _files.getptr(path);
	if (!file) {
		return ERR_FILE_NOT_FOUND;
	}
	const Error error = FileAccessMemory::open_custom(file->data, file->size);
	if (error == OK) {
		_path = path;
	}
	return error;
}

int64_t FileAccessContent::_get_size(const String &p_path) {
	const EmbeddedFile *file = _files.getptr(_normalize_path(p_path));
	return file ? int64_t(file->size) : -1;
}

bool FileAccessContent::_get_read_only_attribute(const String &p_path) {
	return true;
}

String FileAccessContent::_normalize_path(const String &p_path) {
	if (!p_path.contains(".content") && !p_path.begins_with("uid://")) {
		return String();
	}
	String path = p_path.replace_char('\\', '/').simplify_path();
	if (path.begins_with("uid://") && ResourceUID::get_singleton()) {
		path = ResourceUID::ensure_path_nocheck(path);
	}
	if (ProjectSettings::get_singleton()) {
		path = ProjectSettings::get_singleton()->localize_path(path);
	}
	return path == "res://.content" || path.begins_with("res://.content/") ? path : String();
}
