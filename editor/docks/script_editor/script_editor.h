/**************************************************************************/
/*  script_editor.h                                                       */
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

#pragma once

#include "core/error/error_list.h"
#include "core/object/editor_language.h"
#include "core/object/script_language.h"
#include "core/variant/typed_array.h"
#include "editor/docks/editor_dock.h"
#include "editor/script/script_editor_base.h"

class DocumentEditorContainer;
class EditorHelpSearch;

class ScriptEditor : public EditorDock {
	GDCLASS(ScriptEditor, EditorDock);

public:
	ScriptEditor();
	~ScriptEditor();
	static ScriptEditor *get_singleton() { return _script_editor; }
	DocumentEditorContainer *get_script_container() { return _script_container; }
	bool should_use_external_editor(const Ref<Script> &p_for_script);
	void focus_editor();
	void focus_script_editor(const Ref<Resource> &p_for_resource);
	void reload_scripts(bool p_refresh_only = false);
	void open_help_search_dialog(const String &p_search_text = String());
	void open_script_create_dialog(const String &p_base_name, const String &p_base_path);
	void open_text_file_create_dialog(const String &p_base_path, const String &p_base_name = "");
	Ref<Resource> open_file(const String &p_file);
	Error close_file(const String &p_file);
	_FORCE_INLINE_ bool edit(const Ref<Resource> &p_resource, bool p_grab_focus = true) { return edit(p_resource, -1, 0, p_grab_focus); }
	bool edit(const Ref<Resource> &p_resource, int p_line, int p_col, bool p_grab_focus = true);
	Vector<String> _get_breakpoints();
	void get_breakpoints(List<String> *r_breakpoints);
	void reload_open_files();
	PackedStringArray get_unsaved_files() const;
	void save_current_script();
	void save_all_scripts();
	void update_script_times();
	void set_scene_root_script(const Ref<Script> &p_script);
	Vector<Ref<Script>> get_open_scripts() const;
	ScriptEditorBase *get_current_editor() const;
	bool script_goto_method(Ref<Script> p_script, const String &p_method);
	void notify_script_close(const Ref<Script> &p_script);
	void notify_script_changed(const Ref<Script> &p_script);
	void goto_help(const String &p_desc);
	void update_doc(const String &p_name);
	void clear_docs_from_script(const Ref<Script> &p_script);
	void update_docs_from_script(const Ref<Script> &p_script);
	void rename_symbol(const String &p_symbol, const EditorLanguage::LookupResult &p_lookup);
	void trigger_live_script_reload(const String &p_script_path);
	void set_live_auto_reload_running_scripts(bool p_enabled);
	void register_syntax_highlighter(const Ref<EditorSyntaxHighlighter> &p_syntax_highlighter);
	void unregister_syntax_highlighter(const Ref<EditorSyntaxHighlighter> &p_syntax_highlighter);
	static void register_create_script_editor_function(CreateScriptEditorFunc p_func);

protected:
	static void _bind_methods();
	void _notification(int p_what);
	virtual void update_layout(EditorDock::DockLayout p_layout, int p_slot) override;

private:
	void _on_find_in_files_result_selected(const String &p_path, int p_line_number, int p_begin, int p_end);
	TypedArray<ScriptEditorBase> _get_open_script_editors() const;
	void _goto_line(int p_line);
	Ref<Script> _get_current_script();
	TypedArray<Script> _get_open_scripts() const;
	void _update_margins();
	void _apply_editor_settings();
	inline static ScriptEditor *_script_editor = nullptr;
	DocumentEditorContainer *_script_container = nullptr;
	EditorHelpSearch *_help_search_dialog = nullptr;
	bool _external_editor_active = false;
};
