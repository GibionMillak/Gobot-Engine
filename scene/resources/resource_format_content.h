/**************************************************************************/
/*  resource_format_content.h                                             */
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

#include "core/io/resource_loader.h"
#include "core/templates/hash_map.h"

// Loads native assets from the read-only editor content namespace in editors and games.
class ResourceFormatLoaderContent : public ResourceFormatLoader {
	GDSOFTCLASS(ResourceFormatLoaderContent, ResourceFormatLoader);

public:
	using AudioLoader = Ref<Resource> (*)(const String &p_path);

	// Audio modules register during scene initialization and unregister during scene shutdown.
	static void register_audio_format(const String &p_extension, const String &p_type, AudioLoader p_loader);
	static void unregister_audio_format(const String &p_extension);

	Ref<Resource> load(const String &p_path, const String &p_original_path = "", Error *r_error = nullptr, bool p_use_sub_threads = false, float *r_progress = nullptr, CacheMode p_cache_mode = CACHE_MODE_REUSE) override;
	void get_recognized_extensions(List<String> *r_extensions) const override;
	void get_recognized_extensions_for_type(const String &p_type, List<String> *r_extensions) const override;
	bool recognize_path(const String &p_path, const String &p_for_type = String()) const override;
	bool handles_type(const String &p_type) const override;
	String get_resource_type(const String &p_path) const override;

private:
	struct AudioFormat {
		String type;
		AudioLoader loader;
	};

	static String _get_type_for_extension(const String &p_extension);

	static HashMap<String, AudioFormat> _audio_formats;
};
