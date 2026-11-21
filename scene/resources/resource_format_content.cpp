/**************************************************************************/
/*  resource_format_content.cpp                                           */
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

#include "resource_format_content.h"

#include "resource_format_content.gen.h"

#include "core/error/error_macros.h"
#include "core/io/file_access_content.h"
#include "core/io/image.h"
#include "core/io/image_loader.h"
#include "core/object/class_db.h"
#include "core/variant/dictionary.h"
#include "scene/resources/audio/audio_stream_wav.h"
#include "scene/resources/dpi_texture.h"
#include "scene/resources/font.h"
#include "scene/resources/image_texture.h"

#include "modules/modules_enabled.gen.h"

HashMap<String, ResourceFormatLoaderContent::AudioFormat> ResourceFormatLoaderContent::_audio_formats;

void ResourceFormatLoaderContent::register_audio_format(const String &p_extension, const String &p_type, AudioLoader p_loader) {
	ERR_FAIL_COND(p_extension.is_empty() || p_type.is_empty() || p_loader == nullptr);
	ERR_FAIL_COND(_audio_formats.has(p_extension));
	_audio_formats.insert(p_extension, AudioFormat{ p_type, p_loader });
}

void ResourceFormatLoaderContent::unregister_audio_format(const String &p_extension) {
	_audio_formats.erase(p_extension);
}

Ref<Resource> ResourceFormatLoaderContent::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode) {
	if (r_error) {
		*r_error = ERR_FILE_UNRECOGNIZED;
	}
	const String type = get_resource_type(p_path);
	ERR_FAIL_COND_V(type.is_empty(), nullptr);
	Error error = OK;
	Ref<Resource> resource;
	const AudioFormat *audio_format = _audio_formats.getptr(p_path.get_extension().to_lower());
	if (audio_format) {
		resource = audio_format->loader(p_path);
	} else if (type == "FontFile") {
		const Vector<uint8_t> data = FileAccess::get_file_as_bytes(p_path, &error);
		if (error == OK && !data.is_empty()) {
			Ref<FontFile> font;
			font.instantiate();
			font->set_data(data);
			resource = font;
		} else if (error == OK) {
			error = ERR_FILE_CORRUPT;
		}
	} else if (type == "DPITexture") {
		const Vector<uint8_t> data = FileAccess::get_file_as_bytes(p_path, &error);
		String source;
		if (error == OK && !data.is_empty()) {
			error = source.append_utf8(reinterpret_cast<const char *>(data.ptr()), data.size());
		} else if (error == OK) {
			error = ERR_FILE_CORRUPT;
		}
		if (error == OK) {
			Ref<DPITexture> texture = DPITexture::create_from_string(source);
			if (texture->get_rid().is_null()) {
				error = ERR_FILE_CORRUPT;
			} else {
				resource = texture;
			}
		}
	} else if (type == "ImageTexture") {
		Ref<Image> image;
		image.instantiate();
		error = ImageLoader::load_image(p_path, image);
		if (error == OK) {
			resource = ImageTexture::create_from_image(image);
		}
	} else if (type == "AudioStreamWAV") {
		resource = AudioStreamWAV::load_from_file(p_path, Dictionary());
	}
	if (resource.is_null() && error == OK) {
		error = ERR_FILE_CORRUPT;
	}
	if (r_error) {
		*r_error = error;
	}
	ERR_FAIL_COND_V_MSG(error != OK, nullptr, vformat("Could not load editor content '%s'.", p_path));
	if (r_progress) {
		*r_progress = 1.0f;
	}
	return resource;
}

void ResourceFormatLoaderContent::get_recognized_extensions(List<String> *r_extensions) const {
	for (const char *extension : CONTENT_FONT_EXTENSIONS) {
		r_extensions->push_back(extension);
	}
	ImageLoader::get_recognized_extensions(r_extensions);
#ifndef MODULE_SVG_ENABLED
	r_extensions->push_back("svg");
#endif
	r_extensions->push_back("wav");
	for (const KeyValue<String, AudioFormat> &format : _audio_formats) {
		r_extensions->push_back(format.key);
	}
}

void ResourceFormatLoaderContent::get_recognized_extensions_for_type(const String &p_type, List<String> *r_extensions) const {
	List<String> extensions;
	get_recognized_extensions(&extensions);
	for (const String &extension : extensions) {
		const String type = _get_type_for_extension(extension);
		if (p_type.is_empty() || ClassDB::is_parent_class(type, p_type)) {
			r_extensions->push_back(extension);
		}
	}
}

bool ResourceFormatLoaderContent::recognize_path(const String &p_path, const String &p_for_type) const {
	const String type = get_resource_type(p_path);
	return !type.is_empty() && (p_for_type.is_empty() || ClassDB::is_parent_class(type, p_for_type));
}

bool ResourceFormatLoaderContent::handles_type(const String &p_type) const {
	List<String> extensions;
	get_recognized_extensions_for_type(p_type, &extensions);
	return !extensions.is_empty();
}

String ResourceFormatLoaderContent::get_resource_type(const String &p_path) const {
	return FileAccessContent::is_content_path(p_path) ? _get_type_for_extension(p_path.get_extension().to_lower()) : String();
}

String ResourceFormatLoaderContent::_get_type_for_extension(const String &p_extension) {
	for (const char *extension : CONTENT_FONT_EXTENSIONS) {
		if (p_extension == extension) {
			return "FontFile";
		}
	}
	if (p_extension == "svg") {
		return "DPITexture";
	}
	if (ImageLoader::recognize(p_extension).is_valid()) {
		return "ImageTexture";
	}
	if (p_extension == "wav") {
		return "AudioStreamWAV";
	}
	const AudioFormat *audio_format = _audio_formats.getptr(p_extension);
	return audio_format ? audio_format->type : String();
}
