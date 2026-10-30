/**************************************************************************/
/*  editor_viewport_toolbar.cpp                                           */
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

#include "editor_viewport_toolbar.h"

#include "core/math/math_funcs.h"
#include "core/object/callable_mp.h"
#include "editor/gui/containers/editor_toolbar_flow_container.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/popup.h"
#include "scene/gui/scroll_container.h"
#include "scene/main/viewport.h"
#include "scene/resources/style_box.h"
#include "scene/scene_string_names.h"

EditorViewportToolbar::EditorViewportToolbar() {
	set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	set_clip_contents(true);
	set_mouse_filter(MOUSE_FILTER_IGNORE);

	_toolbar_scroll = memnew(ScrollContainer);
	_toolbar_scroll->set_follow_focus(true);
	_toolbar_scroll->set_force_pass_scroll_events(false);
	_toolbar_scroll->set_theme_type_variation("ViewportToolbar");
	add_child(_toolbar_scroll);
	{
		_content_vflow = memnew(EditorToolbarFlowContainer);
		_content_vflow->set_group_separators_enabled(true);
		_content_vflow->set_theme_type_variation("ViewportToolbarGroups");
		_toolbar_scroll->add_child(_content_vflow);
		_content_vflow->connect(SceneStringName(minimum_size_changed), callable_mp(this, &EditorViewportToolbar::_queue_layout));
	}

	_context_scroll = memnew(ScrollContainer);
	_context_scroll->hide();
	_context_scroll->set_follow_focus(true);
	_context_scroll->set_force_pass_scroll_events(false);
	_context_scroll->set_theme_type_variation("ViewportToolbar");
	add_child(_context_scroll);
}

EditorToolbarFlowContainer *EditorViewportToolbar::create_group() {
	EditorToolbarFlowContainer *group_vflow = memnew(EditorToolbarFlowContainer);
	_content_vflow->add_child(group_vflow);
	_controls_changed();
	return group_vflow;
}

Vector2 EditorViewportToolbar::get_overlay_insets() const {
	return _overlay_insets;
}

void EditorViewportToolbar::set_context_control(EditorToolbarFlowContainer *p_control) {
	ERR_FAIL_NULL(p_control);
	ERR_FAIL_COND(_context_control);
	ERR_FAIL_COND(p_control->get_parent());
	_context_control = p_control;
	_context_control->set_group_separators_enabled(true);
	_context_control->set_theme_type_variation("ViewportToolbarGroups");
	_context_scroll->add_child(_context_control);
	_context_control->connect(SceneStringName(minimum_size_changed), callable_mp(this, &EditorViewportToolbar::_queue_layout));
	_controls_changed();
}

void EditorViewportToolbar::set_context_visible(bool p_visible) {
	if (_context_visible == p_visible) {
		return;
	}
	_context_visible = p_visible;
	_queue_layout();
}

void EditorViewportToolbar::popup_tool(Popup *p_popup, Control *p_anchor) {
	ERR_FAIL_NULL(p_popup);
	ERR_FAIL_NULL(p_anchor);
	_position_popup(p_popup, p_anchor);
	p_popup->popup();
}

void EditorViewportToolbar::_bind_methods() {
	ADD_SIGNAL(MethodInfo("overlay_insets_changed"));
}

void EditorViewportToolbar::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY:
		case NOTIFICATION_THEME_CHANGED: {
			_controls_changed();
		} break;

		case NOTIFICATION_RESIZED:
		case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
			_queue_layout();
		} break;
	}
}

void EditorViewportToolbar::_queue_layout() {
	if (_layout_pending || !is_inside_tree()) {
		return;
	}
	_layout_pending = true;
	callable_mp(this, &EditorViewportToolbar::_update_layout).call_deferred();
}

void EditorViewportToolbar::_controls_changed() {
	_controls_dirty = true;
	_queue_layout();
}

