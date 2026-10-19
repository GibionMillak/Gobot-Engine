/**************************************************************************/
/*  script_create_dialog.cpp                                              */
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

#include "script_create_dialog.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "editor/editor_node.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/gui/create_dialog.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_validation_panel.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/line_edit.h"

void ScriptCreateDialog::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			path_button->set_button_icon(get_editor_theme_icon(SNAME("Folder")));
			parent_browse_button->set_button_icon(get_editor_theme_icon(SNAME("Folder")));
			parent_search_button->set_button_icon(get_editor_theme_icon(SNAME("ClassList")));
		} break;
	}
}

void ScriptCreateDialog::_path_hbox_sorted() {
	if (is_visible()) {
		int filename_start_pos = file_path->get_text().rfind_char('/') + 1;
		int filename_end_pos = file_path->get_text().get_basename().length();

		if (!is_built_in) {
			file_path->select(filename_start_pos, filename_end_pos);
		}

		// First set cursor to the end of line to scroll LineEdit view
		// to the right and then set the actual cursor position.
		file_path->set_caret_column(file_path->get_text().length());
		file_path->set_caret_column(filename_start_pos);

		file_path->grab_focus();
	}
}

bool ScriptCreateDialog::_can_be_built_in() {
	return (supports_built_in && built_in_enabled);
}

String ScriptCreateDialog::_adjust_file_path(const String &p_base_path) const {
	if (p_base_path.is_empty() || _script_language == nullptr) {
		return p_base_path;
	}

	String base_dir = p_base_path.get_base_dir();
	String file_name = p_base_path.get_file().get_basename();
	file_name = EditorNode::adjust_script_name_casing(file_name);
	String extension = _script_language->get_extension();
	return base_dir.path_join(file_name + "." + extension);
}

void ScriptCreateDialog::config(const String &p_base_name, const String &p_base_path, bool p_built_in_enabled, bool p_load_enabled) {
	parent_name->set_text(p_base_name);
	parent_name->deselect();
	built_in_name->set_text("");

	file_path->set_text(p_base_path);
	file_path->deselect();

	built_in_enabled = p_built_in_enabled;
	load_enabled = p_load_enabled;

	_update_script_options();

	if (_can_be_built_in()) {
		built_in->set_pressed(EditorSettings::get_singleton()->get_project_metadata("script_setup", "create_built_in_script", false));
		_built_in_pressed();
	}
}

void ScriptCreateDialog::set_inheritance_base_type(const String &p_base) {
	base_type = p_base;
}

bool ScriptCreateDialog::_validate_parent(const String &p_string) {
	if (p_string.length() == 0) {
		return false;
	}

	if (can_inherit_from_file && p_string.is_quoted()) {
		String p = p_string.substr(1, p_string.length() - 2);
		if (_validate_path(p, true).is_empty()) {
			return true;
		}
	}

	return EditorNode::get_editor_data().is_type_recognized(p_string);
}

String ScriptCreateDialog::_validate_path(const String &p_path, bool p_file_must_exist, bool *r_path_valid) {
	String p = p_path.strip_edges();
	if (r_path_valid) {
		*r_path_valid = false;
	}

	if (p.is_empty()) {
		return TTRC("Path is empty.");
	}
	if (p.get_file().get_basename().is_empty()) {
		return TTRC("Filename is empty.");
	}

	if (!p.get_file().get_basename().is_valid_filename()) {
		return TTRC("Filename is invalid.");
	}
	if (p.get_file().begins_with(".")) {
		return TTRC("Name begins with a dot.");
	}

	p = ProjectSettings::get_singleton()->localize_path(p);
	if (!p.begins_with("res://")) {
		return TTRC("Path is not local.");
	}

	{
		missing_base_dir = false;
		Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
		if (da->change_dir(p.get_base_dir()) != OK) {
			missing_base_dir = true;
		}
	}

	{
		// Check if file exists.
		Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
		if (da->dir_exists(p)) {
			return TTRC("A directory with the same name exists.");
		} else if (p_file_must_exist && !da->file_exists(p)) {
			return TTRC("File does not exist.");
		}
	}

	if (r_path_valid) {
		*r_path_valid = true;
	}

	if (_script_language == nullptr) {
		return TTRC("GDScript is unavailable in this build.");
	}
	if (!p.has_extension(_script_language->get_extension())) {
		return TTRC("Invalid extension.");
	}
	return _script_language->validate_path(p);
}

void ScriptCreateDialog::_parent_name_changed(const String &p_parent) {
	is_parent_name_valid = _validate_parent(parent_name->get_text());
	validation_panel->update();
}

