/**************************************************************************/
/*  editor_toaster_container.cpp                                          */
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

#include "editor_toaster_container.h"

#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/object/message_queue.h"
#include "core/templates/local_vector.h"
#include "editor/editor_string_names.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/panel_container.h"
#include "scene/resources/style_box_flat.h"
#include "scene/scene_string_names.h"
#include "servers/display/display_server.h"
#include "servers/text/text_server.h"

EditorToasterContainer::EditorToasterContainer() {
	set_notify_transform(true);

	const Side border_side = is_layout_rtl() ? SIDE_RIGHT : SIDE_LEFT;

	_info_panel_style_background.instantiate();
	_info_panel_style_background->set_corner_radius_all(STYLEBOX_RADIUS * EDSCALE);

	_warning_panel_style_background.instantiate();
	_warning_panel_style_background->set_border_width(border_side, STYLEBOX_RADIUS * EDSCALE);
	_warning_panel_style_background->set_corner_radius_all(STYLEBOX_RADIUS * EDSCALE);

	_error_panel_style_background.instantiate();
	_error_panel_style_background->set_border_width(border_side, STYLEBOX_RADIUS * EDSCALE);
	_error_panel_style_background->set_corner_radius_all(STYLEBOX_RADIUS * EDSCALE);

	const Ref<StyleBoxFlat> backgrounds[] = { _info_panel_style_background, _warning_panel_style_background, _error_panel_style_background };
	for (const Ref<StyleBoxFlat> &background : backgrounds) {
		background->set_content_margin_individual(int(STYLEBOX_RADIUS * 2.5), 3, int(STYLEBOX_RADIUS * 2.5), 3);
	}

	_info_panel_style_progress.instantiate();
	_info_panel_style_progress->set_corner_radius_all(STYLEBOX_RADIUS * EDSCALE);

	_warning_panel_style_progress.instantiate();
	_warning_panel_style_progress->set_border_width(border_side, STYLEBOX_RADIUS * EDSCALE);
	_warning_panel_style_progress->set_corner_radius_all(STYLEBOX_RADIUS * EDSCALE);

	_error_panel_style_progress.instantiate();
	_error_panel_style_progress->set_border_width(border_side, STYLEBOX_RADIUS * EDSCALE);
	_error_panel_style_progress->set_corner_radius_all(STYLEBOX_RADIUS * EDSCALE);

	_toasts_vbox = memnew(VBoxContainer);
	_toasts_vbox->connect(SceneStringName(resized), callable_mp(this, &EditorToasterContainer::_update_vbox_position));
	_toasts_vbox->set_as_top_level(true);
	add_child(_toasts_vbox);

	_main_btn = memnew(Button);
	_main_btn->connect(SceneStringName(pressed), callable_mp(this, &EditorToasterContainer::_set_notifications_enabled).bind(true));
	_main_btn->connect(SceneStringName(pressed), callable_mp(this, &EditorToasterContainer::_repop_old));
	_main_btn->connect(SceneStringName(draw), callable_mp(this, &EditorToasterContainer::_draw_button));
	_main_btn->set_disabled(true);
	_main_btn->set_tooltip_text(TTRC("No notifications."));
	add_child(_main_btn);

	_disable_notifications_pc = memnew(PanelContainer);
	_disable_notifications_pc->add_theme_style_override(SceneStringName(panel), _info_panel_style_background);
	_disable_notifications_pc->set_as_top_level(true);
	add_child(_disable_notifications_pc);

	{
		_disable_notifications_btn = memnew(Button);
		_disable_notifications_btn->connect(SceneStringName(pressed), callable_mp(this, &EditorToasterContainer::_set_notifications_enabled).bind(false));
		_disable_notifications_btn->set_tooltip_text(TTRC("Silence the notifications."));
		_disable_notifications_pc->add_child(_disable_notifications_btn);
	}

	_singleton = this;
	_error_handler_list.errfunc = _error_handler;
	add_error_handler(&_error_handler_list);
}

EditorToasterContainer::~EditorToasterContainer() {
	_singleton = nullptr;
	remove_error_handler(&_error_handler_list);
}

EditorToasterContainer *EditorToasterContainer::get_singleton() {
	return _singleton;
}

