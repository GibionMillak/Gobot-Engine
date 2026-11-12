/**************************************************************************/
/*  file_access_content.h                                                 */
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

#pragma once

#include "core/io/file_access_memory.h"
#include "core/templates/hash_map.h"

// Embedded buffers must outlive every file opened from them.
class FileAccessContent : public FileAccessMemory {
	GDSOFTCLASS(FileAccessContent, FileAccessMemory);

public:
	static bool is_content_path(const String &p_path);
	static void register_file(const String &p_path, const uint8_t *p_data, uint64_t p_size);
	static void cleanup();
	static bool has_file(const String &p_path);

	String get_path() const override;
	String get_path_absolute() const override;
	bool store_buffer(const uint8_t *p_src, uint64_t p_length) override;
	bool file_exists(const String &p_path) override;
	void close() override;

protected:
	Error open_internal(const String &p_path, int p_mode_flags) override;
	int64_t _get_size(const String &p_path) override;
	bool _get_read_only_attribute(const String &p_path) override;

private:
	struct EmbeddedFile {
		const uint8_t *data = nullptr;
		uint64_t size = 0;
	};

	static String _normalize_path(const String &p_path);

	static HashMap<String, EmbeddedFile> _files;
	String _path;
};
