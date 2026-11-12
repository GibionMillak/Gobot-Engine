/**************************************************************************/
/*  editor_sounds.cpp                                                      */
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

#include "editor_sounds.h"

#include "core/error/error_macros.h"
#include "core/io/resource_loader.h"
#include "editor/themes/editor_theme.h"
#include "scene/resources/audio/audio_stream.h"

#include "modules/modules_enabled.gen.h"

#ifdef MODULE_VORBIS_ENABLED
#include "editor/content/assets/sounds/editor_sounds.gen.h"
#endif

void EditorSounds::populate_theme(const Ref<EditorTheme> &p_theme) {
#ifdef MODULE_VORBIS_ENABLED
	for (int i = 0; i < EDITOR_SOUND_DATA_COUNT; i++) {
		const EditorSoundData &sound_data = EDITOR_SOUND_DATA[i];
		Ref<AudioStream> sound = ResourceLoader::load(sound_data.path, "AudioStream", ResourceLoader::CACHE_MODE_IGNORE);
		ERR_CONTINUE_MSG(sound.is_null(), vformat("Could not load bundled editor sound '%s'.", sound_data.name));
		// Keep theme playback settings independent of project resource instances.
		bool valid = false;
		sound->set("loop", false, &valid);
		ERR_CONTINUE_MSG(!valid, vformat("Bundled editor sound '%s' does not support disabling loops.", sound_data.name));
		sound->set_name(sound_data.name);
		p_theme->set_sound(sound_data.name, "EditorSounds", sound);
	}
#endif
}
