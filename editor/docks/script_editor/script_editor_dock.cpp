/**************************************************************************/
/*  script_editor_dock.cpp                                                */
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

#include "script_editor_dock.h"

#include "core/config/project_settings.h"
#include "core/input/shortcut.h"
#include "core/io/config_file.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "editor/doc/editor_help_search.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/docks/script_editor/components/document_editor_container.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/script/find_in_files.h"
#include "editor/script/script_editor_navigation_marker.h"
#include "editor/script/script_text_editor.h"
#include "editor/script/syntax_highlighters.h"
#include "editor/script/text_editor.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "editor/shader/shader_editor_plugin.h"
#include "editor/shader/shader_text_editor.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/line_edit.h"
#include "scene/resources/text_file.h"
#include "scene/resources/theme.h"

ScriptEditorDock::ScriptEditorDock() {
	_script_editor_dock = this;

	ED_SHORTCUT("script_editor/reopen_closed_script", TTRC("Reopen Closed Script"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::T);
	ED_SHORTCUT("script_editor/clear_recent", TTRC("Clear Recent Scripts"));

	ED_SHORTCUT("script_text_editor/convert_to_uppercase", TTRC("Uppercase"), KeyModifierMask::SHIFT | Key::F4);
	ED_SHORTCUT("script_text_editor/convert_to_lowercase", TTRC("Lowercase"), KeyModifierMask::SHIFT | Key::F5);
	ED_SHORTCUT("script_text_editor/capitalize", TTRC("Capitalize"), KeyModifierMask::SHIFT | Key::F6);

	_make_floating_shortcut = ED_SHORTCUT_AND_COMMAND("script_editor/make_floating", TTRC("Make Floating"));
	set_process_shortcut_input(true);

	set_available_layouts(DOCK_LAYOUT_ALL);
	set_default_slot(CENTER_TOP);
	set_dock_shortcut(ED_GET_SHORTCUT("editor/script_editor"));
	set_icon_name("dock_script_editor");
	set_name(TTRC("Script Editor"));

	_script_container = memnew(DocumentEditorContainer(true, "ScriptEditorDock", "script_editor_cache.cfg"));
	_script_container->set_handled_resource_types({ "GDScript", "Script", "JSON" });
	add_child(_script_container);
	connect("_focused", callable_mp(_script_container, &DocumentEditorContainer::focus_active_editor));

	_help_search_dialog = memnew(EditorHelpSearch);
	add_child(_help_search_dialog);
	_help_search_dialog->connect("go_to_help", callable_mp(_script_container, &DocumentEditorContainer::goto_help));

	FindInFilesContainer *find_in_files = FindInFiles::get_singleton()->get_container();
	find_in_files->connect("result_selected", callable_mp(this, &ScriptEditorDock::_on_find_in_files_result_selected));
	find_in_files->connect("files_modified", callable_mp(_script_container, &DocumentEditorContainer::reload_open_files));

	EditorNode::get_singleton()->get_gui_base()->connect(SceneStringName(theme_changed), callable_mp(this, &ScriptEditorDock::_update_margins));
}

ScriptEditorDock::~ScriptEditorDock() {
	ScriptEditorNavigationMarker::release_singleton();
	_script_editor_dock = nullptr;
}

bool ScriptEditorDock::open_in_external_editor(const String &p_path, int p_line, int p_col, bool p_ignore_project) {
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

bool ScriptEditorDock::handles_resource(Object *p_object) const {
	if (Object::cast_to<Script>(p_object) || Object::cast_to<TextFile>(p_object)) {
		return true;
	}
	const JSON *json = Object::cast_to<JSON>(p_object);
	return json && json->get_path().get_extension().to_lower() == "json";
}

void ScriptEditorDock::edit_resource(Object *p_object) {
	if (!handles_resource(p_object)) {
		return;
	}

	const Ref<Resource> resource = Object::cast_to<Resource>(p_object);
	if (Script *script = Object::cast_to<Script>(resource.ptr())) {
		const String scene_path = script->get_path().get_slice("::", 0);
		if (script->is_built_in() && !scene_path.is_empty()) {
			EditorNode::get_singleton()->load_scene_or_resource(scene_path, false, false);
		}
	}

	if (edit(resource)) {
		focus_editor();
		_script_container->ensure_select_current();
	}
}

String ScriptEditorDock::get_unsaved_status(const String &p_for_scene) const {
	PackedStringArray unsaved_documents;
	for (Control *editor : _script_container->get_all_editors()) {
		ScriptEditorBase *document = Object::cast_to<ScriptEditorBase>(editor);
		if (!document || !document->is_unsaved()) {
			continue;
		}
		if (!p_for_scene.is_empty()) {
			const Ref<Resource> resource = document->get_edited_resource();
			if (resource.is_null() || !resource->is_built_in() || resource->get_path().get_slice("::", 0) != p_for_scene) {
				continue;
			}
		}
		unsaved_documents.append(document->get_document_name());
	}
	if (unsaved_documents.is_empty()) {
		return String();
	}

	PackedStringArray message;
	if (p_for_scene.is_empty()) {
		message.push_back(TTR("Save changes to the following file(s) before quitting?"));
	} else {
		message.push_back(TTR("There are unsaved changes in the following built-in resource(s)"));
	}
	message.append_array(unsaved_documents);
	return String("\n").join(message);
}

void ScriptEditorDock::save_external_data() {
	if (!EditorNode::get_singleton()->is_exiting()) {
		save_all_scripts();
	}
}

void ScriptEditorDock::apply_changes() {
	_script_container->apply_scripts();
}

void ScriptEditorDock::set_window_layout(const Ref<ConfigFile> &p_layout) {
	if (bool(EDITOR_GET("text_editor/behavior/files/restore_scripts_on_load"))) {
		_script_container->set_window_layout(p_layout);
	}
}

void ScriptEditorDock::get_window_layout(Ref<ConfigFile> r_layout) {
	_script_container->get_window_layout(r_layout);
}

void ScriptEditorDock::edited_scene_changed() {
	_script_container->edited_scene_changed();
}

bool ScriptEditorDock::should_use_external_editor(const Ref<Script> &p_for_script) {
	return _external_editor_active && !(p_for_script.is_valid() && p_for_script->is_built_in()); // Ignore external editor for built-in scripts.
}

void ScriptEditorDock::focus_editor() {
	EditorDockManager::get_singleton()->force_focus_dock(this);
}

void ScriptEditorDock::focus_script_editor(const Ref<Resource> &p_for_resource) {
	if (!should_use_external_editor(p_for_resource)) {
		focus_editor();
	}
}

void ScriptEditorDock::reload_scripts(bool p_refresh_only) {
	_script_container->reload_scripts(p_refresh_only);
}

void ScriptEditorDock::open_help_search_dialog(const String &p_search_text) {
	_help_search_dialog->popup_dialog(p_search_text);
}

void ScriptEditorDock::open_script_create_dialog(const String &p_base_name, const String &p_base_path) {
	_script_container->open_script_create_dialog(p_base_name, p_base_path);
}

void ScriptEditorDock::open_text_file_create_dialog(const String &p_base_path, const String &p_base_name) {
	_script_container->open_text_file_create_dialog(p_base_path, p_base_name);
}

Ref<Resource> ScriptEditorDock::open_file(const String &p_file) {
	return _script_container->open_file(p_file);
}

Error ScriptEditorDock::close_file(const String &p_file) {
	return _script_container->close_file(p_file);
}

bool ScriptEditorDock::edit(const Ref<Resource> &p_resource, int p_line, int p_col, bool p_grab_focus) {
	return _script_container->edit(p_resource, p_line, p_col, p_grab_focus);
}

Vector<String> ScriptEditorDock::_get_breakpoints() {
	return _script_container->_get_breakpoints();
}

void ScriptEditorDock::get_breakpoints(List<String> *r_breakpoints) {
	_script_container->get_breakpoints(r_breakpoints);
}

void ScriptEditorDock::reload_open_files() {
	_script_container->reload_open_files();
}

PackedStringArray ScriptEditorDock::get_unsaved_files() const {
	return _script_container->get_unsaved_files();
}

void ScriptEditorDock::save_current_script() {
	_script_container->save_current_script();
}

void ScriptEditorDock::save_all_scripts() {
	_script_container->save_all_scripts();
}

void ScriptEditorDock::update_script_times() {
	_script_container->update_script_times();
}

void ScriptEditorDock::set_scene_root_script(const Ref<Script> &p_script) {
	if (p_script.is_null() || should_use_external_editor(p_script)) {
		return;
	}

	bool open_dominant = EDITOR_GET("text_editor/behavior/files/open_dominant_script_on_scene_change");
	if (open_dominant) {
		edit(p_script);
	}
}

Vector<Ref<Script>> ScriptEditorDock::get_open_scripts() const {
	return _script_container->get_open_scripts();
}

ScriptEditorBase *ScriptEditorDock::get_current_editor() const {
	return _script_container->get_current_editor();
}

bool ScriptEditorDock::script_goto_method(Ref<Script> p_script, const String &p_method) {
	return _script_container->script_goto_method(p_script, p_method);
}

void ScriptEditorDock::notify_script_close(const Ref<Script> &p_script) {
	emit_signal(SNAME("script_close"), p_script);
}

void ScriptEditorDock::notify_script_changed(const Ref<Script> &p_script) {
	emit_signal(SNAME("editor_script_changed"), p_script);
}

void ScriptEditorDock::goto_help(const String &p_desc) {
	_script_container->goto_help(p_desc);
}

void ScriptEditorDock::update_doc(const String &p_name) {
	_script_container->update_doc(p_name);
}

void ScriptEditorDock::clear_docs_from_script(const Ref<Script> &p_script) {
	_script_container->clear_docs_from_script(p_script);
}

void ScriptEditorDock::update_docs_from_script(const Ref<Script> &p_script) {
	_script_container->update_docs_from_script(p_script);
}

void ScriptEditorDock::rename_symbol(const String &p_symbol, const EditorLanguage::LookupResult &p_lookup) {
	FindInFiles::get_singleton()->get_container()->create_rename_control(p_symbol, p_lookup);
	EditorDockManager::get_singleton()->focus_dock(FindInFiles::get_singleton()->get_container());

	LineEdit *name_edit = FindInFiles::get_singleton()->get_container()->get_search_control()->get_rename_line_edit();
	callable_mp((Control *)name_edit, &Control::grab_focus).call_deferred(false);
	callable_mp(name_edit, &LineEdit::select_all).call_deferred();
}

void ScriptEditorDock::trigger_live_script_reload(const String &p_script_path) {
	_script_container->trigger_live_script_reload(p_script_path);
}

void ScriptEditorDock::set_live_auto_reload_running_scripts(bool p_enabled) {
	_script_container->set_live_auto_reload_running_scripts(p_enabled);
}

void ScriptEditorDock::register_syntax_highlighter(const Ref<EditorSyntaxHighlighter> &p_syntax_highlighter) {
	_script_container->register_syntax_highlighter(p_syntax_highlighter);
}

void ScriptEditorDock::unregister_syntax_highlighter(const Ref<EditorSyntaxHighlighter> &p_syntax_highlighter) {
	_script_container->unregister_syntax_highlighter(p_syntax_highlighter);
}

void ScriptEditorDock::register_create_script_editor_function(CreateScriptEditorFunc p_func) {
	DocumentEditorContainer::register_create_script_editor_function(p_func);
}

void ScriptEditorDock::_bind_methods() {
	ClassDB::bind_method("get_current_editor", &ScriptEditorDock::get_current_editor);
	ClassDB::bind_method("get_open_script_editors", &ScriptEditorDock::_get_open_script_editors);
	ClassDB::bind_method("get_breakpoints", &ScriptEditorDock::_get_breakpoints);

	ClassDB::bind_method(D_METHOD("register_syntax_highlighter", "syntax_highlighter"), &ScriptEditorDock::register_syntax_highlighter);
	ClassDB::bind_method(D_METHOD("unregister_syntax_highlighter", "syntax_highlighter"), &ScriptEditorDock::unregister_syntax_highlighter);

	ClassDB::bind_method(D_METHOD("goto_line", "line_number"), &ScriptEditorDock::_goto_line);
	ClassDB::bind_method(D_METHOD("get_current_script"), &ScriptEditorDock::_get_current_script);
	ClassDB::bind_method(D_METHOD("get_open_scripts"), &ScriptEditorDock::_get_open_scripts);
	ClassDB::bind_method(D_METHOD("open_script_create_dialog", "base_name", "base_path"), &ScriptEditorDock::open_script_create_dialog);
	ClassDB::bind_method(D_METHOD("reload_open_files"), &ScriptEditorDock::reload_open_files);

	ClassDB::bind_method(D_METHOD("goto_help", "topic"), &ScriptEditorDock::goto_help);
	ClassDB::bind_method(D_METHOD("update_docs_from_script", "script"), &ScriptEditorDock::update_docs_from_script);
	ClassDB::bind_method(D_METHOD("clear_docs_from_script", "script"), &ScriptEditorDock::clear_docs_from_script);

	ClassDB::bind_method(D_METHOD("get_unsaved_files"), &ScriptEditorDock::get_unsaved_files);

	ClassDB::bind_method(D_METHOD("save_all_scripts"), &ScriptEditorDock::save_all_scripts);
	ClassDB::bind_method(D_METHOD("close_file", "path"), &ScriptEditorDock::close_file);

	ADD_SIGNAL(MethodInfo("editor_script_changed", PropertyInfo(Variant::OBJECT, "script", PROPERTY_HINT_RESOURCE_TYPE, Script::get_class_static())));
	ADD_SIGNAL(MethodInfo("script_close", PropertyInfo(Variant::OBJECT, "script", PROPERTY_HINT_RESOURCE_TYPE, Script::get_class_static())));
}

void ScriptEditorDock::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			_apply_editor_settings();
		} break;

		case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
			if (EditorSettings::get_singleton()->check_changed_settings_in_group("text_editor")) {
				_apply_editor_settings();
			}
		} break;
	}
}

