/**************************************************************************/
/*  script_editor_plugin.cpp                                              */
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

#include "script_editor_plugin.h"

#include "core/config/project_settings.h"
#include "core/io/json.h"
#include "core/object/script_language.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/docks/script_editor/components/document_editor_container.h"
#include "editor/docks/script_editor/script_editor.h"
#include "editor/editor_node.h"
#include "editor/script/find_in_files.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "scene/resources/text_file.h"

ScriptEditorPlugin::ScriptEditorPlugin() {
	_script_editor_plugin = this;

	ED_SHORTCUT("script_editor/reopen_closed_script", TTRC("Reopen Closed Script"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::T);
	ED_SHORTCUT("script_editor/clear_recent", TTRC("Clear Recent Scripts"));

	ED_SHORTCUT("script_text_editor/convert_to_uppercase", TTRC("Uppercase"), KeyModifierMask::SHIFT | Key::F4);
	ED_SHORTCUT("script_text_editor/convert_to_lowercase", TTRC("Lowercase"), KeyModifierMask::SHIFT | Key::F5);
	ED_SHORTCUT("script_text_editor/capitalize", TTRC("Capitalize"), KeyModifierMask::SHIFT | Key::F6);

	_make_floating_shortcut = ED_SHORTCUT_AND_COMMAND("script_editor/make_floating", TTRC("Make Floating"));
	set_process_shortcut_input(true);

	memnew(FindInFiles);

	_script_editor = memnew(ScriptEditor);
	_script_editor->set_available_layouts(EditorDock::DOCK_LAYOUT_ALL);
	_script_editor->set_default_slot(EditorDock::CENTER_TOP);
	_script_editor->set_dock_shortcut(ED_GET_SHORTCUT("editor/editor_script"));
	_script_editor->set_icon_name("dock_script_editor");
	_script_editor->set_name(TTRC("Script"));

	EditorDockManager::get_singleton()->add_dock(_script_editor);

	ScriptServer::set_reload_scripts_on_save(EDITOR_GET("text_editor/behavior/files/auto_reload_and_parse_scripts_on_save"));
}

ScriptEditorPlugin::~ScriptEditorPlugin() {
	memdelete(FindInFiles::get_singleton());
}

bool ScriptEditorPlugin::open_in_external_editor(const String &p_path, int p_line, int p_col, bool p_ignore_project) {
	const String path = EDITOR_GET("text_editor/external/exec_path");
	if (path.is_empty()) {
		return false;
	}

	String flags = EDITOR_GET("text_editor/external/exec_flags");

	List<String> args;
	bool has_file_flag = false;

	if (!flags.is_empty()) {
		flags = flags.replacen("{line}", itos(MAX(p_line + 1, 1)));
		flags = flags.replacen("{col}", itos(p_col + 1));
		flags = flags.strip_edges().replace("\\\\", "\\");

		int from = 0;
		int num_chars = 0;
		bool inside_quotes = false;

		for (int i = 0; i < flags.size(); i++) {
			if (flags[i] == '"' && (!i || flags[i - 1] != '\\')) {
				if (!inside_quotes) {
					from++;
				}
				inside_quotes = !inside_quotes;

			} else if (flags[i] == '\0' || (!inside_quotes && flags[i] == ' ')) {
				String arg = flags.substr(from, num_chars);
				if (arg.contains("{file}")) {
					has_file_flag = true;
				}

				// Do path replacement here, else there will be issues with spaces and quotes
				if (p_ignore_project) {
					arg = arg.replacen("{project}", String());
				} else {
					arg = arg.replacen("{project}", ProjectSettings::get_singleton()->get_resource_path());
				}
				arg = arg.replacen("{file}", p_path);
				args.push_back(arg);

				from = i + 1;
				num_chars = 0;
			} else {
				num_chars++;
			}
		}
	}

	// Default to passing script path if no {file} flag is specified.
	if (!has_file_flag) {
		args.push_back(p_path);
	}
	return OS::get_singleton()->create_process(path, args) == OK;
}

void ScriptEditorPlugin::edit(Object *p_object) {
	if (!p_object) {
		return;
	}

	if (Object::cast_to<Script>(p_object)) {
		Script *p_script = Object::cast_to<Script>(p_object);
		String res_path = p_script->get_path().get_slice("::", 0);

		// Only show the Script dock automatically when using the in-engine editor.
		if (!p_script->is_built_in() && bool(EDITOR_GET("text_editor/external/use_external_editor"))) {
			_skip_visible = true;
		} else if (p_script->is_built_in() && !res_path.is_empty()) {
			EditorNode::get_singleton()->load_scene_or_resource(res_path, false, false);
		}
		_script_editor->edit(p_script);
	} else if (Object::cast_to<JSON>(p_object) || Object::cast_to<TextFile>(p_object)) {
		_script_editor->edit(Object::cast_to<Resource>(p_object));
	}
}

bool ScriptEditorPlugin::handles(Object *p_object) const {
	if (Object::cast_to<Script>(p_object)) {
		return true;
	} else if (Object::cast_to<JSON>(p_object)) {
		// This is here to stop resource files of class JSON from getting confused
		// with json files and being opened in the text editor.
		if (Object::cast_to<JSON>(p_object)->get_path().get_extension().to_lower() == "json") {
			return true;
		}
	} else if (Object::cast_to<TextFile>(p_object)) {
		return true;
	}

	return p_object->is_class("Script");
}

void ScriptEditorPlugin::make_visible(bool p_visible) {
	if (p_visible) {
		if (_skip_visible) {
			_skip_visible = false;
			return;
		}
		_script_editor->focus_editor();
		_script_editor->get_script_container()->ensure_select_current();
	}
}

String ScriptEditorPlugin::get_unsaved_status(const String &p_for_scene) const {
	const PackedStringArray unsaved_scripts = _script_editor->get_script_container()->get_unsaved_scripts();
	if (unsaved_scripts.is_empty()) {
		return String();
	}

	PackedStringArray message;
	if (!p_for_scene.is_empty()) {
		PackedStringArray unsaved_built_in_scripts;

		const String scene_file = p_for_scene.get_file();
		for (const String &E : unsaved_scripts) {
			if (!E.is_resource_file() && E.contains(scene_file)) {
				unsaved_built_in_scripts.append(E);
			}
		}

		if (unsaved_built_in_scripts.is_empty()) {
			return String();
		} else {
			message.push_back(TTR("There are unsaved changes in the following built-in resource(s)"));
			message.append_array(unsaved_built_in_scripts);
			return String("\n").join(message);
		}
	}

	message.push_back(TTR("Save changes to the following file(s) before quitting?"));
	message.append_array(unsaved_scripts);
	return String("\n").join(message);
}

void ScriptEditorPlugin::save_external_data() {
	if (!EditorNode::get_singleton()->is_exiting()) {
		_script_editor->save_all_scripts();
	}
}

void ScriptEditorPlugin::apply_changes() {
	_script_editor->get_script_container()->apply_scripts();
}

void ScriptEditorPlugin::set_window_layout(Ref<ConfigFile> p_layout) {
	if (bool(EDITOR_GET("text_editor/behavior/files/restore_scripts_on_load"))) {
		_script_editor->get_script_container()->set_window_layout(p_layout);
	}
}

void ScriptEditorPlugin::get_window_layout(Ref<ConfigFile> p_layout) {
	_script_editor->get_script_container()->get_window_layout(p_layout);
}

void ScriptEditorPlugin::get_breakpoints(List<String> *r_breakpoints) {
	_script_editor->get_breakpoints(r_breakpoints);
}

void ScriptEditorPlugin::edited_scene_changed() {
	_script_editor->get_script_container()->edited_scene_changed();
}

void ScriptEditorPlugin::shortcut_input(const Ref<InputEvent> &p_event) {
	if (p_event.is_null() || !p_event->is_pressed() || p_event->is_echo()) {
		return;
	}

	if (_make_floating_shortcut.is_valid() && _make_floating_shortcut->matches_event(p_event)) {
		_script_editor->make_floating();
	}
}