Control *EditorToasterContainer::popup(Control *p_control, Severity p_severity, double p_time, const String &p_tooltip) {
	PanelContainer *toast_pc = memnew(PanelContainer);
	switch (p_severity) {
		case INFO:
			toast_pc->add_theme_style_override(SceneStringName(panel), _info_panel_style_background);
			break;
		case WARNING:
			toast_pc->add_theme_style_override(SceneStringName(panel), _warning_panel_style_background);
			break;
		case ERROR:
			toast_pc->add_theme_style_override(SceneStringName(panel), _error_panel_style_background);
			break;
	}
	toast_pc->connect(SceneStringName(draw), callable_mp(this, &EditorToasterContainer::_draw_progress).bind(toast_pc));
	toast_pc->connect(SceneStringName(theme_changed), callable_mp(this, &EditorToasterContainer::_toast_theme_changed).bind(toast_pc));
	toast_pc->set_modulate(Color(1, 1, 1, 0));
	toast_pc->set_tooltip_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	toast_pc->set_tooltip_text(p_tooltip);

	Toast &toast = _toasts[toast_pc];
	toast.severity = p_severity;
	toast.duration = p_time > 0.0 ? p_time : -1.0;
	toast.remaining_time = p_time > 0.0 ? p_time : 0.0;
	toast.popped = true;
	_toasts_vbox->add_child(toast_pc);

	{
		HBoxContainer *content_hbox = memnew(HBoxContainer);
		content_hbox->set_h_size_flags(SIZE_EXPAND_FILL);
		toast_pc->add_child(content_hbox);

		{
			p_control->set_h_size_flags(SIZE_EXPAND_FILL);
			content_hbox->add_child(p_control);

			if (p_time > 0.0) {
				Button *copy_btn = memnew(Button);
				copy_btn->connect(SceneStringName(pressed), callable_mp(this, &EditorToasterContainer::copy).bind(toast_pc));
				content_hbox->add_child(copy_btn);
				toast.copy_btn = copy_btn;

				Button *close_btn = memnew(Button);
				close_btn->connect(SceneStringName(pressed), callable_mp(this, &EditorToasterContainer::instant_close).bind(toast_pc));
				content_hbox->add_child(close_btn);
				toast.close_btn = close_btn;
			}
		}
	}

	_toast_theme_changed(toast_pc);
	_auto_hide_or_free_toasts();
	_update_vbox_position();
	_update_disable_notifications_button();
	_main_btn->queue_redraw();
	return toast_pc;
}

void EditorToasterContainer::popup_str(const String &p_message, Severity p_severity, const String &p_tooltip) {
	if (_is_processing_error) {
		return;
	}

	// Since "_popup_str" adds nodes to the tree, and since the "add_child" method is not
	// thread-safe, it's better to defer the call to the next cycle to be thread-safe.
	_is_processing_error = true;
	MessageQueue::get_main_singleton()->push_callable(callable_mp(this, &EditorToasterContainer::_popup_str), p_message, p_severity, p_tooltip);
	_is_processing_error = false;
}

void EditorToasterContainer::close(Control *p_control) {
	ERR_FAIL_COND(!_toasts.has(p_control));
	_toasts[p_control].remaining_time = -1.0;
	_toasts[p_control].popped = false;
}

void EditorToasterContainer::instant_close(Control *p_control) {
	close(p_control);
	p_control->set_modulate(Color(1, 1, 1, 0));
}

void EditorToasterContainer::copy(Control *p_control) {
	ERR_FAIL_COND(!_toasts.has(p_control));
	DisplayServer::get_singleton()->clipboard_set(_toasts[p_control].message);
}

void EditorToasterContainer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("push_toast", "message", "severity", "tooltip"), &EditorToasterContainer::_popup_str, DEFVAL(EditorToasterContainer::INFO), DEFVAL(String()));

	BIND_ENUM_CONSTANT(INFO);
	BIND_ENUM_CONSTANT(WARNING);
	BIND_ENUM_CONSTANT(ERROR);
}