void EditorViewportToolbar::_update_layout() {
	_layout_pending = false;
	if (!is_inside_tree()) {
		return;
	}
	if (_controls_dirty) {
		_refresh_controls();
		_controls_dirty = false;
	}

	const bool right_side = int(EDITOR_GET("editors/viewport/toolbar_side")) == 1;
	const bool show_context = _context_visible && _context_control;
	const real_t margin = get_theme_constant(SNAME("margin"), SNAME("ViewportToolbar"));
	const real_t available_height = MAX(0.0f, get_size().y - 2 * margin);
	const real_t available_width = MAX(0.0f, get_size().x - (show_context ? 3 : 2) * margin);
	const Size2 padding = _toolbar_scroll->get_theme_stylebox(SceneStringName(panel))->get_minimum_size();
	_configure_flow(_content_vflow, MAX(1, int(Math::floor(available_height - padding.y))), right_side);
	if (_context_control) {
		_configure_flow(_context_control, MAX(1, int(Math::floor(available_height - padding.y))), !right_side);
	}

	const real_t minimum_width = 32 * EDSCALE + padding.x;
	Size2 content_size = _content_vflow->get_bound_desired_size() + padding;
	Size2 context_size = show_context ? _context_control->get_bound_desired_size() + padding : Size2();
	if (MAX(content_size.x, minimum_width) + (show_context ? MAX(context_size.x, minimum_width) : 0) > available_width) {
		// Keep every control reachable when wrapped columns no longer fit between the edges.
		_configure_flow(_content_vflow, 0, right_side);
		content_size = _content_vflow->get_bound_desired_size() + padding;
		if (show_context) {
			_configure_flow(_context_control, 0, !right_side);
			context_size = _context_control->get_bound_desired_size() + padding;
		}
	}

	if (content_size.y > available_height) {
		content_size.x += _toolbar_scroll->get_v_scroll_bar()->get_combined_minimum_size().x + _toolbar_scroll->get_theme_constant(SNAME("scrollbar_h_separation"));
	}
	if (show_context && context_size.y > available_height) {
		context_size.x += _context_scroll->get_v_scroll_bar()->get_combined_minimum_size().x + _context_scroll->get_theme_constant(SNAME("scrollbar_h_separation"));
	}

	const real_t main_width = MAX(content_size.x, minimum_width);
	const real_t context_width = show_context ? MAX(context_size.x, minimum_width) : 0;
	const real_t main_budget = show_context ? MIN(main_width, MAX(Math::floor(available_width * 0.5f), available_width - context_width)) : available_width;
	const Rect2 toolbar_rect = _get_toolbar_rect(content_size, right_side, main_budget);
	_toolbar_scroll->set_visible(toolbar_rect.has_area());
	_toolbar_scroll->set_size(toolbar_rect.size);
	_toolbar_scroll->set_position(toolbar_rect.position);

	Rect2 context_rect;
	if (_context_control) {
		context_rect = _get_toolbar_rect(context_size, !right_side, MAX(0.0f, available_width - toolbar_rect.size.x));
		_context_scroll->set_visible(show_context && context_rect.has_area());
		_context_scroll->set_size(context_rect.size);
		_context_scroll->set_position(context_rect.position);
	}

	Vector2 insets(margin, margin);
	if (toolbar_rect.has_area()) {
		insets[right_side ? 1 : 0] += toolbar_rect.size.x + margin;
	}
	if (show_context && context_rect.has_area()) {
		insets[right_side ? 0 : 1] += context_rect.size.x + margin;
	}
	if (_overlay_insets != insets) {
		_overlay_insets = insets;
		emit_signal(SNAME("overlay_insets_changed"));
	}
}

Rect2 EditorViewportToolbar::_get_toolbar_rect(const Size2 &p_content_size, bool p_right_side, real_t p_width_limit) const {
	const real_t margin = get_theme_constant(SNAME("margin"), SNAME("ViewportToolbar"));
	const real_t available_height = MAX(0.0f, get_size().y - 2 * margin);
	const real_t minimum_width = 32 * EDSCALE + get_theme_stylebox(SceneStringName(panel), SNAME("ViewportToolbar"))->get_minimum_size().x;
	const Size2 toolbar_size(MIN(MAX(p_content_size.x, minimum_width), p_width_limit), MIN(p_content_size.y, available_height));
	const Vector2 position(p_right_side ? MAX(margin, get_size().x - margin - toolbar_size.x) : margin, MAX(margin, Math::floor((get_size().y - toolbar_size.y) * 0.5f)));
	return Rect2(position, toolbar_size);
}

