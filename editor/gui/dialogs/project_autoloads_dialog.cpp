/**************************************************************************/
/*  project_autoloads_dialog.cpp                                          */
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

#include "project_autoloads_dialog.h"

#include "core/error/error_macros.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/callable_mp.h"
#include "editor/editor_node.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_validation_panel.h"
#include "editor/managers/editor_managers.h"
#include "editor/managers/project_autoload_manager.h"
#include "editor/scene/scene_create_dialog.h"
#include "editor/script/script_create_dialog.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/tree.h"
#include "scene/resources/packed_scene.h"

ProjectAutoloadsDialog::ProjectAutoloadsDialog() {
	_manager = EditorManagers::get_singleton()->get_project_autoload_manager();

	set_flag(Window::FLAG_RESIZE_DISABLED, false);
	set_ok_button_text(TTRC("Close"));
	set_title(TTRC("Project Autoloads"));

	VBoxContainer *content_vbox = memnew(VBoxContainer);
	add_child(content_vbox);
	{
		HBoxContainer *actions_hbox = memnew(HBoxContainer);
		content_vbox->add_child(actions_hbox);
		{
			_autoload_file_dialog = memnew(EditorFileDialog);
			_autoload_file_dialog->connect("file_selected", callable_mp(this, &ProjectAutoloadsDialog::_autoload_file_selected));
			_autoload_file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
			actions_hbox->add_child(_autoload_file_dialog);

			_scene_file_dialog = memnew(EditorFileDialog);
			_scene_file_dialog->connect("file_selected", callable_mp(this, &ProjectAutoloadsDialog::_scene_file_selected));
			_scene_file_dialog->set_file_mode(FileDialog::FILE_MODE_SAVE_FILE);
			actions_hbox->add_child(_scene_file_dialog);

			_browse_btn = memnew(Button);
			_browse_btn->connect(SceneStringName(pressed), callable_mp(_autoload_file_dialog, &FileDialog::popup_file_dialog));
			_browse_btn->set_text(TTRC("Select Script/Scene"));
			actions_hbox->add_child(_browse_btn);

			_create_script_autoload_btn = memnew(Button);
			_create_script_autoload_btn->connect(SceneStringName(pressed), callable_mp(this, &ProjectAutoloadsDialog::_create_script_autoload));
			_create_script_autoload_btn->set_text(TTRC("Create Script"));
			actions_hbox->add_child(_create_script_autoload_btn);

			_create_scene_autoload_btn = memnew(Button);
			_create_scene_autoload_btn->connect(SceneStringName(pressed), callable_mp(this, &ProjectAutoloadsDialog::_create_scene_autoload));
			_create_scene_autoload_btn->set_text(TTRC("Create Scene"));
			actions_hbox->add_child(_create_scene_autoload_btn);
		}

		MarginContainer *list_margin = memnew(MarginContainer);
		list_margin->set_theme_type_variation("NoBorderBottomWideWindow");
		list_margin->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		content_vbox->add_child(list_margin);
		{
			_tree = memnew(Tree);
			_tree->set_columns(4);
			_tree->connect("cell_selected", callable_mp(this, &ProjectAutoloadsDialog::_autoload_selected));
			_tree->connect("item_edited", callable_mp(this, &ProjectAutoloadsDialog::_autoload_edited));
			_tree->connect("button_clicked", callable_mp(this, &ProjectAutoloadsDialog::_autoload_button_pressed));
			_tree->connect("item_activated", callable_mp(this, &ProjectAutoloadsDialog::_autoload_activated));
			_tree->set_allow_reselect(true);
			_tree->set_column_clip_content(1, true);
			_tree->set_column_expand(0, true);
			_tree->set_column_expand(1, true);
			_tree->set_column_expand(2, false);
			_tree->set_column_expand(3, false);
			_tree->set_column_expand_ratio(0, 1);
			_tree->set_column_expand_ratio(1, 2);
			_tree->set_column_title(0, TTRC("Name"));
			_tree->set_column_title(1, TTRC("Path"));
			_tree->set_column_title(2, TTRC("Global Variable"));
			_tree->set_column_title_alignment(0, HORIZONTAL_ALIGNMENT_LEFT);
			_tree->set_column_title_alignment(1, HORIZONTAL_ALIGNMENT_LEFT);
			_tree->set_column_title_tooltip_text(0, TTRC("Name of the autoload. Double-click to rename."));
			_tree->set_column_title_tooltip_text(1, TTRC("Path to the autoload file. Double-click to open it."));
			_tree->set_column_title_tooltip_text(2, TTRC("If enabled, this autoload can be accessed in scripts using its name."));
			_tree->set_column_titles_visible(true);
			_tree->set_hide_folding(true);
			_tree->set_hide_root(true);
			_tree->set_select_mode(Tree::SELECT_MULTI);
			_tree->set_theme_type_variation("TreeTable");
			SET_DRAG_FORWARDING_GCDU(_tree, ProjectAutoloadsDialog);
			list_margin->add_child(_tree, true);
		}
	}

	_name_dialog = memnew(Dialog(true));
	_name_dialog->connect(SceneStringName(confirmed), callable_mp(this, &ProjectAutoloadsDialog::_confirm_autoload_name));
	_name_dialog->set_title(TTRC("Enter Autoload Name"));
	add_child(_name_dialog);
	{
		VBoxContainer *name_vbox = memnew(VBoxContainer);
		_name_dialog->add_child(name_vbox);
		{
			_name_edit = memnew(LineEdit);
			name_vbox->add_child(_name_edit);
			_name_dialog->register_text_enter(_name_edit);

			_name_validator = memnew(EditorValidationPanel);
			_name_validator->add_line(0, TTRC("Autoload name is valid."));
			_name_validator->set_accept_button(_name_dialog->get_ok_button());
			_name_validator->set_update_callback(callable_mp(this, &ProjectAutoloadsDialog::_validate_autoload_name));
			name_vbox->add_child(_name_validator);
			_name_edit->connect(SceneStringName(text_changed), callable_mp(_name_validator, &EditorValidationPanel::update).unbind(1));
		}
	}

	_manager->connect("autoloads_changed", callable_mp(this, &ProjectAutoloadsDialog::_refresh_tree), CONNECT_DEFERRED);
}

