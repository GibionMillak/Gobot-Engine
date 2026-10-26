/**************************************************************************/
/*  script_editor.cpp                                                     */
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

#include "script_editor.h"

#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
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
#include "editor/settings/editor_settings.h"
#include "editor/shader/shader_editor_plugin.h"
#include "editor/shader/shader_text_editor.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/line_edit.h"
#include "scene/resources/theme.h"

ScriptEditor::ScriptEditor() {
	_script_editor = this;

	_script_container = memnew(DocumentEditorContainer(true, "ScriptEditor", "script_editor_cache.cfg"));
	_script_container->set_handled_resource_types({ "GDScript", "Script", "JSON" });
	add_child(_script_container);
	connect("_focused", callable_mp(_script_container, &DocumentEditorContainer::focus_active_editor));

	_help_search_dialog = memnew(EditorHelpSearch);
	add_child(_help_search_dialog);
	_help_search_dialog->connect("go_to_help", callable_mp(_script_container, &DocumentEditorContainer::goto_help));

	FindInFilesContainer *find_in_files = FindInFiles::get_singleton()->get_container();
	find_in_files->connect("result_selected", callable_mp(this, &ScriptEditor::_on_find_in_files_result_selected));
	find_in_files->connect("files_modified", callable_mp(_script_container, &DocumentEditorContainer::reload_open_files));

	EditorNode::get_singleton()->get_gui_base()->connect(SceneStringName(theme_changed), callable_mp(this, &ScriptEditor::_update_margins));
}

ScriptEditor::~ScriptEditor() {
	ScriptEditorNavigationMarker::release_singleton();
}

bool ScriptEditor::should_use_external_editor(const Ref<Script> &p_for_script) {
	return _external_editor_active && !(p_for_script.is_valid() && p_for_script->is_built_in()); // Ignore external editor for built-in scripts.
}

void ScriptEditor::focus_editor() {
	EditorDockManager::get_singleton()->force_focus_dock(this);
}

void ScriptEditor::focus_script_editor(const Ref<Resource> &p_for_resource) {
	if (!should_use_external_editor(p_for_resource)) {
		focus_editor();
	}
}

void ScriptEditor::reload_scripts(bool p_refresh_only) {
	_script_container->reload_scripts(p_refresh_only);
}

void ScriptEditor::open_help_search_dialog(const String &p_search_text) {
	_help_search_dialog->popup_dialog(p_search_text);
}

void ScriptEditor::open_script_create_dialog(const String &p_base_name, const String &p_base_path) {
	_script_container->open_script_create_dialog(p_base_name, p_base_path);
}

void ScriptEditor::open_text_file_create_dialog(const String &p_base_path, const String &p_base_name) {
	_script_container->open_text_file_create_dialog(p_base_path, p_base_name);
}

Ref<Resource> ScriptEditor::open_file(const String &p_file) {
	return _script_container->open_file(p_file);
}

Error ScriptEditor::close_file(const String &p_file) {
	return _script_container->close_file(p_file);
}

bool ScriptEditor::edit(const Ref<Resource> &p_resource, int p_line, int p_col, bool p_grab_focus) {
	return _script_container->edit(p_resource, p_line, p_col, p_grab_focus);
}

Vector<String> ScriptEditor::_get_breakpoints() {
	return _script_container->_get_breakpoints();
}

void ScriptEditor::get_breakpoints(List<String> *r_breakpoints) {
	_script_container->get_breakpoints(r_breakpoints);
}

void ScriptEditor::reload_open_files() {
	_script_container->reload_open_files();
}

PackedStringArray ScriptEditor::get_unsaved_files() const {
	return _script_container->get_unsaved_files();
}

void ScriptEditor::save_current_script() {
	_script_container->save_current_script();
}

void ScriptEditor::save_all_scripts() {
	_script_container->save_all_scripts();
}

void ScriptEditor::update_script_times() {
	_script_container->update_script_times();
}

void ScriptEditor::set_scene_root_script(const Ref<Script> &p_script) {
	if (p_script.is_null() || should_use_external_editor(p_script)) {
		return;
	}

	bool open_dominant = EDITOR_GET("text_editor/behavior/files/open_dominant_script_on_scene_change");
	if (open_dominant) {
		edit(p_script);
	}
}

Vector<Ref<Script>> ScriptEditor::get_open_scripts() const {
	return _script_container->get_open_scripts();
}

ScriptEditorBase *ScriptEditor::get_current_editor() const {
	return _script_container->get_current_editor();
}

bool ScriptEditor::script_goto_method(Ref<Script> p_script, const String &p_method) {
	return _script_container->script_goto_method(p_script, p_method);
}

void ScriptEditor::notify_script_close(const Ref<Script> &p_script) {
	emit_signal(SNAME("script_close"), p_script);
}

void ScriptEditor::notify_script_changed(const Ref<Script> &p_script) {
	emit_signal(SNAME("editor_script_changed"), p_script);
}

void ScriptEditor::goto_help(const String &p_desc) {
	_script_container->goto_help(p_desc);
}

void ScriptEditor::update_doc(const String &p_name) {
	_script_container->update_doc(p_name);
}

void ScriptEditor::clear_docs_from_script(const Ref<Script> &p_script) {
	_script_container->clear_docs_from_script(p_script);
}

void ScriptEditor::update_docs_from_script(const Ref<Script> &p_script) {
	_script_container->update_docs_from_script(p_script);
}