void ScriptCreateDialog::_on_confirmed() {
	if (missing_base_dir) {
		String path = file_path->get_text();
		Error err = EditorFileSystem::get_singleton()->make_dir_recursive(path.strip_edges().get_base_dir());
		if (err != OK) {
			alert->set_text(TTR("Error - Could not create the directory for the script."));
			alert->popup_centered();
			return;
		}
	}

	if (is_new_script_created) {
		_create_new();
		if (_can_be_built_in()) {
			// Only save state of built-in checkbox if it's enabled.
			EditorSettings::get_singleton()->set_project_metadata("script_setup", "create_built_in_script", built_in->is_pressed());
		}
	} else {
		_load_exist();
	}

	EditorSettings::get_singleton()->save();
	is_new_script_created = true;
	validation_panel->update();
}

void ScriptCreateDialog::_create_new() {
	String parent_class = parent_name->get_text();
	if (!parent_name->get_text().is_quoted() && !ClassDB::class_exists(parent_class) && !ScriptServer::is_global_class(parent_class)) {
		// If base is a custom type, replace with script path instead.
		const EditorData::CustomType *type = EditorNode::get_editor_data().get_custom_type_by_name(parent_class);
		ERR_FAIL_NULL(type);
		parent_class = "\"" + type->script->get_path() + "\"";
	}

	Ref<Script> scr = _script_language->make_script(parent_class);
	ERR_FAIL_COND(scr.is_null());

	if (is_built_in) {
		scr->set_name(built_in_name->get_text());
		// Make sure the script is compiled to make its type recognizable.
		scr->reload();
	} else {
		String lpath = ProjectSettings::get_singleton()->localize_path(file_path->get_text());
		scr->set_path(lpath);
		Error err = ResourceSaver::save(scr, lpath, ResourceSaver::FLAG_CHANGE_PATH);
		if (err != OK) {
			alert->set_text(TTR("Error - Could not create script in filesystem."));
			alert->popup_centered();
			return;
		}
	}

	emit_signal(SNAME("script_created"), scr);
	hide();
}

void ScriptCreateDialog::_load_exist() {
	String path = file_path->get_text();
	Ref<Resource> p_script = ResourceLoader::load(path, "Script");
	if (p_script.is_null()) {
		alert->set_text(vformat(TTR("Error loading script from %s"), path));
		alert->popup_centered();
		return;
	}

	emit_signal(SNAME("script_created"), p_script);
	hide();
}

void ScriptCreateDialog::_update_script_options() {
	if (_script_language == nullptr) {
		return;
	}

	can_inherit_from_file = _script_language->can_inherit_from_file();
	supports_built_in = _script_language->supports_builtin_mode();
	if (!supports_built_in) {
		is_built_in = false;
	}

	String path = file_path->get_text();
	path = _adjust_file_path(path);
	_path_changed(path);
	file_path->set_text(path);

	_parent_name_changed(parent_name->get_text());
	validation_panel->update();
}

void ScriptCreateDialog::_built_in_pressed() {
	if (built_in->is_pressed()) {
		is_built_in = true;
		is_new_script_created = true;
	} else {
		is_built_in = false;
		_path_changed(file_path->get_text());
	}
	validation_panel->update();
}

void ScriptCreateDialog::_browse_path(bool browse_parent, bool p_save) {
	is_browsing_parent = browse_parent;

	if (p_save) {
		file_browse->set_file_mode(EditorFileDialog::FILE_MODE_SAVE_FILE);
		file_browse->set_title(TTR("Open Script / Choose Location"));
		file_browse->set_ok_button_text(TTR("Open"));
	} else {
		file_browse->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILE);
		file_browse->set_title(TTR("Open Script"));
	}

	file_browse->set_customization_flag_enabled(FileDialog::CUSTOMIZATION_OVERWRITE_WARNING, false);

	file_browse->clear_filters();
	file_browse->add_filter("*." + _script_language->get_extension());

	file_browse->set_current_path(file_path->get_text());
	file_browse->popup_file_dialog();
}

void ScriptCreateDialog::_file_selected(const String &p_file) {
	String path = ProjectSettings::get_singleton()->localize_path(p_file);
	if (is_browsing_parent) {
		parent_name->set_text("\"" + path + "\"");
		_parent_name_changed(parent_name->get_text());
	} else {
		file_path->set_text(path);
		_path_changed(path);

		String filename = path.get_file().get_basename();
		int select_start = path.rfind(filename);
		file_path->select(select_start, select_start + filename.length());
		file_path->set_caret_column(select_start + filename.length());
		file_path->grab_focus();
	}
}

void ScriptCreateDialog::_create() {
	parent_name->set_text(select_class->get_selected_type_name());
	_parent_name_changed(parent_name->get_text());
}

