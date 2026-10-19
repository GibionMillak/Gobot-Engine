/**************************************************************************/
/*  dialog.cpp                                                            */
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

#include "dialog.h"

#include "core/config/engine.h"
#include "core/error/error_macros.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "scene/gui/line_edit.h"
#include "scene/scene_string_names.h"
#include "scene/theme/theme_db.h"

Dialog::Dialog(bool p_show_cancel) {
	set_clamp_to_embedder(true);
	set_exclusive(true);
	set_flag(FLAG_MAXIMIZE_DISABLED, true);
	set_flag(FLAG_MINIMIZE_DISABLED, true);
	set_keep_title_visible(true);
	set_transient(true);
	set_visible(false);
	set_wrap_controls(true);

	_bg_panel = memnew(Panel);
	add_child(_bg_panel, false, INTERNAL_MODE_FRONT);

	_message_lbl = memnew(Label);
	_message_lbl->set_anchor(SIDE_BOTTOM, Control::ANCHOR_END);
	_message_lbl->set_anchor(SIDE_RIGHT, Control::ANCHOR_END);
	add_child(_message_lbl, false, INTERNAL_MODE_FRONT);

	_buttons_hbox = memnew(HBoxContainer);
	_buttons_hbox->set_layout_direction(Control::LAYOUT_DIRECTION_LTR);
	add_child(_buttons_hbox, false, INTERNAL_MODE_BACK);
	{
		_buttons_hbox->add_spacer();
		Control *bound_spacer = _buttons_hbox->add_spacer();
		_ok_btn = memnew(Button);
		_ok_btn->connect(SceneStringName(pressed), callable_mp(this, &Dialog::_ok_pressed));
		_ok_btn->connect(SceneStringName(visibility_changed), callable_mp(this, &Dialog::_button_visibility_changed).bind(_ok_btn));
		_ok_btn->set_meta("__bound_spacer", bound_spacer);
		_buttons_hbox->add_child(_ok_btn);
		_buttons_hbox->move_child(_ok_btn, bound_spacer->get_index());
	}
	_set_default_ok_text(ETR("OK"));
	_cancel_btn = add_cancel_button();
	set_cancel_button_visible(p_show_cancel);
	if (p_show_cancel) {
		set_default_title(ETR("Please Confirm..."));
		set_min_size(Size2(200, 70));
	} else {
		set_default_title(ETR("Alert!"));
	}
}

void Dialog::register_text_enter(LineEdit *p_line_edit) {
	ERR_FAIL_NULL(p_line_edit);
	p_line_edit->connect(SceneStringName(text_submitted), callable_mp(this, &Dialog::_text_submitted));
}

Button *Dialog::add_button(const String &p_text, bool p_right, const String &p_action) {
	Control *bound_spacer = _buttons_hbox->add_spacer(!p_right);
	Button *btn = memnew(Button);
	if (!p_action.is_empty()) {
		btn->connect(SceneStringName(pressed), callable_mp(this, &Dialog::_custom_action).bind(p_action));
	}
	btn->connect(SceneStringName(visibility_changed), callable_mp(this, &Dialog::_button_visibility_changed).bind(btn));
	btn->set_meta("__bound_spacer", bound_spacer);
	btn->set_text(p_text);
	_buttons_hbox->add_child(btn);
	_buttons_hbox->move_child(btn, p_right ? bound_spacer->get_index() : 1);

	child_controls_changed();
	if (is_visible()) {
		_update_child_rects();
	}
	return btn;
}

Button *Dialog::add_cancel_button(const String &p_cancel) {
	String c = p_cancel;
	if (p_cancel.is_empty()) {
		c = ETR("Cancel");
	}

	Button *b = add_button(c);

	b->connect(SceneStringName(pressed), callable_mp(this, &Dialog::_cancel_pressed));

	_update_confirmation_style();
	return b;
}