void ScriptEditorDock::shortcut_input(const Ref<InputEvent> &p_event) {
	if (p_event.is_null() || !p_event->is_pressed() || p_event->is_echo()) {
		return;
	}

	if (_make_floating_shortcut.is_valid() && _make_floating_shortcut->matches_event(p_event)) {
		make_floating();
	}
}

void ScriptEditorDock::update_layout(EditorDock::DockLayout p_layout, int p_slot) {
	begin_bulk_theme_override();
	if (p_layout == DOCK_LAYOUT_FLOATING) {
		remove_theme_constant_override("margin_left");
		remove_theme_constant_override("margin_right");
		remove_theme_constant_override("margin_bottom");
	} else {
		int margin = EDSCALE_RND(EditorNode::get_singleton()->get_editor_theme()->get_constant("base_margin", EditorStringName(Editor)));
		add_theme_constant_override("margin_left", margin);
		add_theme_constant_override("margin_right", margin);
		add_theme_constant_override("margin_bottom", margin);
	}
	end_bulk_theme_override();
}

void ScriptEditorDock::_on_find_in_files_result_selected(const String &p_path, int p_line_number, int p_begin, int p_end) {
	ScriptEditorNavigationMarker::get_singleton()->locate_begin();
	if (ResourceLoader::exists(p_path)) {
		Ref<Resource> res = ResourceLoader::load(p_path);

		if (p_path.has_extension("gdshader") || p_path.has_extension("gdshaderinc")) {
			ShaderEditorPlugin *shader_editor = Object::cast_to<ShaderEditorPlugin>(EditorNode::get_editor_data().get_editor_by_name("Shader"));
			shader_editor->edit(res.ptr());
			shader_editor->set_current();
			if (ShaderTextEditor *shader_te = Object::cast_to<ShaderTextEditor>(shader_editor->get_shader_container()->get_resource_editor(res))) {
				shader_te->goto_line_selection(p_line_number - 1, p_begin, p_end);
			}
			ScriptEditorNavigationMarker::get_singleton()->locate_end();
			return;
		} else if (p_path.has_extension("tscn")) {
			const PackedStringArray lines = FileAccess::get_file_as_string(p_path).split("\n");
			if (p_line_number > lines.size()) {
				ScriptEditorNavigationMarker::get_singleton()->locate_end();
				return;
			}

			const char *scr_header = "[sub_resource type=\"GDScript\" id=\"";
			const char *source_header = "script/source = \"";
			String script_id;

			// Search the scene backwards from the found line.
			int scan_line = p_line_number - 1;
			while (scan_line >= 0) {
				const String &line = lines[scan_line];
				if (line.begins_with(source_header)) {
					// Adjust line relative to the script beginning.
					p_line_number -= scan_line + 1;
				} else if (line.begins_with(scr_header)) {
					script_id = line.trim_prefix(scr_header).get_slicec('"', 0);
					break;
				}
				scan_line--;
			}

			EditorNode::get_singleton()->open_scene(p_path);
			if (!script_id.is_empty()) {
				Ref<Script> scr = ResourceLoader::load(p_path + "::" + script_id, "Script");
				if (scr.is_valid()) {
					edit(scr);
					ScriptTextEditor *ste = Object::cast_to<ScriptTextEditor>(get_current_editor());

					if (ste) {
						callable_mp(this, &ScriptEditorDock::focus_editor).call_deferred();
						if (p_line_number == 0) {
							const int source_len = strlen(source_header);
							ste->goto_line_selection(p_line_number, p_begin - source_len, p_end - source_len);
						} else {
							ste->goto_line_selection(p_line_number, p_begin, p_end);
						}
					}
				}
			}

			ScriptEditorNavigationMarker::get_singleton()->locate_end();
			return;
		} else {
			Ref<Script> scr = res;
			Ref<JSON> json = res;
			if (scr.is_valid() || json.is_valid()) {
				if (edit(res)) {
					if (TextEditorBase *text_editor = Object::cast_to<TextEditorBase>(get_current_editor())) {
						focus_script_editor(res);
						text_editor->goto_line_selection(p_line_number - 1, p_begin, p_end);
					}
				}
				ScriptEditorNavigationMarker::get_singleton()->locate_end();
				return;
			}
		}
	}

	// If the file is not a valid resource/script, load it as a text file.
	Ref<Resource> text_file = _script_container->open_file(p_path);
	if (text_file.is_valid()) {
		TextEditor *te = Object::cast_to<TextEditor>(get_current_editor());
		if (te) {
			te->goto_line_selection(p_line_number - 1, p_begin, p_end);
		}
	}
	ScriptEditorNavigationMarker::get_singleton()->locate_end();
}