void ScriptEditor::rename_symbol(const String &p_symbol, const EditorLanguage::LookupResult &p_lookup) {
	FindInFiles::get_singleton()->get_container()->create_rename_control(p_symbol, p_lookup);
	EditorDockManager::get_singleton()->focus_dock(FindInFiles::get_singleton()->get_container());

	LineEdit *name_edit = FindInFiles::get_singleton()->get_container()->get_search_control()->get_rename_line_edit();
	callable_mp((Control *)name_edit, &Control::grab_focus).call_deferred(false);
	callable_mp(name_edit, &LineEdit::select_all).call_deferred();
}

void ScriptEditor::trigger_live_script_reload(const String &p_script_path) {
	_script_container->trigger_live_script_reload(p_script_path);
}

void ScriptEditor::set_live_auto_reload_running_scripts(bool p_enabled) {
	_script_container->set_live_auto_reload_running_scripts(p_enabled);
}

void ScriptEditor::register_syntax_highlighter(const Ref<EditorSyntaxHighlighter> &p_syntax_highlighter) {
	_script_container->register_syntax_highlighter(p_syntax_highlighter);
}

void ScriptEditor::unregister_syntax_highlighter(const Ref<EditorSyntaxHighlighter> &p_syntax_highlighter) {
	_script_container->unregister_syntax_highlighter(p_syntax_highlighter);
}

void ScriptEditor::register_create_script_editor_function(CreateScriptEditorFunc p_func) {
	DocumentEditorContainer::register_create_script_editor_function(p_func);
}

void ScriptEditor::_bind_methods() {
	ClassDB::bind_method("get_current_editor", &ScriptEditor::get_current_editor);
	ClassDB::bind_method("get_open_script_editors", &ScriptEditor::_get_open_script_editors);
	ClassDB::bind_method("get_breakpoints", &ScriptEditor::_get_breakpoints);

	ClassDB::bind_method(D_METHOD("register_syntax_highlighter", "syntax_highlighter"), &ScriptEditor::register_syntax_highlighter);
	ClassDB::bind_method(D_METHOD("unregister_syntax_highlighter", "syntax_highlighter"), &ScriptEditor::unregister_syntax_highlighter);

	ClassDB::bind_method(D_METHOD("goto_line", "line_number"), &ScriptEditor::_goto_line);
	ClassDB::bind_method(D_METHOD("get_current_script"), &ScriptEditor::_get_current_script);
	ClassDB::bind_method(D_METHOD("get_open_scripts"), &ScriptEditor::_get_open_scripts);
	ClassDB::bind_method(D_METHOD("open_script_create_dialog", "base_name", "base_path"), &ScriptEditor::open_script_create_dialog);
	ClassDB::bind_method(D_METHOD("reload_open_files"), &ScriptEditor::reload_open_files);

	ClassDB::bind_method(D_METHOD("goto_help", "topic"), &ScriptEditor::goto_help);
	ClassDB::bind_method(D_METHOD("update_docs_from_script", "script"), &ScriptEditor::update_docs_from_script);
	ClassDB::bind_method(D_METHOD("clear_docs_from_script", "script"), &ScriptEditor::clear_docs_from_script);

	ClassDB::bind_method(D_METHOD("get_unsaved_files"), &ScriptEditor::get_unsaved_files);

	ClassDB::bind_method(D_METHOD("save_all_scripts"), &ScriptEditor::save_all_scripts);
	ClassDB::bind_method(D_METHOD("close_file", "path"), &ScriptEditor::close_file);

	ADD_SIGNAL(MethodInfo("editor_script_changed", PropertyInfo(Variant::OBJECT, "script", PROPERTY_HINT_RESOURCE_TYPE, Script::get_class_static())));
	ADD_SIGNAL(MethodInfo("script_close", PropertyInfo(Variant::OBJECT, "script", PROPERTY_HINT_RESOURCE_TYPE, Script::get_class_static())));
}

void ScriptEditor::_notification(int p_what) {
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

void ScriptEditor::update_layout(EditorDock::DockLayout p_layout, int p_slot) {
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

void ScriptEditor::_on_find_in_files_result_selected(const String &p_path, int p_line_number, int p_begin, int p_end) {
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
						callable_mp(this, &ScriptEditor::focus_editor).call_deferred();
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

TypedArray<ScriptEditorBase> ScriptEditor::_get_open_script_editors() const {
	TypedArray<ScriptEditorBase> script_editors;
	for (Control *editor : _script_container->get_all_editors()) {
		ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(editor);
		if (seb) {
			script_editors.push_back(seb);
		}
	}
	return script_editors;
}

void ScriptEditor::_goto_line(int p_line) {
	TextEditorBase *teb = Object::cast_to<TextEditorBase>(get_current_editor());
	if (teb) {
		ScriptEditorNavigationMarker::get_singleton()->locate_begin();
		teb->goto_line(p_line);
		ScriptEditorNavigationMarker::get_singleton()->locate_end();
	}
}

Ref<Script> ScriptEditor::_get_current_script() {
	ScriptEditorBase *seb = get_current_editor();
	if (seb) {
		Ref<Script> scr = seb->get_edited_resource();
		return scr.is_valid() ? scr : nullptr;
	} else {
		return nullptr;
	}
}

TypedArray<Script> ScriptEditor::_get_open_scripts() const {
	TypedArray<Script> ret;
	Vector<Ref<Script>> scripts = get_open_scripts();
	for (const Ref<Script> &script : scripts) {
		ret.push_back(script);
	}
	return ret;
}

void ScriptEditor::_update_margins() {
	update_layout(get_current_layout(), get_current_slot());
}

void ScriptEditor::_apply_editor_settings() {
	_external_editor_active = EDITOR_GET("text_editor/external/use_external_editor");
	set_auto_switch_allowed(!bool(EDITOR_GET("text_editor/behavior/navigation/stay_in_script_editor_on_node_selected")));
}
