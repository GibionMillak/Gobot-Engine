/**************************************************************************/
/*  editor_fullscreen_button.cpp                                          */
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

#include "editor_fullscreen_button.h"

#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "servers/display/display_server.h"

EditorFullscreenButton::EditorFullscreenButton() {
	set_focus_mode(FOCUS_NONE);
	set_shortcut(ED_SHORTCUT_AND_COMMAND("editor/fullscreen_mode", TTRC("Toggle Fullscreen"), KeyModifierMask::SHIFT | Key::F11));
	ED_SHORTCUT_OVERRIDE("editor/fullscreen_mode", "macos", KeyModifierMask::META | KeyModifierMask::CTRL | Key::F);
	set_toggle_mode(true);
	set_tooltip_auto_translate_mode(AUTO_TRANSLATE_MODE_ALWAYS);
}

void EditorFullscreenButton::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
#ifdef ANDROID_ENABLED
			if (bool(EDITOR_DEF("_is_editor_fullscreen", false))) {
				const DisplayServerEnums::WindowMode mode = DisplayServer::get_singleton()->window_get_mode();
				if (mode == DisplayServerEnums::WINDOW_MODE_WINDOWED || mode == DisplayServerEnums::WINDOW_MODE_MAXIMIZED) {
					_previous_mode = mode;
				}
				DisplayServer::get_singleton()->window_set_mode(DisplayServerEnums::WINDOW_MODE_FULLSCREEN);
			}
#endif
			_sync_state();
			set_process_internal(true);
		} break;
		case NOTIFICATION_INTERNAL_PROCESS: {
			// Native window controls and other callers can change fullscreen without pressing this button.
			_sync_state();
		} break;
		case NOTIFICATION_THEME_CHANGED:
		case NOTIFICATION_TRANSLATION_CHANGED: {
			if (is_inside_tree()) {
				_update_appearance();
			}
		} break;
	}
}

void EditorFullscreenButton::pressed() {
	_sync_state();
	const bool fullscreen = _mode == DisplayServerEnums::WINDOW_MODE_FULLSCREEN || _mode == DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN;
	DisplayServer::get_singleton()->window_set_mode(fullscreen ? _previous_mode : DisplayServerEnums::WINDOW_MODE_FULLSCREEN);
	_sync_state();
}

void EditorFullscreenButton::_sync_state() {
	const DisplayServerEnums::WindowMode mode = DisplayServer::get_singleton()->window_get_mode();
	const bool fullscreen = mode == DisplayServerEnums::WINDOW_MODE_FULLSCREEN || mode == DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN;
	set_pressed_no_signal(fullscreen);
	if (_state_initialized && mode == _mode) {
		return;
	}

	_mode = mode;
	_state_initialized = true;
	if (_mode == DisplayServerEnums::WINDOW_MODE_WINDOWED || _mode == DisplayServerEnums::WINDOW_MODE_MAXIMIZED) {
		_previous_mode = _mode;
	}

	_update_appearance();
#ifdef ANDROID_ENABLED
	if (bool(EDITOR_GET("_is_editor_fullscreen")) != fullscreen) {
		EditorSettings::get_singleton()->set("_is_editor_fullscreen", fullscreen);
		EditorSettings::get_singleton()->save();
	}
#endif
}

void EditorFullscreenButton::_update_appearance() {
	const bool fullscreen = _mode == DisplayServerEnums::WINDOW_MODE_FULLSCREEN || _mode == DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN;
	set_button_icon(get_editor_theme_icon(fullscreen ? SNAME("editor_window_restore") : SNAME("view_expand")));
	set_tooltip_text(fullscreen ? TTRC("Exit Fullscreen") : TTRC("Enter Fullscreen"));
}
