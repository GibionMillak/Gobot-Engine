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
#include "editor/themes/editor_theme.h"

#include "modules/modules_enabled.gen.h"

#ifdef MODULE_VORBIS_ENABLED
#include "editor_sounds.gen.h"

#include "core/templates/vector.h"

#include "modules/vorbis/audio_stream_ogg_vorbis.h"

#include <cstring>
#endif

void EditorSounds::populate_theme(const Ref<EditorTheme> &p_theme) {
#ifdef MODULE_VORBIS_ENABLED
	for (const EditorSoundData &sound_data : EDITOR_SOUND_DATA) {
		Vector<uint8_t> buffer;
		const Error error = buffer.resize(sound_data.size);
		ERR_CONTINUE_MSG(error != OK, vformat("Could not allocate built-in editor sound '%s'.", sound_data.name));
		memcpy(buffer.ptrw(), sound_data.data, sound_data.size);
		Ref<AudioStreamOggVorbis> sound = AudioStreamOggVorbis::load_from_buffer(buffer);
		ERR_CONTINUE_MSG(sound.is_null(), vformat("Could not load built-in editor sound '%s'.", sound_data.name));
		// Operations that need loops must own their playback and stopping.
		sound->set_loop(false);
		sound->set_name(sound_data.name);
		p_theme->set_sound(sound_data.name, "EditorSounds", sound);
	}
#endif
}