void Dialog::remove_button(Button *p_button) {
	ERR_FAIL_NULL(p_button);
	ERR_FAIL_COND_MSG(p_button->get_parent() != _buttons_hbox, vformat("Cannot remove button %s as it does not belong to this dialog.", p_button->get_name()));
	ERR_FAIL_COND_MSG(p_button == _ok_btn || p_button == _cancel_btn, "Cannot remove the dialog's built-in buttons. Hide them instead.");

	Control *bound_spacer = Object::cast_to<Control>(p_button->get_meta("__bound_spacer"));
	if (bound_spacer) {
		ERR_FAIL_COND_MSG(bound_spacer->get_parent() != _buttons_hbox, vformat("Cannot remove button %s as its associated spacer does not belong to this dialog.", p_button->get_name()));
	}

	p_button->disconnect(SceneStringName(visibility_changed), callable_mp(this, &Dialog::_button_visibility_changed));
	if (p_button->is_connected(SceneStringName(pressed), callable_mp(this, &Dialog::_custom_action))) {
		p_button->disconnect(SceneStringName(pressed), callable_mp(this, &Dialog::_custom_action));
	}
	if (p_button->is_connected(SceneStringName(pressed), callable_mp(this, &Dialog::_cancel_pressed))) {
		p_button->disconnect(SceneStringName(pressed), callable_mp(this, &Dialog::_cancel_pressed));
	}

	if (bound_spacer) {
		_buttons_hbox->remove_child(bound_spacer);
		p_button->remove_meta("__bound_spacer");
		bound_spacer->queue_free();
	}
	_buttons_hbox->remove_child(p_button);
	_update_confirmation_style();

	child_controls_changed();
	if (is_visible()) {
		_update_child_rects();
	}
}

void Dialog::set_cancel_button_visible(bool p_visible) {
	_cancel_btn->set_visible(p_visible);
	_update_confirmation_style();
}

bool Dialog::is_cancel_button_visible() const {
	return _cancel_btn->is_visible();
}

void Dialog::set_cancel_button_text(String p_cancel_button_text) {
	_cancel_btn->set_text(p_cancel_button_text);
}

String Dialog::get_cancel_button_text() const {
	return _cancel_btn->get_text();
}

void Dialog::set_hide_on_ok(bool p_hide) {
	_hide_on_ok = p_hide;
}

bool Dialog::get_hide_on_ok() const {
	return _hide_on_ok;
}

void Dialog::set_close_on_escape(bool p_enable) {
	_close_on_escape = p_enable;
}

bool Dialog::get_close_on_escape() const {
	return _close_on_escape;
}

void Dialog::set_text(String p_text) {
	if (_message_lbl->get_text() == p_text) {
		return;
	}

	_message_lbl->set_text(p_text);

	child_controls_changed();
	if (is_visible()) {
		_update_child_rects();
	}
}

String Dialog::get_text() const {
	return _message_lbl->get_text();
}

void Dialog::set_autowrap(bool p_autowrap) {
	_message_lbl->set_autowrap_mode(p_autowrap ? TextServer::AUTOWRAP_WORD : TextServer::AUTOWRAP_OFF);
}

bool Dialog::has_autowrap() {
	return _message_lbl->get_autowrap_mode() != TextServer::AUTOWRAP_OFF;
}

void Dialog::set_ok_button_text(String p_ok_button_text) {
	_ok_text = p_ok_button_text;
	_update_ok_text();
}

String Dialog::get_ok_button_text() const {
	return _ok_text;
}