void ProjectAutoloadsDialog::popup_autoloads() {
	_manager->refresh_autoloads();
	_refresh_tree();
	popup_centered_clamped(Size2i(900, 600) * EDSCALE);
}

void ProjectAutoloadsDialog::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			List<String> afn;
			ResourceLoader::get_recognized_extensions_for_type("PackedScene", &afn);

			for (const String &E : afn) {
				_scene_file_dialog->add_filter("*." + E);
			}

			ResourceLoader::get_recognized_extensions_for_type("Script", &afn);

			for (const String &E : afn) {
				_autoload_file_dialog->add_filter("*." + E);
			}
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			_browse_btn->set_button_icon(get_editor_theme_icon(SNAME("file_browse")));
			_create_script_autoload_btn->set_button_icon(get_editor_theme_icon(SNAME("action_add")));
			_create_scene_autoload_btn->set_button_icon(get_editor_theme_icon(SNAME("action_add")));
		} break;
	}
}

void ProjectAutoloadsDialog::_validate_autoload_name() {
	String error;
	bool is_valid = _manager->is_name_valid(_name_edit->get_text(), &error);
	if (!is_valid) {
		_name_validator->set_message(0, error, EditorValidationPanel::MSG_ERROR);
	}
}

void ProjectAutoloadsDialog::_refresh_tree() {
	_updating_tree = true;
	_tree->clear();
	TreeItem *root = _tree->create_item();
	for (const ProjectAutoloadManager::AutoloadEntry &entry : _manager->get_autoloads()) {
		TreeItem *item = _tree->create_item(root);
		item->set_text(0, entry.name);
		item->set_auto_translate_mode(0, AUTO_TRANSLATE_MODE_DISABLED);
		item->set_editable(0, true);
		item->set_text(1, entry.path);
		item->set_selectable(1, true);
		item->set_cell_mode(2, TreeItem::CELL_MODE_CHECK);
		item->set_editable(2, true);
		item->set_text(2, TTRC("Enable"));
		item->set_checked(2, entry.is_singleton);
		item->add_button(3, get_editor_theme_icon(SNAME("action_move_up")), MOVE_UP);
		item->add_button(3, get_editor_theme_icon(SNAME("action_move_down")), MOVE_DOWN);
		item->add_button(3, get_editor_theme_icon(SNAME("action_remove")), DELETE);
		item->set_selectable(3, false);
		if (entry.name == _selected_autoload) {
			item->select(0);
		}
	}
	_updating_tree = false;
}