void ScriptCreateDialog::_browse_class_in_tree() {
	select_class->set_base_type(base_type);
	select_class->popup_create(true);
	select_class->set_title(vformat(TTR("Inherit %s"), base_type));
	select_class->set_ok_button_text(TTR("Inherit"));
}

void ScriptCreateDialog::_path_changed(const String &p_path) {
	if (is_built_in) {
		return;
	}

	is_new_script_created = true;

	path_error = _validate_path(p_path, false, &is_path_valid);
	if (!path_error.is_empty()) {
		validation_panel->update();
		return;
	}

	// Check if file exists.
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	String p = ProjectSettings::get_singleton()->localize_path(p_path.strip_edges());
	if (da->file_exists(p)) {
		is_new_script_created = false;
	}
	validation_panel->update();
}

void ScriptCreateDialog::_update_dialog() {
	// "Add Script Dialog" GUI logic and script checks.

	// Is script path/name valid (order from top to bottom)?

	if (missing_base_dir) {
		validation_panel->set_message(MSG_ID_SCRIPT, TTRC("Base path is invalid, the target folder will be created automatically."), EditorValidationPanel::MSG_WARNING);
	}

	if (!is_built_in && !is_path_valid) {
		validation_panel->set_message(MSG_ID_SCRIPT, TTRC("Invalid path."), EditorValidationPanel::MSG_ERROR);
	}

	if (!is_parent_name_valid && is_new_script_created) {
		validation_panel->set_message(MSG_ID_SCRIPT, TTRC("Invalid inherited parent name or path."), EditorValidationPanel::MSG_ERROR);
	}

	if (validation_panel->is_valid() && !is_new_script_created) {
		validation_panel->set_message(MSG_ID_SCRIPT, TTRC("File exists, it will be reused."), EditorValidationPanel::MSG_OK);
	}

	if (!is_built_in && !path_error.is_empty()) {
		validation_panel->set_message(MSG_ID_PATH, path_error, EditorValidationPanel::MSG_ERROR);
	}

	// Is script Built-in?

	if (is_built_in) {
		file_path->set_editable(false);
		path_button->set_disabled(true);
		re_check_path = true;
	} else {
		file_path->set_editable(true);
		path_button->set_disabled(false);
		if (re_check_path) {
			re_check_path = false;
			_path_changed(file_path->get_text());
		}
	}

	if (!_can_be_built_in()) {
		built_in->set_pressed(false);
	}
	built_in->set_disabled(!_can_be_built_in());

	// Is Script created or loaded from existing file?

	if (is_built_in) {
		validation_panel->set_message(MSG_ID_BUILT_IN, TTRC("Note: Built-in scripts have some limitations and can't be edited using an external editor."), EditorValidationPanel::MSG_INFO, false);
	} else if (file_path->get_text().get_file().get_basename() == parent_name->get_text()) {
		validation_panel->set_message(MSG_ID_BUILT_IN, TTRC("Warning: Having the script name be the same as a built-in type is usually not desired."), EditorValidationPanel::MSG_WARNING, false);
	}

	path_controls[0]->set_visible(!is_built_in);
	path_controls[1]->set_visible(!is_built_in);
	name_controls[0]->set_visible(is_built_in);
	name_controls[1]->set_visible(is_built_in);

	bool is_new_file = is_built_in || is_new_script_created;

	parent_name->set_editable(is_new_file);
	parent_search_button->set_disabled(!is_new_file);
	parent_browse_button->set_disabled(!is_new_file || !can_inherit_from_file);
	String button_text = is_new_file ? TTR("Create") : TTR("Load");
	set_ok_button_text(button_text);

	if (is_new_file) {
		if (is_built_in) {
			validation_panel->set_message(MSG_ID_PATH, TTRC("Built-in script (into scene file)."), EditorValidationPanel::MSG_OK);
		}
	} else {
		if (load_enabled) {
			if (is_path_valid) {
				validation_panel->set_message(MSG_ID_PATH, TTRC("Will load an existing script file."), EditorValidationPanel::MSG_OK);
			}
		} else {
			validation_panel->set_message(MSG_ID_PATH, TTRC("Script file already exists."), EditorValidationPanel::MSG_ERROR);
		}
	}
}

void ScriptCreateDialog::_bind_methods() {
	ClassDB::bind_method(D_METHOD("config", "inherits", "path", "built_in_enabled", "load_enabled"), &ScriptCreateDialog::config, DEFVAL(true), DEFVAL(true));

	ADD_SIGNAL(MethodInfo("script_created", PropertyInfo(Variant::OBJECT, "script", PROPERTY_HINT_RESOURCE_TYPE, Script::get_class_static())));
}