void Dialog::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_ok_button"), &Dialog::get_ok_button);
	ClassDB::bind_method(D_METHOD("get_label"), &Dialog::get_label);
	ClassDB::bind_method(D_METHOD("set_hide_on_ok", "hide_on_ok"), &Dialog::set_hide_on_ok);
	ClassDB::bind_method(D_METHOD("get_hide_on_ok"), &Dialog::get_hide_on_ok);
	ClassDB::bind_method(D_METHOD("set_close_on_escape", "close_on_escape"), &Dialog::set_close_on_escape);
	ClassDB::bind_method(D_METHOD("get_close_on_escape"), &Dialog::get_close_on_escape);
	ClassDB::bind_method(D_METHOD("add_button", "text", "right", "action"), &Dialog::add_button, DEFVAL(false), DEFVAL(""));
	ClassDB::bind_method(D_METHOD("add_cancel_button", "name"), &Dialog::add_cancel_button, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("remove_button", "button"), &Dialog::remove_button);
	ClassDB::bind_method(D_METHOD("register_text_enter", "line_edit"), &Dialog::register_text_enter);
	ClassDB::bind_method(D_METHOD("set_text", "text"), &Dialog::set_text);
	ClassDB::bind_method(D_METHOD("get_text"), &Dialog::get_text);
	ClassDB::bind_method(D_METHOD("set_autowrap", "autowrap"), &Dialog::set_autowrap);
	ClassDB::bind_method(D_METHOD("has_autowrap"), &Dialog::has_autowrap);
	ClassDB::bind_method(D_METHOD("set_ok_button_text", "ok_button_text"), &Dialog::set_ok_button_text);
	ClassDB::bind_method(D_METHOD("get_ok_button_text"), &Dialog::get_ok_button_text);

	ClassDB::bind_method(D_METHOD("get_cancel_button"), &Dialog::get_cancel_button);
	ClassDB::bind_method(D_METHOD("set_cancel_button_visible", "cancel_button_visible"), &Dialog::set_cancel_button_visible);
	ClassDB::bind_method(D_METHOD("is_cancel_button_visible"), &Dialog::is_cancel_button_visible);
	ClassDB::bind_method(D_METHOD("set_cancel_button_text", "cancel_button_text"), &Dialog::set_cancel_button_text);
	ClassDB::bind_method(D_METHOD("get_cancel_button_text"), &Dialog::get_cancel_button_text);

	ADD_SIGNAL(MethodInfo("confirmed"));
	ADD_SIGNAL(MethodInfo("canceled"));
	ADD_SIGNAL(MethodInfo("custom_action", PropertyInfo(Variant::STRING_NAME, "action")));

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "ok_button_text"), "set_ok_button_text", "get_ok_button_text");

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "cancel_button_text"), "set_cancel_button_text", "get_cancel_button_text");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "cancel_button_visible"), "set_cancel_button_visible", "is_cancel_button_visible");

	ADD_GROUP("Dialog", "dialog_");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "dialog_text", PROPERTY_HINT_MULTILINE_TEXT), "set_text", "get_text");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "dialog_hide_on_ok"), "set_hide_on_ok", "get_hide_on_ok");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "dialog_close_on_escape"), "set_close_on_escape", "get_close_on_escape");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "dialog_autowrap"), "set_autowrap", "has_autowrap");

	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_STYLEBOX, Dialog, panel_style, "panel");
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, Dialog, buttons_separation);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, Dialog, buttons_min_width);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, Dialog, buttons_min_height);

	ADD_CLASS_DEPENDENCY("Button");
}

void Dialog::_notification(int p_what) {
	switch (p_what) {

		case NOTIFICATION_POST_ENTER_TREE: {
			_update_confirmation_style();
			if (is_visible()) {
				get_ok_button()->grab_focus();
			}
		} break;
		case NOTIFICATION_VISIBILITY_CHANGED: {
			if (is_visible()) {
				if (get_ok_button()->is_inside_tree()) {
					get_ok_button()->grab_focus();
				}
				_update_child_rects();

				_parent_visible = get_parent_visible_window();
				if (_parent_visible) {
					_parent_visible->connect(SceneStringName(focus_entered), callable_mp(this, &Dialog::_parent_focused));
				}
			} else {
				_popped_up = false;
				if (_parent_visible) {
					_parent_visible->disconnect(SceneStringName(focus_entered), callable_mp(this, &Dialog::_parent_focused));
					_parent_visible = nullptr;
				}
			}
		} break;

		case NOTIFICATION_WM_WINDOW_FOCUS_IN: {
			if (!is_in_edited_scene_root()) {
				if (has_focus()) {
					_popped_up = true;
				}
			}
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			_bg_panel->add_theme_style_override(SceneStringName(panel), theme_cache.panel_style);

			child_controls_changed();
			if (is_visible()) {
				_update_child_rects();
			}
		} break;

		case NOTIFICATION_EXIT_TREE: {
			if (_parent_visible) {
				_parent_visible->disconnect(SceneStringName(focus_entered), callable_mp(this, &Dialog::_parent_focused));
				_parent_visible = nullptr;
			}
		} break;

		case NOTIFICATION_READY:
		case NOTIFICATION_WM_SIZE_CHANGED: {
			if (is_visible()) {
				_update_child_rects();
			}
		} break;

		case NOTIFICATION_WM_CLOSE_REQUEST: {
			_cancel_pressed();
		} break;
	}
}

