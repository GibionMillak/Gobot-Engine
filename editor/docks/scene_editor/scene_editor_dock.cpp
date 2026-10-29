/**************************************************************************/
/*  scene_editor_dock.cpp                                               */
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

#include "scene_editor_dock.h"

#include "core/object/callable_mp.h"
#include "editor/editor_node.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/scene/editor_scene_tabs.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"

SceneEditorDock::SceneEditorDock() {
	set_available_layouts(DOCK_LAYOUT_ALL);
	set_default_slot(CENTER_TOP);
	set_icon_name("dock_scene_editor");
	set_name(TTRC("Scene Editor"));

	VBoxContainer *content_vbox = memnew(VBoxContainer);
	content_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(content_vbox);
	{
		HBoxContainer *scene_tabs_hbox = memnew(HBoxContainer);
		content_vbox->add_child(scene_tabs_hbox);
		{
			_viewport_mode_btn = memnew(Button);
			_viewport_mode_btn->set_toggle_mode(true);
			scene_tabs_hbox->add_child(_viewport_mode_btn);
			_viewport_mode_btn->connect(SceneStringName(toggled), callable_mp(this, &SceneEditorDock::set_3d_mode));

			_scene_tabs = memnew(EditorSceneTabs);
			_scene_tabs->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			scene_tabs_hbox->add_child(_scene_tabs);
		}

		_viewport_mc = memnew(MarginContainer);
		_viewport_mc->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		content_vbox->add_child(_viewport_mc);
	}
}

void SceneEditorDock::set_2d_editor(CanvasItemEditor *p_editor) {
	ERR_FAIL_NULL(p_editor);
	ERR_FAIL_COND(_editor_2d != nullptr);
	_editor_2d = p_editor;
	_viewport_mc->add_child(_editor_2d);
	_update_editor_visibility();
}

void SceneEditorDock::set_3d_editor(Node3DEditor *p_editor) {
	ERR_FAIL_NULL(p_editor);
	ERR_FAIL_COND(_editor_3d != nullptr);
	_editor_3d = p_editor;
	_viewport_mc->add_child(_editor_3d);
	_update_editor_visibility();
}

void SceneEditorDock::set_3d_mode(bool p_enabled) {
	const bool new_mode = p_enabled && _3d_enabled;
	const bool changed = _3d_mode != new_mode;
	_3d_mode = new_mode;
	_update_editor_visibility();
	if (changed && is_inside_tree()) {
		EditorNode::get_singleton()->save_editor_layout_delayed();
	}
}

void SceneEditorDock::set_3d_enabled(bool p_enabled) {
	_3d_enabled = p_enabled;
	_viewport_mode_btn->set_disabled(!p_enabled);
	set_3d_mode(_3d_mode);
}

void SceneEditorDock::save_layout_to_config(Ref<ConfigFile> &r_layout, const String &p_section) const {
	r_layout->set_value(p_section, "3d_mode", _3d_mode);
}

void SceneEditorDock::load_layout_from_config(const Ref<ConfigFile> &p_layout, const String &p_section) {
	set_3d_mode(p_layout->get_value(p_section, "3d_mode", true));
}

void SceneEditorDock::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			_update_editor_visibility();
		} break;
	}
}

void SceneEditorDock::_update_editor_visibility() {
	if (is_inside_tree()) {
		_viewport_mode_btn->set_button_icon(get_editor_theme_icon(_3d_mode ? SNAME("gui_3d") : SNAME("gui_2d")));
	}
	_viewport_mode_btn->set_pressed_no_signal(_3d_mode);
	_viewport_mode_btn->set_tooltip_text(_3d_mode ? TTR("Switch to 2D viewport.") : TTR("Switch to 3D viewport."));
	if (_editor_2d) {
		_editor_2d->set_visible(!_3d_mode);
	}
	if (_editor_3d) {
		_editor_3d->set_visible(_3d_mode);
	}
}
