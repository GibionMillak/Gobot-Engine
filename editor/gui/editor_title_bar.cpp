/**************************************************************************/
/*  editor_title_bar.cpp                                                  */
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

#include "editor_title_bar.h"

#include "core/object/callable_mp.h"
#include "editor/editor_string_names.h"
#include "scene/gui/button.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "servers/display/display_server.h"

static Ref<Texture2D> _get_theme_icon_or_fallback(const Control *p_control, const StringName &p_name, const StringName &p_fallback) {
	if (p_control) {
		if (p_control->has_theme_icon(p_name, EditorStringName(EditorIcons))) {
			return p_control->get_editor_theme_icon(p_name);
		}
		if (p_control->has_theme_icon(p_fallback, EditorStringName(EditorIcons))) {
			return p_control->get_editor_theme_icon(p_fallback);
		}
	}
	return Ref<Texture2D>();
}

void EditorTitleBar::set_center_control(Control *p_center_control) {
	_center_control = p_center_control;

	if (_center_control->get_parent() != this) {
		_center_control->reparent(this);
	}
}

Control *EditorTitleBar::get_center_control() const {
	return _center_control;
}

void EditorTitleBar::set_can_move_window(bool p_enabled) {
	_can_move = p_enabled;
	set_process_input(_can_move);

	if (_can_move && DisplayServer::get_singleton()->window_get_safe_title_margins() == Vector3i()) {
		_create_window_buttons();
	} else {
		_destroy_window_buttons();
	}
}

bool EditorTitleBar::get_can_move_window() const {
	return _can_move;
}

void EditorTitleBar::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_EXIT_TREE: {
			SceneTree::get_singleton()->get_root()->disconnect(SceneStringName(nonclient_window_input), callable_mp(this, &EditorTitleBar::gui_input));
			get_window()->set_nonclient_area(Rect2i());
		} break;
		case NOTIFICATION_ENTER_TREE: {
			SceneTree::get_singleton()->get_root()->connect(SceneStringName(nonclient_window_input), callable_mp(this, &EditorTitleBar::gui_input));
			[[fallthrough]];
		}
		case NOTIFICATION_RESIZED: {
			get_window()->set_nonclient_area(get_global_transform().xform(Rect2i(get_position(), get_size())));
			_update_window_buttons();
		} break;
		case NOTIFICATION_THEME_CHANGED: {
			_update_window_buttons();
		} break;
		case NOTIFICATION_SORT_CHILDREN: {
			if (_window_buttons_hb && _window_buttons_hb->get_index() != get_child_count() - 1) {
				move_child(_window_buttons_hb, get_child_count() - 1);
			}

			if (!_center_control) {
				break;
			}
			Control *prev = nullptr;
			Control *base = nullptr;
			Control *next = nullptr;

			bool rtl = is_layout_rtl();

			int start;
			int end;
			int delta;
			if (rtl) {
				start = get_child_count() - 1;
				end = -1;
				delta = -1;
			} else {
				start = 0;
				end = get_child_count();
				delta = +1;
			}

			for (int i = start; i != end; i += delta) {
				Control *c = as_sortable_control(get_child(i));
				if (!c) {
					continue;
				}
				if (base) {
					next = c;
					break;
				}
				if (c != _center_control) {
					prev = c;
					continue;
				}
				base = c;
			}
			if (base && prev && next) {
				Size2i title_size = get_size();
				int center = title_size.x / 2;
				int width = MIN(center - prev->get_position().x, next->get_position().x + next->get_size().x - center) * 1.9;
				width = MAX(width, base->get_combined_minimum_size().x);
				base->set_custom_maximum_size(Size2i(width, title_size.y));
				Size2i c_size = base->get_bound_desired_size();

				int min_offset = prev->get_position().x + prev->get_combined_minimum_size().x;
				int max_offset = next->get_position().x + next->get_size().x - next->get_combined_minimum_size().x - c_size.x;

				int offset = (title_size.width - c_size.width) / 2;
				offset = CLAMP(offset, min_offset, max_offset);

				fit_child_in_rect(prev, Rect2i(prev->get_position().x, 0, offset - prev->get_position().x, title_size.height));
				fit_child_in_rect(next, Rect2i(offset + c_size.width, 0, next->get_position().x + next->get_size().x - (offset + c_size.width), title_size.height));
				fit_child_in_rect(base, Rect2i(offset, 0, c_size.width, title_size.height));
			}
		} break;
	}
}

