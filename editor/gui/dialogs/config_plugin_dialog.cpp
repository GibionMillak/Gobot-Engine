/**************************************************************************/
/*  config_plugin_dialog.cpp                                              */
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

#include "config_plugin_dialog.h"

#include "core/io/config_file.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_saver.h"
#include "core/object/callable_mp.h"
#include "core/object/script_language.h"
#include "editor/editor_node.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/gui/editor_validation_panel.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/text_edit.h"
#include "scene/scene_string_names.h"

#include "modules/modules_enabled.gen.h"

#ifdef MODULE_GDSCRIPT_ENABLED
#include "modules/gdscript/gdscript.h"
#endif

ConfigPluginDialog::ConfigPluginDialog() {
	get_ok_button()->set_disabled(true);
	set_hide_on_ok(false);

	VBoxContainer *content_vbox = memnew(VBoxContainer);
	content_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	content_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(content_vbox);
	{
		GridContainer *fields_grid = memnew(GridContainer);
		fields_grid->set_columns(2);
		fields_grid->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		content_vbox->add_child(fields_grid);
		{
			Label *name_lbl = memnew(Label);
			name_lbl->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
			name_lbl->set_text(TTR("Plugin Name:"));
			fields_grid->add_child(name_lbl);

			_name_edit = memnew(LineEdit);
			_name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			_name_edit->set_placeholder("MyPlugin");
			_name_edit->set_tooltip_text(TTR("Required. This name will be displayed in the list of plugins."));
			fields_grid->add_child(_name_edit);

			_subfolder_lbl = memnew(Label);
			_subfolder_lbl->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
			_subfolder_lbl->set_text(TTR("Subfolder:"));
			fields_grid->add_child(_subfolder_lbl);

			_subfolder_edit = memnew(LineEdit);
			_subfolder_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			_subfolder_edit->set_placeholder(U"\"my_plugin\" → res://addons/my_plugin");
			_subfolder_edit->set_tooltip_text(TTR("Optional. The folder name should generally use `snake_case` naming (avoid spaces and special characters).\nIf left empty, the folder will be named after the plugin name converted to `snake_case`."));
			fields_grid->add_child(_subfolder_edit);

			Label *description_lbl = memnew(Label);
			description_lbl->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
			description_lbl->set_text(TTR("Description:"));
			fields_grid->add_child(description_lbl);

			_description_edit = memnew(TextEdit);
			_description_edit->set_custom_minimum_size(Size2(400, 80) * EDSCALE);
			_description_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			_description_edit->set_line_wrapping_mode(TextEdit::LineWrappingMode::LINE_WRAPPING_BOUNDARY);
			_description_edit->set_tooltip_text(TTR("Optional. This description should be kept relatively short (up to 5 lines).\nIt will display when hovering the plugin in the list of plugins."));
			_description_edit->set_v_size_flags(Control::SIZE_EXPAND_FILL);
			fields_grid->add_child(_description_edit);

			Label *author_lbl = memnew(Label);
			author_lbl->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
			author_lbl->set_text(TTR("Author:"));
			fields_grid->add_child(author_lbl);

			_author_edit = memnew(LineEdit);
			_author_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			_author_edit->set_placeholder("Godette");
			_author_edit->set_tooltip_text(TTR("Optional. The author's username, full name, or organization name."));
			fields_grid->add_child(_author_edit);

			Label *version_lbl = memnew(Label);
			version_lbl->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
			version_lbl->set_text(TTR("Version:"));
			fields_grid->add_child(version_lbl);

			_version_edit = memnew(LineEdit);
			_version_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			_version_edit->set_placeholder("1.0");
			_version_edit->set_tooltip_text(TTR("Optional. A human-readable version identifier used for informational purposes only."));
			fields_grid->add_child(_version_edit);

			Label *script_name_lbl = memnew(Label);
			script_name_lbl->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
			script_name_lbl->set_text(TTR("Script Name:"));
			fields_grid->add_child(script_name_lbl);

			_script_edit = memnew(LineEdit);
			_script_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			_script_edit->set_placeholder(U"\"plugin.gd\" → res://addons/my_plugin/plugin.gd");
			_script_edit->set_tooltip_text(TTR("Optional. The name of the script file. If left empty, will default to the subfolder name."));
			fields_grid->add_child(_script_edit);
		}

		Control *spacing = memnew(Control);
		spacing->set_custom_minimum_size(Size2(0, 10 * EDSCALE));
		content_vbox->add_child(spacing);

		_validation_panel = memnew(EditorValidationPanel);
		_validation_panel->add_line(PLUGIN_NAME, TTRC("Plugin name is valid."));
		_validation_panel->add_line(SCRIPT, TTRC("Script extension is valid."));
		_validation_panel->add_line(SUBFOLDER, TTRC("Subfolder name is valid."));
		_validation_panel->add_line(ENABLE_WARNINGS);
		_validation_panel->set_accept_button(get_ok_button());
		_validation_panel->set_update_callback(callable_mp(this, &ConfigPluginDialog::_validate));
		content_vbox->add_child(_validation_panel);
	}

	_name_edit->connect(SceneStringName(text_changed), callable_mp(_validation_panel, &EditorValidationPanel::update).unbind(1));
	_subfolder_edit->connect(SceneStringName(text_changed), callable_mp(_validation_panel, &EditorValidationPanel::update).unbind(1));
	_script_edit->connect(SceneStringName(text_changed), callable_mp(_validation_panel, &EditorValidationPanel::update).unbind(1));

	_update_mode();
}