void ProjectAutoloadsDialog::_autoload_selected() {
	TreeItem *ti = _tree->get_selected();

	if (!ti) {
		return;
	}

	_selected_autoload = ti->get_text(0);
}

void ProjectAutoloadsDialog::_autoload_edited() {
	if (_updating_tree) {
		return;
	}
	TreeItem *item = _tree->get_edited();
	ERR_FAIL_NULL(item);
	const int column = _tree->get_edited_column();
	if (column == 0) {
		const String new_name = item->get_text(0);
		String error;
		if (!_manager->rename_autoload(_selected_autoload, new_name, &error)) {
			item->set_text(0, _selected_autoload);
			EditorNode::get_singleton()->show_warning(error);
			return;
		}
		_selected_autoload = new_name;
	} else if (column == 2) {
		_manager->set_autoload_singleton(item->get_text(0), item->is_checked(2));
	}
}

void ProjectAutoloadsDialog::_autoload_button_pressed(Object *p_item, int p_column, int p_button, MouseButton p_mouse_button) {
	if (p_mouse_button != MouseButton::LEFT) {
		return;
	}
	TreeItem *item = Object::cast_to<TreeItem>(p_item);
	ERR_FAIL_NULL(item);
	const String name = item->get_text(0);
	switch (p_button) {
		case MOVE_UP:
		case MOVE_DOWN: {
			TreeItem *other = p_button == MOVE_UP ? item->get_prev() : item->get_next();
			if (other) {
				_manager->swap_autoloads(name, other->get_text(0));
			}
		} break;
		case DELETE: {
			_manager->remove_autoload(name, true);
		} break;
	}
}

void ProjectAutoloadsDialog::_autoload_activated() {
	TreeItem *ti = _tree->get_selected();
	if (!ti) {
		return;
	}
	_autoload_open(ti->get_text(1));
}

void ProjectAutoloadsDialog::_autoload_open(const String &p_path) {
	EditorNode::get_singleton()->load_scene_or_resource(p_path);
	hide();
}

void ProjectAutoloadsDialog::_create_script_autoload() {
	if (!_script_create_dialog) {
		_script_create_dialog = memnew(ScriptCreateDialog);
		_script_create_dialog->connect("script_created", callable_mp(this, &ProjectAutoloadsDialog::_script_created));
		add_child(_script_create_dialog);
	}
	_script_create_dialog->config("Node", "res://new_autoload_script.gd", false, false);
	_script_create_dialog->popup_centered();
}

void ProjectAutoloadsDialog::_create_scene_autoload() {
	_scene_file_dialog->set_current_file("new_autoload_scene.tscn");
	_scene_file_dialog->popup_file_dialog();
}

void ProjectAutoloadsDialog::_autoload_file_selected(const String &p_path) {
	// Convert the file name to PascalCase, which is the convention for classes in GDScript.
	_try_add_autoload(p_path.get_file().get_basename().to_pascal_case(), p_path);
}

void ProjectAutoloadsDialog::_scene_file_selected(const String &p_path) {
	if (!_scene_create_dialog) {
		_scene_create_dialog = memnew(SceneCreateDialog);
		_scene_create_dialog->connect(SceneStringName(confirmed), callable_mp(this, &ProjectAutoloadsDialog::_scene_created));
		add_child(_scene_create_dialog);
	}
	_scene_create_dialog->config(p_path.get_base_dir(), p_path.get_file().get_basename());
	_scene_create_dialog->popup_centered();
}

void ProjectAutoloadsDialog::_script_created(Ref<Script> p_script) {
	_script_create_dialog->hide();
	_try_add_autoload(p_script->get_path().get_file().get_basename().to_pascal_case(), p_script->get_path());
}