Size2 Dialog::_get_contents_minimum_size() const {
	// First, we then iterate over the label and any other custom controls
	// to try and find the size that encompasses all content.
	Size2 content_minsize;
	for (int i = 0; i < get_child_count(); i++) {
		Control *c = Object::cast_to<Control>(get_child(i));
		if (!c) {
			continue;
		}

		// Buttons will be included afterwards.
		// The panel only displays the stylebox and doesn't contribute to the size.
		if (c == _buttons_hbox || c == _bg_panel || c->is_set_as_top_level()) {
			continue;
		}

		Size2 child_minsize = c->get_bound_minimum_size();
		content_minsize = child_minsize.max(content_minsize);
	}

	// Then we add buttons. Horizontally we're interested in whichever
	// value is the biggest. Vertically buttons add to the overall size.
	Size2 buttons_minsize = _buttons_hbox->get_bound_minimum_size();
	content_minsize.x = MAX(buttons_minsize.x, content_minsize.x);
	content_minsize.y += buttons_minsize.y;
	// Plus there is a separation size added on top.
	content_minsize.y += theme_cache.buttons_separation;

	// Then we take the background panel as it provides the offsets,
	// which are always added to the minimum size.
	if (theme_cache.panel_style.is_valid()) {
		content_minsize += theme_cache.panel_style->get_minimum_size();
	}

	return content_minsize;
}

void Dialog::_input_from_window(const Ref<InputEvent> &p_event) {
	if (_close_on_escape && p_event->is_action_pressed(SNAME("ui_close_dialog"), false, true)) {
		_cancel_pressed();
	}
	Window::_input_from_window(p_event);
}

void Dialog::_post_popup() {
	Window::_post_popup();
	_popped_up = true;
}