void ConfigPluginDialog::popup_create() {
	_config_path = String();
	_clear_fields();
	_update_mode();
	popup_centered();
}

void ConfigPluginDialog::popup_edit(const String &p_config_path) {
	Ref<ConfigFile> config;
	config.instantiate();
	const Error error = config->load(p_config_path);
	ERR_FAIL_COND_MSG(error != OK, vformat("Cannot load plugin config at '%s' (error %d).", p_config_path, error));

	_config_path = p_config_path;
	_clear_fields();
	_name_edit->set_text(config->get_value("plugin", "name", ""));
	_description_edit->set_text(config->get_value("plugin", "description", ""));
	_author_edit->set_text(config->get_value("plugin", "author", ""));
	_version_edit->set_text(config->get_value("plugin", "version", ""));
	_script_edit->set_text(config->get_value("plugin", "script", ""));

	_update_mode();
	popup_centered();
}

void ConfigPluginDialog::_bind_methods() {
	ADD_SIGNAL(MethodInfo("plugin_saved"));
}

void ConfigPluginDialog::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			connect(SceneStringName(confirmed), callable_mp(this, &ConfigPluginDialog::_on_confirmed));
		} break;

		case NOTIFICATION_VISIBILITY_CHANGED: {
			if (is_visible()) {
				_name_edit->grab_focus();
			}
		} break;
	}
}

void ConfigPluginDialog::_clear_fields() {
	_name_edit->clear();
	_subfolder_edit->clear();
	_description_edit->clear();
	_author_edit->clear();
	_version_edit->clear();
	_script_edit->clear();
}

void ConfigPluginDialog::_update_mode() {
	const bool editing = !_config_path.is_empty();
	_subfolder_lbl->set_visible(!editing);
	_subfolder_edit->set_visible(!editing);
	set_ok_button_text(editing ? TTR("Update") : TTR("Create"));
	set_title(editing ? TTR("Edit a Plugin") : TTR("Create a Plugin"));
	get_ok_button()->set_disabled(true);
	_validation_panel->update();
}

void ConfigPluginDialog::_on_confirmed() {
	const Error error = _save_plugin();
	if (error != OK) {
		const String path = _config_path.is_empty() ? String("res://addons").path_join(_get_subfolder()).path_join("plugin.cfg") : _config_path;
		EditorNode::get_singleton()->show_warning(vformat(TTR("Could not save plugin configuration at '%s' (error %d)."), path, error));
		return;
	}

	EditorFileSystem::get_singleton()->scan();
	emit_signal("plugin_saved");
	hide();
}