void EditorViewportToolbar::_configure_flow(Control *p_control, int p_height, bool p_right_side) {
	for (int i = 0; i < p_control->get_child_count(); i++) {
		Control *child = Object::cast_to<Control>(p_control->get_child(i));
		if (child && !child->is_set_as_top_level()) {
			_configure_flow(child, p_height, p_right_side);
		}
	}
	EditorToolbarFlowContainer *flow = Object::cast_to<EditorToolbarFlowContainer>(p_control);
	if (flow) {
		flow->set_height_limit(p_height);
		flow->set_reverse_fill(p_right_side);
	}
}

void EditorViewportToolbar::_refresh_controls() {
	HashSet<ObjectID> controls;
	HashMap<ObjectID, Callable> popup_connections;
	_configure_buttons(_content_vflow, controls, popup_connections);
	if (_context_control) {
		_configure_buttons(_context_control, controls, popup_connections);
	}

	const Callable controls_changed = callable_mp(this, &EditorViewportToolbar::_controls_changed);
	for (const ObjectID &id : _observed_controls) {
		Control *control = Object::cast_to<Control>(ObjectDB::get_instance(id));
		if (control && !controls.has(id)) {
			control->disconnect(SNAME("child_order_changed"), controls_changed);
		}
	}
	for (const KeyValue<ObjectID, Callable> &entry : _popup_connections) {
		Popup *popup = Object::cast_to<Popup>(ObjectDB::get_instance(entry.key));
		if (popup && (!popup_connections.has(entry.key) || popup_connections[entry.key] != entry.value)) {
			popup->disconnect(SNAME("about_to_popup"), entry.value);
		}
	}
	_observed_controls = controls;
	_popup_connections = popup_connections;
}

void EditorViewportToolbar::_configure_buttons(Control *p_control, HashSet<ObjectID> &r_controls, HashMap<ObjectID, Callable> &r_popup_connections) {
	r_controls.insert(p_control->get_instance_id());
	Button *button = Object::cast_to<Button>(p_control);
	if (button) {
		button->set_custom_minimum_size(Size2(32, 32) * EDSCALE);
		button->set_icon_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	}
	const Callable controls_changed = callable_mp(this, &EditorViewportToolbar::_controls_changed);
	if (!p_control->is_connected(SNAME("child_order_changed"), controls_changed)) {
		p_control->connect(SNAME("child_order_changed"), controls_changed);
	}

	for (int i = 0; i < p_control->get_child_count(); i++) {
		Node *child_node = p_control->get_child(i);
		Control *child = Object::cast_to<Control>(child_node);
		if (child) {
			_configure_buttons(child, r_controls, r_popup_connections);
		}
		Popup *popup = Object::cast_to<Popup>(child_node);
		if (button && popup) {
			const Callable position_popup = callable_mp(this, &EditorViewportToolbar::_position_popup).bind(popup, button);
			r_popup_connections[popup->get_instance_id()] = position_popup;
			if (!popup->is_connected(SNAME("about_to_popup"), position_popup)) {
				popup->connect(SNAME("about_to_popup"), position_popup);
			}
		}
	}
}

void EditorViewportToolbar::_position_popup(Popup *p_popup, Control *p_anchor) {
	if (!p_anchor || !is_ancestor_of(p_anchor)) {
		return;
	}
	p_popup->reset_size();
	const bool contextual = _context_control && (_context_control == p_anchor || _context_control->is_ancestor_of(p_anchor));
	Rect2 anchor_rect = p_anchor->get_screen_rect();
	Rect2 toolbar_rect = (contextual ? _context_scroll : _toolbar_scroll)->get_screen_rect();
	if (get_viewport()->is_embedding_subwindows() && p_popup->get_force_native()) {
		anchor_rect = get_viewport()->get_popup_base_transform_native().xform(anchor_rect);
		toolbar_rect = get_viewport()->get_popup_base_transform_native().xform(toolbar_rect);
	}
	const bool main_right_side = int(EDITOR_GET("editors/viewport/toolbar_side")) == 1;
	const bool right_side = contextual ? !main_right_side : main_right_side;
	const Vector2 popup_position(right_side ? toolbar_rect.position.x - p_popup->get_size().x : toolbar_rect.get_end().x, anchor_rect.position.y);
	p_popup->set_position(popup_position);
}
