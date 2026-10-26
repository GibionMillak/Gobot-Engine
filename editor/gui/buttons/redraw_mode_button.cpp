/**************************************************************************/
/*  redraw_mode_button.cpp                                                */
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

#include "redraw_mode_button.h"

#include "core/config/engine.h"
#include "core/error/error_macros.h"
#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "editor/editor_string_names.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_theme_manager.h"
#include "scene/gui/popup_menu.h"
#include "scene/scene_string_names.h"

RedrawModeButton::RedrawModeButton() {
	set_focus_mode(FOCUS_NONE);
	set_tooltip_auto_translate_mode(AUTO_TRANSLATE_MODE_ALWAYS);

	PopupMenu *const mode_popup = get_popup();
	mode_popup->add_radio_check_item(TTRC("Reactive"), REACTIVE);
	mode_popup->add_radio_check_item(TTRC("Continuously"), CONTINUOUSLY);
	mode_popup->connect(SceneStringName(id_pressed), callable_mp(this, &RedrawModeButton::_mode_selected));
}

void RedrawModeButton::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			_last_step_msec = OS::get_singleton()->get_ticks_msec();
			_last_step_frame = Engine::get_singleton()->get_frames_drawn();
			_update_mode();
			_update_icon();
			set_process(true);
		} break;
		case NOTIFICATION_THEME_CHANGED: {
			if (is_inside_tree()) {
				_update_appearance();
				_update_icon();
			}
		} break;
		case NOTIFICATION_PROCESS: {
			const uint64_t frame = Engine::get_singleton()->get_frames_drawn();
			const uint64_t tick = OS::get_singleton()->get_ticks_msec();
			if (frame != _last_step_frame && tick - _last_step_msec > 1000 / 8) {
				_animation_frame = (_animation_frame + 1) % 8;
				_last_step_msec = tick;
				// Ignore the redraw caused by advancing this button's own icon.
				_last_step_frame = frame + 1;
				_update_icon();
			}
		} break;
		case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
			if (EditorSettings::get_singleton()->check_changed_settings_in_group("interface/editor/display")) {
				_update_mode();
			}
		} break;
	}
}

void RedrawModeButton::_mode_selected(int p_id) {
	ERR_FAIL_COND(p_id != REACTIVE && p_id != CONTINUOUSLY);
	EditorSettings::get_singleton()->set("interface/editor/display/redraw_mode", p_id);
	_update_mode();
}

void RedrawModeButton::_update_mode() {
	const int redraw_mode = EDITOR_GET("interface/editor/display/redraw_mode");
	ERR_FAIL_COND_MSG(redraw_mode != REACTIVE && redraw_mode != CONTINUOUSLY, vformat("Invalid editor redraw mode: %d.", redraw_mode));
	_redraw_mode = static_cast<RedrawMode>(redraw_mode);
	OS::get_singleton()->set_low_processor_usage_mode(_redraw_mode == REACTIVE);

	PopupMenu *const mode_popup = get_popup();
	mode_popup->set_item_checked(mode_popup->get_item_index(REACTIVE), _redraw_mode == REACTIVE);
	mode_popup->set_item_checked(mode_popup->get_item_index(CONTINUOUSLY), _redraw_mode == CONTINUOUSLY);
	_update_appearance();
}

void RedrawModeButton::_update_appearance() {
	if (_redraw_mode == CONTINUOUSLY) {
		set_tooltip_text(TTRC("Spins when the editor window redraws.\nRedraw mode: Continuously (every frame)."));
		// Compensate for the icon's base color when tinting it in dark and light themes.
		const bool dark_icon_and_font = EditorThemeManager::is_dark_icon_and_font();
		set_self_modulate(get_theme_color(SNAME("error_color"), EditorStringName(Editor)) * (dark_icon_and_font ? Color(1.1, 1.1, 1.1) : Color(4.25, 4.25, 4.25)));
	} else {
		set_tooltip_text(TTRC("Spins when the editor window redraws.\nRedraw mode: Reactive (when needed)."));
		set_self_modulate(Color(1, 1, 1));
	}
}

void RedrawModeButton::_update_icon() {
	set_button_icon(get_editor_theme_icon("status_progress" + itos(_animation_frame + 1)));
}