void ConfigPluginDialog::_validate() {
	if (_name_edit->get_text().is_empty()) {
		_validation_panel->set_message(PLUGIN_NAME, TTRC("Plugin name cannot be blank."), EditorValidationPanel::MSG_ERROR);
	}

	if (_config_path.is_empty()) {
		const String subfolder = _get_subfolder();
		if (!subfolder.is_valid_filename()) {
			_validation_panel->set_message(SUBFOLDER, TTRC("Subfolder name is not a valid folder name."), EditorValidationPanel::MSG_ERROR);
		} else if (DirAccess::exists("res://addons/" + subfolder)) {
			_validation_panel->set_message(SUBFOLDER, TTRC("Subfolder cannot be one which already exists."), EditorValidationPanel::MSG_ERROR);
		}
	} else {
		_validation_panel->set_message(SUBFOLDER, "", EditorValidationPanel::MSG_OK);
	}

#ifndef MODULE_GDSCRIPT_ENABLED
	_validation_panel->set_message(SCRIPT, TTRC("GDScript is unavailable in this build."), EditorValidationPanel::MSG_ERROR);
#endif

	const String script = _script_edit->get_text();
	if ((!script.get_extension().is_empty() && script.get_extension() != "gd") || script.ends_with(".")) {
		_validation_panel->set_message(SCRIPT, TTRC("Script extension must be .gd."), EditorValidationPanel::MSG_ERROR);
	}
	_validation_panel->set_message(ENABLE_WARNINGS, TTRC("Consider enabling GDScript warnings for this plugin by adding an entry for it to the project setting Debug > GDScript > Warnings > Directory Rules."), EditorValidationPanel::MSG_INFO);
}

String ConfigPluginDialog::_get_subfolder() const {
	return _subfolder_edit->get_text().is_empty() ? _name_edit->get_text().replace_char(' ', '_').to_lower() : _subfolder_edit->get_text();
}

Error ConfigPluginDialog::_save_plugin() {
	const bool creating = _config_path.is_empty();
	const String config_path = creating ? String("res://addons").path_join(_get_subfolder()).path_join("plugin.cfg") : _config_path;
	const String plugin_path = config_path.get_base_dir();

	Ref<ConfigFile> config;
	config.instantiate();
	if (creating) {
		Ref<DirAccess> directory = DirAccess::create(DirAccess::ACCESS_RESOURCES);
		ERR_FAIL_COND_V(directory.is_null(), ERR_CANT_OPEN);
		const Error error = directory->make_dir_recursive(plugin_path);
		if (error != OK) {
			return error;
		}
	} else {
		const Error error = config->load(config_path);
		if (error != OK) {
			return error;
		}
	}

	String script_name = _script_edit->get_text();
	if (script_name.is_empty()) {
		script_name = creating ? _get_subfolder() : plugin_path.get_file();
	}
	if (script_name.get_extension() != "gd") {
		script_name += ".gd";
	}

	if (creating) {
		const Error error = _create_plugin_script(plugin_path.path_join(script_name));
		if (error != OK) {
			return error;
		}
	}

	config->set_value("plugin", "name", _name_edit->get_text());
	config->set_value("plugin", "description", _description_edit->get_text());
	config->set_value("plugin", "author", _author_edit->get_text());
	config->set_value("plugin", "version", _version_edit->get_text());
	config->set_value("plugin", "script", script_name);
	return config->save(config_path);
}

Error ConfigPluginDialog::_create_plugin_script(const String &p_script_path) {
	if (FileAccess::exists(p_script_path)) {
		return OK;
	}

	ScriptLanguage *language = nullptr;
#ifdef MODULE_GDSCRIPT_ENABLED
	language = GDScriptLanguage::get_singleton();
#endif
	ERR_FAIL_NULL_V(language, ERR_UNAVAILABLE);

	Ref<Script> script = language->make_script("EditorPlugin");
	ERR_FAIL_COND_V(script.is_null(), ERR_CANT_CREATE);
	script->set_path(p_script_path, true);
	return ResourceSaver::save(script);
}
