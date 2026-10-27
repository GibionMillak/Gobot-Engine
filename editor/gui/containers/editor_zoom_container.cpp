/**************************************************************************/
/*  editor_zoom_container.cpp                                             */
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

#include "editor_zoom_container.h"

#include "core/error/error_macros.h"
#include "core/input/input.h"
#include "core/math/math_funcs.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/os/keyboard.h"
#include "core/string/translation_server.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/scene_string_names.h"

EditorZoomContainer::EditorZoomContainer() {
	{
		_zoom_minus_btn = memnew(Button);
		_zoom_minus_btn->connect(SceneStringName(pressed), callable_mp(this, &EditorZoomContainer::_button_zoom_minus));
		_zoom_minus_btn->set_focus_mode(FOCUS_NONE);
		_zoom_minus_btn->set_shortcut(ED_SHORTCUT_ARRAY("canvas_item_editor/zoom_minus", TTRC("Zoom Out"), { int32_t(KeyModifierMask::CMD_OR_CTRL | Key::MINUS), int32_t(KeyModifierMask::CMD_OR_CTRL | Key::KP_SUBTRACT) }));
		_zoom_minus_btn->set_shortcut_context(this);
		add_child(_zoom_minus_btn);

		_zoom_reset_btn = memnew(Button);
		_zoom_reset_btn->connect(SceneStringName(pressed), callable_mp(this, &EditorZoomContainer::_button_zoom_reset));
		_zoom_reset_btn->set_focus_mode(FOCUS_NONE);
		_zoom_reset_btn->set_shortcut(ED_GET_SHORTCUT("canvas_item_editor/zoom_100_percent"));
		_zoom_reset_btn->set_shortcut_context(this);
		add_child(_zoom_reset_btn);

		_zoom_plus_btn = memnew(Button);
		_zoom_plus_btn->connect(SceneStringName(pressed), callable_mp(this, &EditorZoomContainer::_button_zoom_plus));
		_zoom_plus_btn->set_focus_mode(FOCUS_NONE);
		_zoom_plus_btn->set_shortcut(ED_SHORTCUT_ARRAY("canvas_item_editor/zoom_plus", TTRC("Zoom In"), { int32_t(KeyModifierMask::CMD_OR_CTRL | Key::EQUAL), int32_t(KeyModifierMask::CMD_OR_CTRL | Key::KP_ADD) }));
		_zoom_plus_btn->set_shortcut_context(this);
		add_child(_zoom_plus_btn);
	}

	_update_zoom_label();
}

void EditorZoomContainer::set_zoom(float p_zoom) {
	const float new_zoom = CLAMP(p_zoom, _min_zoom, _max_zoom);
	if (_zoom != new_zoom) {
		_zoom = new_zoom;
		_update_zoom_label();
	}
}

float EditorZoomContainer::get_zoom() const {
	return _zoom;
}

void EditorZoomContainer::set_zoom_by_increments(int p_increment_count, bool p_integer_only) {
	// Remove editor scale from the index computation.
	const float zoom_noscale = _zoom / MAX(1, EDSCALE);

	if (p_integer_only) {
		// Only visit integer scaling factors above 100%, and fractions with an integer denominator below 100%
		// (1/2 = 50%, 1/3 = 33.33%, 1/4 = 25%, …).
		// This is useful when working on pixel art projects to avoid distortion.
		// This algorithm is designed to handle fractional start zoom values correctly
		// (e.g. 190% will zoom up to 200% and down to 100%).
		if (zoom_noscale + p_increment_count * 0.001 >= 1.0 - CMP_EPSILON) {
			// New zoom is certain to be above 100%.
			if (p_increment_count >= 1) {
				// Zooming.
				set_zoom(Math::floor(zoom_noscale + p_increment_count) * MAX(1, EDSCALE));
			} else {
				// Dezooming.
				set_zoom(Math::ceil(zoom_noscale + p_increment_count) * MAX(1, EDSCALE));
			}
		} else {
			if (p_increment_count >= 1) {
				// Zooming in. Convert the current zoom into a denominator.
				float new_zoom = 1.0 / Math::ceil(1.0 / zoom_noscale - p_increment_count);
				if (Math::is_equal_approx(zoom_noscale, new_zoom)) {
					// New zoom is identical to the old zoom, so try again.
					// This can happen due to floating-point precision issues.
					new_zoom = 1.0 / Math::ceil(1.0 / zoom_noscale - p_increment_count - 1);
				}
				set_zoom(new_zoom * MAX(1, EDSCALE));
			} else {
				// Zooming out. Convert the current zoom into a denominator.
				float new_zoom = 1.0 / Math::floor(1.0 / zoom_noscale - p_increment_count);
				if (Math::is_equal_approx(zoom_noscale, new_zoom)) {
					// New zoom is identical to the old zoom, so try again.
					// This can happen due to floating-point precision issues.
					new_zoom = 1.0 / Math::floor(1.0 / zoom_noscale - p_increment_count + 1);
				}
				set_zoom(new_zoom * MAX(1, EDSCALE));
			}
		}
	} else {
		if (_zoom < CMP_EPSILON || p_increment_count == 0) {
			return;
		}

		// Zoom is calculated as pow(zoom_factor, zoom_step).
		// This ensures the zoom will always equal 100% when zoom_step is 0.
		const float zoom_factor = EDITOR_GET("editors/2d/zoom_speed_factor");
		const float current_zoom_step = Math::round(Math::log(zoom_noscale) / Math::log(zoom_factor));
		float new_zoom = Math::pow(zoom_factor, current_zoom_step + p_increment_count);

		// Restore Editor scale transformation.
		new_zoom *= MAX(1, EDSCALE);

		set_zoom(new_zoom);
	}
}