void EditorToasterContainer::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_INTERNAL_PROCESS: {
			const double delta = get_process_delta_time();

			// Pause notification timers while a toast is hovered.
			bool hovered = false;
			for (const KeyValue<Control *, Toast> &element : _toasts) {
				if (Rect2(Vector2(), element.key->get_size()).has_point(element.key->get_local_mouse_position())) {
					hovered = true;
					break;
				}
			}

			// Expire timed notifications.
			if (!hovered) {
				for (const KeyValue<Control *, Toast> &element : _toasts) {
					if (!element.value.popped || element.value.duration <= 0) {
						continue;
					}
					_toasts[element.key].remaining_time -= delta;
					if (_toasts[element.key].remaining_time < 0) {
						close(element.key);
					}
					element.key->queue_redraw();
				}
			} else {
				// Reset the timers when hovered.
				for (const KeyValue<Control *, Toast> &element : _toasts) {
					if (!element.value.popped || element.value.duration <= 0) {
						continue;
					}
					_toasts[element.key].remaining_time = element.value.duration;
					element.key->queue_redraw();
				}
			}

			// Change alpha over time.
			bool needs_update = false;
			for (const KeyValue<Control *, Toast> &element : _toasts) {
				Color modulate_fade = element.key->get_modulate();

				// Change alpha over time.
				if (element.value.popped && modulate_fade.a < 1.0) {
					modulate_fade.a += delta * 3;
					element.key->set_modulate(modulate_fade);
				} else if (!element.value.popped && modulate_fade.a > 0.0) {
					modulate_fade.a -= delta * 2;
					element.key->set_modulate(modulate_fade);
				}

				// Hide element if it is not visible anymore.
				if (modulate_fade.a <= 0.0 && element.key->is_visible()) {
					element.key->hide();
					needs_update = true;
				} else if (modulate_fade.a > 0.0 && !element.key->is_visible()) {
					element.key->show();
					needs_update = true;
				}
			}

			if (needs_update) {
				_update_vbox_position();
				_update_disable_notifications_button();
				_main_btn->queue_redraw();
			}
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			if (_toasts_vbox->is_visible()) {
				_main_btn->set_button_icon(get_editor_theme_icon(SNAME("status_notification")));
			} else {
				_main_btn->set_button_icon(get_editor_theme_icon(SNAME("status_notification_disabled")));
			}
			_disable_notifications_btn->set_button_icon(get_editor_theme_icon(SNAME("status_notification_disabled")));

			// Styleboxes background.
			const Color base_color = get_theme_color(SNAME("base_color"), EditorStringName(Editor));
			const Color bg_color = base_color.lerp(get_theme_color(SNAME("mono_color"), EditorStringName(Editor)), 0.08);

			_info_panel_style_background->set_bg_color(bg_color);

			_warning_panel_style_background->set_bg_color(bg_color);
			_warning_panel_style_background->set_border_color(get_theme_color(SNAME("warning_color"), EditorStringName(Editor)));

			_error_panel_style_background->set_bg_color(bg_color);
			_error_panel_style_background->set_border_color(get_theme_color(SNAME("error_color"), EditorStringName(Editor)));

			// Styleboxes progress.
			const Color bg_progress_color = base_color.lerp(get_theme_color(SNAME("mono_color"), EditorStringName(Editor)), 0.135);

			_info_panel_style_progress->set_bg_color(bg_progress_color);

			_warning_panel_style_progress->set_bg_color(bg_progress_color);
			_warning_panel_style_progress->set_border_color(get_theme_color(SNAME("warning_color"), EditorStringName(Editor)));

			_error_panel_style_progress->set_bg_color(bg_progress_color);
			_error_panel_style_progress->set_border_color(get_theme_color(SNAME("error_color"), EditorStringName(Editor)));
		} break;

		case NOTIFICATION_TRANSFORM_CHANGED: {
			_update_vbox_position();
			_update_disable_notifications_button();
		} break;
	}
}

void EditorToasterContainer::_error_handler(void *p_self, const char *p_func, const char *p_file, int p_line, const char *p_error, const char *p_errorexp, bool p_editor_notify, ErrorHandlerType p_type) {
	// This may be called from a thread. Since we will deal with non-thread-safe elements,
	// we have to put it in the queue for safety.
	callable_mp_static(&EditorToasterContainer::_error_handler_impl).call_deferred(String::utf8(p_file), p_line, String::utf8(p_error), String::utf8(p_errorexp), p_editor_notify, p_type);
}

void EditorToasterContainer::_error_handler_impl(const String &p_file, int p_line, const String &p_error, const String &p_errorexp, bool p_editor_notify, int p_type) {
	if (!EditorToasterContainer::get_singleton() || !EditorToasterContainer::get_singleton()->is_inside_tree()) {
		return;
	}

#ifdef DEV_ENABLED
	const bool in_dev = true;
#else
	const bool in_dev = false;
#endif

	const int show_all_setting = EDITOR_GET("interface/editor/behavior/show_internal_errors_in_toast_notifications");

	if (p_editor_notify || (show_all_setting == 0 && in_dev) || show_all_setting == 1) {
		String err_str = !p_errorexp.is_empty() ? p_errorexp : p_error;
		const String tooltip_str = p_file + ":" + itos(p_line);

		if (!p_editor_notify) {
			if (p_type == ERR_HANDLER_WARNING) {
				err_str = "INTERNAL WARNING: " + err_str;
			} else {
				err_str = "INTERNAL ERROR: " + err_str;
			}
		}

		const Severity severity = (static_cast<ErrorHandlerType>(p_type) == ERR_HANDLER_WARNING) ? WARNING : ERROR;
		EditorToasterContainer::get_singleton()->popup_str(err_str, severity, tooltip_str);
	}
}