void ProjectAutoloadsDialog::_scene_created() {
	Node *root = _scene_create_dialog->create_scene_root();

	Ref<PackedScene> ps;
	ps.instantiate();
	ps->pack(root);

	Error err = ResourceSaver::save(ps, _scene_create_dialog->get_scene_path());
	if (err != OK) {
		EditorNode::get_singleton()->show_warning(vformat(TTR("Failed to create scene. Error: %d."), err));
		return;
	}

	_try_add_autoload(_scene_create_dialog->get_root_name().to_pascal_case(), _scene_create_dialog->get_scene_path());
}

void ProjectAutoloadsDialog::_add_autoload(const String &p_name, const String &p_path) {
	String error;
	if (!_manager->add_autoload(p_name, p_path, true, &error)) {
		EditorNode::get_singleton()->show_warning(error);
	}
}

void ProjectAutoloadsDialog::_try_add_autoload(const String &p_name, const String &p_path) {
	if (_manager->is_name_valid(p_name)) {
		_add_autoload(p_name, p_path);
		return;
	}
	_pending_autoload_path = p_path;

	_name_edit->set_text(p_name);
	_name_validator->update();
	_name_dialog->popup_centered(Vector2i(600 * EDSCALE, 0));
	_name_edit->grab_focus();
	_name_edit->select_all();
}

void ProjectAutoloadsDialog::_confirm_autoload_name() {
	_add_autoload(_name_edit->get_text(), _pending_autoload_path);
}

Variant ProjectAutoloadsDialog::_get_drag_data_fw(const Point2 &p_point, Control *p_control) {
	const int autoload_count = _manager->get_autoloads().size();
	if (autoload_count <= 1) {
		return false;
	}

	PackedStringArray autoloads;

	TreeItem *next = _tree->get_next_selected(nullptr);

	while (next) {
		autoloads.push_back(next->get_text(0));
		next = _tree->get_next_selected(next);
	}

	if (autoloads.is_empty() || autoloads.size() == autoload_count) {
		return Variant();
	}

	VBoxContainer *preview_vbox = memnew(VBoxContainer);

	{
		const int preview_count = MIN(PREVIEW_LIST_MAX_SIZE, autoloads.size());
		for (int i = 0; i < preview_count; i++) {
			Label *preview_lbl = memnew(Label(autoloads[i]));
			preview_lbl->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
			preview_lbl->set_self_modulate(Color(1, 1, 1, Math::lerp(1, 0, float(i) / PREVIEW_LIST_MAX_SIZE)));
			preview_vbox->add_child(preview_lbl);
		}
	}

	_tree->set_drop_mode_flags(Tree::DROP_MODE_INBETWEEN);
	_tree->set_drag_preview(preview_vbox);

	Dictionary drop_data;
	drop_data["type"] = "autoload";
	drop_data["autoloads"] = autoloads;

	return drop_data;
}

bool ProjectAutoloadsDialog::_can_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_control) const {
	if (_updating_tree) {
		return false;
	}
	const Dictionary drop_data = p_data;
	if (!drop_data.has("type") || String(drop_data["type"]) != "autoload" || !drop_data.has("autoloads")) {
		return false;
	}
	TreeItem *item = p_point == Vector2(Math::INF, Math::INF) ? _tree->get_selected() : _tree->get_item_at_position(p_point);
	if (!item) {
		return false;
	}
	const int section = p_point == Vector2(Math::INF, Math::INF) ? _tree->get_drop_section_at_position(_tree->get_item_rect(item).position) : _tree->get_drop_section_at_position(p_point);
	return section >= -1;
}

void ProjectAutoloadsDialog::_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_control) {
	if (!_can_drop_data_fw(p_point, p_data, p_control)) {
		return;
	}
	TreeItem *item = p_point == Vector2(Math::INF, Math::INF) ? _tree->get_selected() : _tree->get_item_at_position(p_point);
	const int section = p_point == Vector2(Math::INF, Math::INF) ? _tree->get_drop_section_at_position(_tree->get_item_rect(item).position) : _tree->get_drop_section_at_position(p_point);
	String before_name;
	if (section < 0) {
		before_name = item->get_text(0);
	} else if (item->get_next()) {
		before_name = item->get_next()->get_text(0);
	}
	const Dictionary drop_data = p_data;
	_manager->reorder_autoloads(drop_data["autoloads"], before_name);
}