float EditorZoomContainer::get_min_zoom() const {
	return _min_zoom;
}

float EditorZoomContainer::get_max_zoom() const {
	return _max_zoom;
}

void EditorZoomContainer::setup_zoom_limits(float p_min, float p_max) {
	ERR_FAIL_COND(p_min < 0 || p_min > p_max);

	_min_zoom = p_min;
	_max_zoom = p_max;

	if (_zoom > _max_zoom) {
		set_zoom(_max_zoom);
		emit_signal(SNAME("zoom_changed"), _zoom);
	} else if (_zoom < _min_zoom) {
		set_zoom(_min_zoom);
		emit_signal(SNAME("zoom_changed"), _zoom);
	}
}

void EditorZoomContainer::set_shortcut_context(const Node *p_node) const {
	_zoom_minus_btn->set_shortcut_context(p_node);
	_zoom_plus_btn->set_shortcut_context(p_node);
	_zoom_reset_btn->set_shortcut_context(p_node);
}

void EditorZoomContainer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_zoom", "zoom"), &EditorZoomContainer::set_zoom);
	ClassDB::bind_method(D_METHOD("get_zoom"), &EditorZoomContainer::get_zoom);
	ClassDB::bind_method(D_METHOD("set_zoom_by_increments", "increment", "integer_only"), &EditorZoomContainer::set_zoom_by_increments);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "zoom"), "set_zoom", "get_zoom");

	ADD_SIGNAL(MethodInfo("zoom_changed", PropertyInfo(Variant::FLOAT, "zoom")));
}

void EditorZoomContainer::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			_zoom_minus_btn->set_button_icon(get_editor_theme_icon(SNAME("navigation_zoom_less")));
			_zoom_plus_btn->set_button_icon(get_editor_theme_icon(SNAME("navigation_zoom_more")));
		} break;
	}
}

void EditorZoomContainer::_update_zoom_label() {
	const String lang = _get_locale();

	String zoom_text;
	// The zoom level displayed is relative to the editor scale
	// (like in most image editors). Its lower bound is clamped to 1 as some people
	// lower the editor scale to increase the available real estate,
	// even if their display doesn't have a particularly low DPI.
	if (_zoom >= 10) {
		zoom_text = TranslationServer::get_singleton()->format_number(rtos(Math::round((_zoom / MAX(1, EDSCALE)) * 100)), lang);
	} else {
		// 2 decimal places if the zoom is below 10%, 1 decimal place if it's below 1000%.
		zoom_text = TranslationServer::get_singleton()->format_number(rtos(Math::snapped((_zoom / MAX(1, EDSCALE)) * 100, (_zoom >= 0.1) ? 0.1 : 0.01)), lang);
	}
	zoom_text += " " + TranslationServer::get_singleton()->get_percent_sign(lang);
	_zoom_reset_btn->set_text(zoom_text);
}

void EditorZoomContainer::_button_zoom_by(int p_direction) {
	const bool by_integer = Input::get_singleton()->is_key_pressed(Key::ALT);
	// Integer mode steps one scaling factor at a time.
	set_zoom_by_increments(p_direction * (by_integer ? 1 : 6), by_integer);
	emit_signal(SNAME("zoom_changed"), _zoom);
}

void EditorZoomContainer::_button_zoom_minus() {
	_button_zoom_by(-1);
}

void EditorZoomContainer::_button_zoom_reset() {
	set_zoom(1.0 * MAX(1, EDSCALE));
	emit_signal(SNAME("zoom_changed"), _zoom);
}

void EditorZoomContainer::_button_zoom_plus() {
	_button_zoom_by(1);
}