void EditorToasterContainer::_update_vbox_position() {
	_toasts_vbox->set_size(Vector2());

	Point2 pos = get_global_position();
	const Size2 vbox_size = _toasts_vbox->get_size();
	pos.y -= vbox_size.y + 5 * EDSCALE;
	if (!is_layout_rtl()) {
		pos.x = pos.x - vbox_size.x + get_size().x;
	}

	_toasts_vbox->set_position(pos);
}

void EditorToasterContainer::_update_disable_notifications_button() {
	bool any_visible = false;
	for (const KeyValue<Control *, Toast> &element : _toasts) {
		if (element.key->is_visible()) {
			any_visible = true;
			break;
		}
	}

	if (!any_visible || !_toasts_vbox->is_visible()) {
		_disable_notifications_pc->hide();
	} else {
		_disable_notifications_pc->show();

		Point2 pos = get_global_position();
		const int sep = 5 * EDSCALE;
		const Size2 disable_panel_size = _disable_notifications_pc->get_minimum_size();
		pos.y -= disable_panel_size.y + sep;
		if (is_layout_rtl()) {
			pos.x = pos.x - disable_panel_size.x - sep;
		} else {
			pos.x += get_size().x + sep;
		}

		_disable_notifications_pc->set_position(pos);
	}
}

void EditorToasterContainer::_auto_hide_or_free_toasts() {
	// Hide or free old temporary items.
	int visible_temporary = 0;
	int temporary = 0;
	LocalVector<Control *> to_delete;
	for (int i = _toasts_vbox->get_child_count() - 1; i >= 0; i--) {
		Control *control = Object::cast_to<Control>(_toasts_vbox->get_child(i));
		if (_toasts[control].duration <= 0) {
			continue; // Ignore non-temporary toasts.
		}

		temporary++;
		if (control->is_visible()) {
			visible_temporary++;
		}

		// Hide
		if (visible_temporary > MAX_TEMPORARY_COUNT) {
			close(control);
		}

		// Free
		if (temporary > MAX_TEMPORARY_COUNT * 2) {
			to_delete.push_back(control);
		}
	}

	// Remove expired controls before the next traversal of the toast container.
	for (Control *c : to_delete) {
		_toasts_vbox->remove_child(c);
		c->queue_free();
		_toasts.erase(c);
	}

	if (_toasts.is_empty()) {
		_main_btn->set_tooltip_text(TTRC("No notifications."));
		_main_btn->set_modulate(Color(0.5, 0.5, 0.5));
		_main_btn->set_disabled(true);
		set_process_internal(false);
	} else {
		_main_btn->set_tooltip_text(TTRC("Show notifications."));
		_main_btn->set_modulate(Color(1, 1, 1));
		_main_btn->set_disabled(false);
	}
}

void EditorToasterContainer::_draw_button() {
	bool has_one = false;
	Severity highest_severity = INFO;
	for (const KeyValue<Control *, Toast> &element : _toasts) {
		if (!element.key->is_visible()) {
			continue;
		}
		has_one = true;
		if (element.value.severity > highest_severity) {
			highest_severity = element.value.severity;
		}
	}

	if (!has_one) {
		return;
	}

	Color color;
	const real_t button_radius = _main_btn->get_size().x / 8;
	switch (highest_severity) {
		case INFO:
			color = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
			break;
		case WARNING:
			color = get_theme_color(SNAME("warning_color"), EditorStringName(Editor));
			break;
		case ERROR:
			color = get_theme_color(SNAME("error_color"), EditorStringName(Editor));
			break;
		default:
			break;
	}
	_main_btn->draw_circle(Vector2(button_radius * 2, button_radius * 2), button_radius, color);
}

void EditorToasterContainer::_draw_progress(Control *p_panel) {
	if (_toasts.has(p_panel) && _toasts[p_panel].remaining_time > 0 && _toasts[p_panel].duration > 0) {
		Ref<StyleBoxFlat> stylebox;
		switch (_toasts[p_panel].severity) {
			case INFO:
				stylebox = _info_panel_style_progress;
				break;
			case WARNING:
				stylebox = _warning_panel_style_progress;
				break;
			case ERROR:
				stylebox = _error_panel_style_progress;
				break;
			default:
				break;
		}

		const Size2 size = p_panel->get_size();
		Size2 progress = size;
		progress.width *= MIN(1, Math::remap(_toasts[p_panel].remaining_time, 0, _toasts[p_panel].duration, 0, 2));
		if (is_layout_rtl()) {
			p_panel->draw_style_box(stylebox, Rect2(size - progress, progress));
		} else {
			p_panel->draw_style_box(stylebox, Rect2(Vector2(), progress));
		}
	}
}

