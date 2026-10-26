/**************************************************************************/
/*  renderer_method_button.cpp                                            */
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

#include "renderer_method_button.h"

#include "core/config/project_settings.h"
#include "core/error/error_macros.h"
#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "editor/editor_node.h"
#include "scene/gui/dialog.h"
#include "scene/gui/label.h"

void RendererMethodButton::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_POSTINITIALIZE: {
			// Setters can send translation notifications that require OptionButton's initialized theme cache.
			_initialize();
		} break;
		case NOTIFICATION_READY:
		case NOTIFICATION_THEME_CHANGED: {
			if (is_inside_tree()) {
				_update_icons();
			}
		} break;
		case NOTIFICATION_TRANSLATION_CHANGED: {
			_update_labels();
		} break;
	}
}

void RendererMethodButton::_initialize() {
	set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	set_fit_to_longest_item(false);
	set_focus_mode(FOCUS_NONE);
	set_tooltip_auto_translate_mode(AUTO_TRANSLATE_MODE_ALWAYS);
	set_tooltip_text(TTRC("Choose a renderer.\n\nOn mobile platforms, the Mobile renderer is used if Forward+ is selected here."));

	const String current_renderer_ps = String(GLOBAL_GET("rendering/renderer/rendering_method")).to_lower();
	const String current_renderer_os = OS::get_singleton()->get_current_rendering_method().to_lower();
	_rendering_method_overridden = current_renderer_ps != current_renderer_os;

	if (_rendering_method_overridden) {
		add_item(String());
		set_item_metadata(0, current_renderer_os);
	} else {
		connect(SceneStringName(item_selected), callable_mp(this, &RendererMethodButton::_renderer_selected));

		const PackedStringArray renderers = ProjectSettings::get_singleton()->get_custom_property_info().get(StringName("rendering/renderer/rendering_method")).hint_string.split(",", false);
		for (const String &renderer : renderers) {
			const String rendering_method = renderer.to_lower();
			if (rendering_method == "dummy") {
				continue;
			}

			add_item(String());
			const int item_index = get_item_count() - 1;
			set_item_metadata(item_index, rendering_method);
			if (current_renderer_ps == rendering_method) {
				select(item_index);
			}
		}
	}

	_update_labels();
}

void RendererMethodButton::_renderer_selected(int p_index) {
	const String rendering_method = get_item_metadata(p_index);
	const String current_renderer = GLOBAL_GET("rendering/renderer/rendering_method");
	if (rendering_method == current_renderer) {
		return;
	}

	// Keep displaying the active renderer until the restart is confirmed.
	for (int i = 0; i < get_item_count(); i++) {
		if (get_item_metadata(i) == current_renderer) {
			select(i);
			break;
		}
	}

	_pending_rendering_method = rendering_method;
	if (_restart_dialog == nullptr) {
		_restart_dialog = memnew(Dialog(true));
		_restart_dialog->connect(SceneStringName(confirmed), callable_mp(this, &RendererMethodButton::_save_and_restart));
		_restart_dialog->get_label()->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
		_restart_dialog->set_ok_button_text(TTRC("Save & Restart"));
		add_child(_restart_dialog);
	}

	const String mobile_rendering_method = rendering_method == "forward_plus" ? "mobile" : rendering_method;
	_restart_dialog->set_text(
			vformat(TTR("Changing the renderer requires restarting the editor.\n\nChoosing Save & Restart will change the renderer to:\n- Desktop platforms: %s\n- Mobile platforms: %s"),
					_to_rendering_method_display_name(rendering_method), _to_rendering_method_display_name(mobile_rendering_method)));
	_restart_dialog->popup_centered();
}

void RendererMethodButton::_update_labels() {
	for (int i = 0; i < get_item_count(); i++) {
		const String display_name = _to_rendering_method_display_name(get_item_metadata(i));
		if (_rendering_method_overridden) {
			// TRANSLATORS: The placeholder is the rendering method that has overridden the default one.
			set_item_text(i, vformat(TTR("%s (Overridden)"), display_name));
		} else {
			set_item_text(i, display_name);
		}
	}
}

void RendererMethodButton::_update_icons() {
	for (int i = 0; i < get_item_count(); i++) {
		const String rendering_method = get_item_metadata(i);
		StringName icon_name;
		if (rendering_method == "gl_compatibility") {
			icon_name = SNAME("gui_renderer_compatibility");
		} else if (rendering_method == "forward_plus") {
			icon_name = SNAME("gui_renderer_forward");
		} else if (rendering_method == "mobile") {
			icon_name = SNAME("gui_renderer_mobile");
		} else {
			continue;
		}

		set_item_icon(i, get_editor_theme_icon(icon_name));
	}
}

String RendererMethodButton::_to_rendering_method_display_name(const String &p_rendering_method) {
	if (p_rendering_method == "forward_plus") {
		return TTR("Forward+");
	}
	if (p_rendering_method == "mobile") {
		return TTR("Mobile");
	}
	if (p_rendering_method == "gl_compatibility") {
		return TTR("Compatibility");
	}
	return p_rendering_method;
}

void RendererMethodButton::_save_and_restart() {
	ProjectSettings *const project_settings = ProjectSettings::get_singleton();
	project_settings->set("rendering/renderer/rendering_method", _pending_rendering_method);

	if (_pending_rendering_method == "mobile" || _pending_rendering_method == "gl_compatibility") {
		// Keep desktop and mobile rendering methods visually consistent.
		project_settings->set("rendering/renderer/rendering_method.mobile", _pending_rendering_method);
	} else if (_pending_rendering_method == "forward_plus") {
		project_settings->set("rendering/renderer/rendering_method.mobile", "mobile");
	}

	const Error error = project_settings->save();
	ERR_FAIL_COND_MSG(error != OK, "Could not save project settings before changing the renderer.");

	EditorNode::get_singleton()->save_all_scenes();
	EditorNode::get_singleton()->restart_editor();
}