ScriptCreateDialog::ScriptCreateDialog() :
		Dialog(true) {
	/* Main Controls */

	GridContainer *gc = memnew(GridContainer);
	gc->set_columns(2);

	/* Information Messages Field */

	validation_panel = memnew(EditorValidationPanel);
	validation_panel->add_line(MSG_ID_SCRIPT, TTRC("Script path/name is valid."));
	validation_panel->add_line(MSG_ID_PATH, TTRC("Will create a new script file."));
	validation_panel->add_line(MSG_ID_BUILT_IN);
	validation_panel->set_update_callback(callable_mp(this, &ScriptCreateDialog::_update_dialog));
	validation_panel->set_accept_button(get_ok_button());

	/* Spacing */

	Control *spacing = memnew(Control);
	spacing->set_custom_minimum_size(Size2(0, 10 * EDSCALE));

	VBoxContainer *vb = memnew(VBoxContainer);
	vb->add_child(gc);
	vb->add_child(spacing);
	vb->add_child(validation_panel);
	add_child(vb);

	for (int i = 0; i < ScriptServer::get_language_count(); i++) {
		if (ScriptServer::get_language(i)->get_name() == "GDScript") {
			_script_language = ScriptServer::get_language(i);
			break;
		}
	}
	if (_script_language == nullptr) {
		get_ok_button()->set_disabled(true);
	}

	/* Inherits */

	base_type = "Object";

	HBoxContainer *hb = memnew(HBoxContainer);
	hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	parent_name = memnew(LineEdit);
	parent_name->connect(SceneStringName(text_changed), callable_mp(this, &ScriptCreateDialog::_parent_name_changed));
	parent_name->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hb->add_child(parent_name);
	register_text_enter(parent_name);
	parent_search_button = memnew(Button);
	parent_search_button->connect(SceneStringName(pressed), callable_mp(this, &ScriptCreateDialog::_browse_class_in_tree));
	hb->add_child(parent_search_button);
	parent_browse_button = memnew(Button);
	parent_browse_button->connect(SceneStringName(pressed), callable_mp(this, &ScriptCreateDialog::_browse_path).bind(true, false));
	hb->add_child(parent_browse_button);
	gc->add_child(memnew(Label(TTR("Inherits:"))));
	gc->add_child(hb);

	/* Built-in Script */

	built_in = memnew(CheckBox);
	built_in->set_text(TTR("On"));
	built_in->connect(SceneStringName(pressed), callable_mp(this, &ScriptCreateDialog::_built_in_pressed));
	gc->add_child(memnew(Label(TTR("Built-in Script:"))));
	gc->add_child(built_in);

	/* Path */

	hb = memnew(HBoxContainer);
	hb->connect(SceneStringName(sort_children), callable_mp(this, &ScriptCreateDialog::_path_hbox_sorted));
	file_path = memnew(LineEdit);
	file_path->connect(SceneStringName(text_changed), callable_mp(this, &ScriptCreateDialog::_path_changed));
	file_path->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hb->add_child(file_path);
	register_text_enter(file_path);
	path_button = memnew(Button);
	path_button->connect(SceneStringName(pressed), callable_mp(this, &ScriptCreateDialog::_browse_path).bind(false, true));
	hb->add_child(path_button);
	Label *label = memnew(Label(TTR("Path:")));
	gc->add_child(label);
	gc->add_child(hb);
	path_controls[0] = label;
	path_controls[1] = hb;

	/* Name */

	built_in_name = memnew(LineEdit);
	built_in_name->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	register_text_enter(built_in_name);
	label = memnew(Label(TTR("Name:")));
	gc->add_child(label);
	gc->add_child(built_in_name);
	name_controls[0] = label;
	name_controls[1] = built_in_name;
	label->hide();
	built_in_name->hide();

	/* Dialog Setup */

	select_class = memnew(CreateDialog);
	select_class->connect("create", callable_mp(this, &ScriptCreateDialog::_create));
	select_class->for_inherit();
	add_child(select_class);

	file_browse = memnew(EditorFileDialog);
	file_browse->connect("file_selected", callable_mp(this, &ScriptCreateDialog::_file_selected));
	file_browse->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILE);
	add_child(file_browse);
	set_ok_button_text(TTR("Create"));
	alert = memnew(Dialog);
	alert->set_flag(Window::FLAG_RESIZE_DISABLED, true);
	alert->get_label()->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	alert->get_label()->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	alert->get_label()->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
	alert->get_label()->set_custom_minimum_size(Size2(325, 60) * EDSCALE);
	add_child(alert);

	set_hide_on_ok(false);
	set_title(TTR("Attach Node Script"));
}