void EditorToasterContainer::_set_notifications_enabled(bool p_enabled) {
	_toasts_vbox->set_visible(p_enabled);
	if (p_enabled) {
		_main_btn->set_button_icon(get_editor_theme_icon(SNAME("status_notification")));
	} else {
		_main_btn->set_button_icon(get_editor_theme_icon(SNAME("status_notification_disabled")));
	}
	_update_disable_notifications_button();
}

void EditorToasterContainer::_repop_old() {
	// Restore the most recent notifications, up to MAX_TEMPORARY_COUNT.
	bool needs_update = false;
	int visible_count = 0;
	for (int i = _toasts_vbox->get_child_count() - 1; i >= 0; i--) {
		Control *control = Object::cast_to<Control>(_toasts_vbox->get_child(i));
		if (!control->is_visible()) {
			control->show();
			_toasts[control].remaining_time = _toasts[control].duration;
			_toasts[control].popped = true;
			needs_update = true;
		}
		visible_count++;
		if (visible_count >= MAX_TEMPORARY_COUNT) {
			break;
		}
	}
	if (needs_update) {
		_update_vbox_position();
		_update_disable_notifications_button();
		_main_btn->queue_redraw();
	}
}

void EditorToasterContainer::_popup_str(const String &p_message, Severity p_severity, const String &p_tooltip) {
	_is_processing_error = true;
	// Check if we already have a popup with the given message.
	Control *control = nullptr;
	for (const KeyValue<Control *, Toast> &element : _toasts) {
		if (element.value.message == p_message && element.value.severity == p_severity && element.value.tooltip == p_tooltip) {
			control = element.key;
			break;
		}
	}

	// Create a new message if needed.
	if (control == nullptr) {
		HBoxContainer *message_hbox = memnew(HBoxContainer);
		message_hbox->add_theme_constant_override("separation", 0);
		control = popup(message_hbox, p_severity, DEFAULT_MESSAGE_DURATION, p_tooltip);

		{
			Label *message_lbl = memnew(Label);
			message_lbl->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
			message_hbox->add_child(message_lbl);

			Label *count_lbl = memnew(Label);
			count_lbl->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
			message_hbox->add_child(count_lbl);

			Toast &toast = _toasts[control];
			toast.message = p_message;
			toast.tooltip = p_tooltip;
			toast.count = 1;
			toast.message_lbl = message_lbl;
			toast.message_count_lbl = count_lbl;
		}
	} else {
		Toast &toast = _toasts[control];
		if (toast.popped) {
			toast.count += 1;
		} else {
			toast.count = 1;
		}
		toast.remaining_time = toast.duration;
		toast.popped = true;
		control->show();
		_toasts_vbox->move_child(control, _toasts_vbox->get_child_count());
		_auto_hide_or_free_toasts();
		_update_vbox_position();
		_update_disable_notifications_button();
		_main_btn->queue_redraw();
	}

	// Retrieve the label back, then update the text.
	Label *message_lbl = _toasts[control].message_lbl;
	ERR_FAIL_NULL(message_lbl);
	message_lbl->set_text(p_message);
	message_lbl->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
	message_lbl->set_custom_minimum_size(Size2());

	const Size2i size = message_lbl->get_combined_minimum_size();
	const int limit_width = get_viewport_rect().size.x / 2; // Limit label size to half the viewport size.
	if (size.x > limit_width) {
		message_lbl->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
		message_lbl->set_custom_minimum_size(Size2(limit_width, 0));
	}

	// Retrieve the count label back, then update the text.
	Label *message_count_lbl = _toasts[control].message_count_lbl;
	if (_toasts[control].count == 1) {
		message_count_lbl->hide();
	} else {
		message_count_lbl->set_text(vformat("(%d)", _toasts[control].count));
		message_count_lbl->show();
	}

	_toasts_vbox->reset_size();

	_is_processing_error = false;
	set_process_internal(true);
}

void EditorToasterContainer::_toast_theme_changed(Control *p_control) {
	ERR_FAIL_COND(!_toasts.has(p_control));

	Toast &toast = _toasts[p_control];
	if (toast.close_btn) {
		toast.close_btn->set_button_icon(get_editor_theme_icon(SNAME("action_close")));
	}
	if (toast.copy_btn) {
		toast.copy_btn->set_button_icon(get_editor_theme_icon(SNAME("action_copy")));
	}
}