TypedArray<ScriptEditorBase> ScriptEditorDock::_get_open_script_editors() const {
	TypedArray<ScriptEditorBase> script_editors;
	for (Control *editor : _script_container->get_all_editors()) {
		ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(editor);
		if (seb) {
			script_editors.push_back(seb);
		}
	}
	return script_editors;
}

void ScriptEditorDock::_goto_line(int p_line) {
	TextEditorBase *teb = Object::cast_to<TextEditorBase>(get_current_editor());
	if (teb) {
		ScriptEditorNavigationMarker::get_singleton()->locate_begin();
		teb->goto_line(p_line);
		ScriptEditorNavigationMarker::get_singleton()->locate_end();
	}
}

Ref<Script> ScriptEditorDock::_get_current_script() {
	ScriptEditorBase *seb = get_current_editor();
	if (seb) {
		Ref<Script> scr = seb->get_edited_resource();
		return scr.is_valid() ? scr : nullptr;
	} else {
		return nullptr;
	}
}

TypedArray<Script> ScriptEditorDock::_get_open_scripts() const {
	TypedArray<Script> ret;
	Vector<Ref<Script>> scripts = get_open_scripts();
	for (const Ref<Script> &script : scripts) {
		ret.push_back(script);
	}
	return ret;
}

void ScriptEditorDock::_update_margins() {
	update_layout(get_current_layout(), get_current_slot());
}

void ScriptEditorDock::_apply_editor_settings() {
	_external_editor_active = EDITOR_GET("text_editor/external/use_external_editor");
	set_auto_switch_allowed(!bool(EDITOR_GET("text_editor/behavior/navigation/stay_in_script_editor_on_node_selected")));
}