void EditorTitleBar::gui_input(const Ref<InputEvent> &p_event) {
	if (!_can_move) {
		return;
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid() && _moving) {
		if (mm->get_button_mask().has_flag(MouseButtonMask::LEFT)) {
			Window *w = Object::cast_to<Window>(get_viewport());
			if (w) {
				Point2 mouse = DisplayServer::get_singleton()->mouse_get_position();
				w->set_position(mouse - _click_pos);
			}
		} else {
			_moving = false;
		}
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && has_point(mb->get_position())) {
		Window *w = Object::cast_to<Window>(get_viewport());
		if (w) {
			if (mb->get_button_index() == MouseButton::LEFT) {
				if (mb->is_pressed()) {
					if (DisplayServer::get_singleton()->has_feature(DisplayServerEnums::FEATURE_WINDOW_DRAG)) {
						DisplayServer::get_singleton()->window_start_drag(w->get_window_id());
					} else {
						_click_pos = DisplayServer::get_singleton()->mouse_get_position() - w->get_position();
						_moving = true;
					}
				} else {
					_moving = false;
				}
			}
			if (mb->get_button_index() == MouseButton::LEFT && mb->is_double_click() && mb->is_pressed()) {
				if (DisplayServer::get_singleton()->window_maximize_on_title_dbl_click()) {
					if (w->get_mode() == Window::MODE_WINDOWED) {
						w->set_mode(Window::MODE_MAXIMIZED);
					} else if (w->get_mode() == Window::MODE_MAXIMIZED) {
						w->set_mode(Window::MODE_WINDOWED);
					}
				} else if (DisplayServer::get_singleton()->window_minimize_on_title_dbl_click()) {
					w->set_mode(Window::MODE_MINIMIZED);
				}
				_moving = false;
			}
		}
	}
}

void EditorTitleBar::_on_minimize_pressed() {
	Window *w = get_window();
	if (!w) {
		w = Object::cast_to<Window>(get_viewport());
	}
	if (w) {
		w->set_mode(Window::MODE_MINIMIZED);
	}
}

void EditorTitleBar::_on_maximize_pressed() {
	Window *w = get_window();
	if (!w) {
		w = Object::cast_to<Window>(get_viewport());
	}
	if (w) {
		if (w->get_mode() == Window::MODE_MAXIMIZED) {
			w->set_mode(Window::MODE_WINDOWED);
		} else {
			w->set_mode(Window::MODE_MAXIMIZED);
		}
		_update_window_buttons();
	}
}

void EditorTitleBar::_on_close_pressed() {
	Window *w = get_window();
	if (!w) {
		w = Object::cast_to<Window>(get_viewport());
	}
	if (!w && SceneTree::get_singleton()) {
		w = SceneTree::get_singleton()->get_root();
	}
	if (w) {
		w->propagate_notification(Node::NOTIFICATION_WM_CLOSE_REQUEST);
		w->emit_signal(SNAME("close_requested"));
	} else if (SceneTree::get_singleton()) {
		SceneTree::get_singleton()->quit();
	}
}

void EditorTitleBar::_update_window_buttons() {
	if (!_window_buttons_hb) {
		return;
	}

	_btn_minimize->set_button_icon(_get_theme_icon_or_fallback(this, SNAME("editor_window_minimize"), SNAME("collapse")));
	_btn_close->set_button_icon(_get_theme_icon_or_fallback(this, SNAME("editor_window_close"), SNAME("action_close")));

	Window *w = Object::cast_to<Window>(get_viewport());
	bool is_max = w && (w->get_mode() == Window::MODE_MAXIMIZED);
	if (is_max) {
		_btn_maximize->set_button_icon(_get_theme_icon_or_fallback(this, SNAME("editor_window_restore"), SNAME("editor_window_maximize")));
		_btn_maximize->set_tooltip_text(TTR("Restore"));
	} else {
		_btn_maximize->set_button_icon(_get_theme_icon_or_fallback(this, SNAME("editor_window_maximize"), SNAME("expand_tree")));
		_btn_maximize->set_tooltip_text(TTR("Maximize"));
	}
}

void EditorTitleBar::_create_window_buttons() {
	if (_window_buttons_hb) {
		return;
	}

	_window_buttons_hb = memnew(HBoxContainer);
	_window_buttons_hb->set_mouse_filter(MOUSE_FILTER_STOP);

	_btn_minimize = memnew(Button);
	_btn_minimize->set_focus_mode(FOCUS_NONE);
	_btn_minimize->set_tooltip_text(TTR("Minimize"));
	_btn_minimize->connect(SceneStringName(pressed), callable_mp(this, &EditorTitleBar::_on_minimize_pressed));
	_window_buttons_hb->add_child(_btn_minimize);

	_btn_maximize = memnew(Button);
	_btn_maximize->set_focus_mode(FOCUS_NONE);
	_btn_maximize->connect(SceneStringName(pressed), callable_mp(this, &EditorTitleBar::_on_maximize_pressed));
	_window_buttons_hb->add_child(_btn_maximize);

	_btn_close = memnew(Button);
	_btn_close->set_focus_mode(FOCUS_NONE);
	_btn_close->set_tooltip_text(TTR("Close"));
	_btn_close->connect(SceneStringName(pressed), callable_mp(this, &EditorTitleBar::_on_close_pressed));
	_window_buttons_hb->add_child(_btn_close);

	add_child(_window_buttons_hb);
	_update_window_buttons();
}

void EditorTitleBar::_destroy_window_buttons() {
	if (_window_buttons_hb) {
		_window_buttons_hb->queue_free();
		_window_buttons_hb = nullptr;
		_btn_minimize = nullptr;
		_btn_maximize = nullptr;
		_btn_close = nullptr;
	}
}