void Dialog::_validate_property(PropertyInfo &r_property) const {
	if (!Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	if (r_property.name == "ok_button_text") {
		r_property.hint = PROPERTY_HINT_PLACEHOLDER_TEXT;
		r_property.hint_string = _default_ok_text;
	}
}

void Dialog::_set_default_ok_text(const String &p_text) {
	if (_default_ok_text == p_text) {
		return;
	}
	_default_ok_text = p_text;
	_update_ok_text();
	notify_property_list_changed();
}

void Dialog::_text_submitted(const String &p_text) {
	if (get_ok_button() && get_ok_button()->is_disabled()) {
		return; // Do not allow submission if OK button is disabled.
	}
	_ok_pressed();
}

void Dialog::_ok_pressed() {
	if (_hide_on_ok) {
		_popped_up = false;
		set_visible(false);
	}
	_on_confirmed();
	emit_signal(SceneStringName(confirmed));
	set_input_as_handled();
}

void Dialog::_cancel_pressed() {
	_popped_up = false;
	Window *parent_window = _parent_visible;
	if (_parent_visible) {
		_parent_visible->disconnect(SceneStringName(focus_entered), callable_mp(this, &Dialog::_parent_focused));
		_parent_visible = nullptr;
	}

	callable_mp((Window *)this, &Window::hide).call_deferred();

	emit_signal(SNAME("canceled"));

	_on_canceled();

	if (parent_window) {
		//parent_window->grab_focus();
	}
	set_input_as_handled();
}

void Dialog::_custom_action(const String &p_action) {
	emit_signal(SNAME("custom_action"), p_action);
	_on_custom_action(p_action);
}

void Dialog::_button_visibility_changed(Button *p_button) {
	Control *bound_spacer = Object::cast_to<Control>(p_button->get_meta("__bound_spacer"));
	if (bound_spacer) {
		bound_spacer->set_visible(p_button->is_visible());
	}
	_update_confirmation_style();
	child_controls_changed();
	if (is_visible()) {
		_update_child_rects();
	}
}

void Dialog::_update_child_rects() {
	Size2 dlg_size = Vector2(get_size()) / get_content_scale_factor();
	float h_margins = theme_cache.panel_style->get_margin(SIDE_LEFT) + theme_cache.panel_style->get_margin(SIDE_RIGHT);
	float v_margins = theme_cache.panel_style->get_margin(SIDE_TOP) + theme_cache.panel_style->get_margin(SIDE_BOTTOM);

	// Fill the entire size of the window with the background.
	_bg_panel->set_position(Point2());
	_bg_panel->set_size(dlg_size);

	for (int i = 0; i < _buttons_hbox->get_child_count(); i++) {
		Button *b = Object::cast_to<Button>(_buttons_hbox->get_child(i));
		if (!b) {
			continue;
		}

		b->set_custom_minimum_size(Size2(theme_cache.buttons_min_width, theme_cache.buttons_min_height));
	}

	// Place the buttons from the bottom edge to their minimum required size.
	Size2 buttons_minsize = _buttons_hbox->get_bound_minimum_size();
	Size2 buttons_size = Size2(dlg_size.x - h_margins, buttons_minsize.y);
	Point2 buttons_position = Point2(theme_cache.panel_style->get_margin(SIDE_LEFT), dlg_size.y - theme_cache.panel_style->get_margin(SIDE_BOTTOM) - buttons_size.y);
	_buttons_hbox->set_position(buttons_position);
	_buttons_hbox->set_size(buttons_size);

	// Place the content from the top to fill the rest of the space (minus the separation).
	Point2 content_position = Point2(theme_cache.panel_style->get_margin(SIDE_LEFT), theme_cache.panel_style->get_margin(SIDE_TOP));
	Size2 content_size = Size2(dlg_size.x - h_margins, dlg_size.y - v_margins - buttons_size.y - theme_cache.buttons_separation);

	for (int i = 0; i < get_child_count(); i++) {
		Control *c = Object::cast_to<Control>(get_child(i));
		if (!c) {
			continue;
		}
		if (c == _buttons_hbox || c == _bg_panel || c->is_set_as_top_level()) {
			continue;
		}

		c->set_position(content_position);
		c->set_size(content_size);
	}
}

void Dialog::_update_ok_text() {
	String prev_text = _ok_btn->get_text();
	String new_text = _default_ok_text;

	if (!_ok_text.is_empty()) {
		new_text = _ok_text;
	}

	if (new_text == prev_text) {
		return;
	}
	_ok_btn->set_text(new_text);

	child_controls_changed();
	if (is_visible()) {
		_update_child_rects();
	}
}

void Dialog::_parent_focused() {
	if (_popped_up && !is_exclusive() && get_flag(FLAG_POPUP)) {
		_cancel_pressed();
	}
}

void Dialog::_update_confirmation_style() {
	bool has_cancel = false;
	for (int i = 0; i < _buttons_hbox->get_child_count(); i++) {
		const Button *btn = Object::cast_to<Button>(_buttons_hbox->get_child(i));
		if (btn && btn->is_visible() && btn->is_connected(SceneStringName(pressed), callable_mp(this, &Dialog::_cancel_pressed))) {
			has_cancel = true;
			break;
		}
	}
	_ok_btn->set_theme_type_variation(has_cancel ? SNAME("PrimaryButton") : StringName());
}
