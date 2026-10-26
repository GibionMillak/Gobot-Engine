/**************************************************************************/
/*  document_editor_container.h                                           */
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
#include "core/templates/local_vector.h"
#include "editor/script/script_editor_base.h"
#include "scene/gui/margin_container.h"

class Button;
class CodeTextEditor;
class ConfigFile;
class Dialog;
class DocumentList;
class DocumentOutline;
class EditorFileDialog;
class FindReplaceBar;
class HSplitContainer;
class MenuButton;
class PopupMenu;
class ScriptCreateDialog;
class ShaderCreateDialog;
class TabContainer;
class TextFile;
class Timer;
class Tree;
class VSplitContainer;

class DocumentEditorContainer : public MarginContainer {
	GDCLASS(DocumentEditorContainer, MarginContainer);

public:
	DocumentEditorContainer(bool p_is_main_editor, const String &p_config_section, const String &p_cache_path);
	~DocumentEditorContainer() override;
	static Dictionary get_context_data(Control *p_tab_control);
	void apply_scripts() const;
	void reload_scripts(bool p_refresh_only = false);
	void open_script_create_dialog(const String &p_base_name, const String &p_base_path);
	void open_text_file_create_dialog(const String &p_base_path, const String &p_base_name = "");
	Ref<Resource> open_file(const String &p_file);
	bool can_open_file(const String &p_file) const;
	Error close_file(const String &p_file);
	void ensure_select_current();
	void set_handled_resource_types(const HashSet<String> &p_file_types) { _handled_resource_types = p_file_types; }
	_FORCE_INLINE_ bool edit(const Ref<Resource> &p_resource, bool p_grab_focus = true) { return edit(p_resource, -1, 0, p_grab_focus); }
	bool edit(const Ref<Resource> &p_resource, int p_line, int p_col, bool p_grab_focus = true);
	Control *get_active_editor() const;
	Vector<Control *> get_all_editors() const;
	void focus_active_editor();
	Vector<String> _get_breakpoints();
	void get_breakpoints(List<String> *r_breakpoints);
	void reload_open_files();
	PackedStringArray get_unsaved_files() const;
	PackedStringArray get_unsaved_scripts() const;
	void save_current_script();
	void save_all_scripts();
	void update_script_times();
	void sort_document_editors();
	void set_window_layout(Ref<ConfigFile> p_layout);
	void get_window_layout(Ref<ConfigFile> p_layout);
	Vector<Ref<Script>> get_open_scripts() const;
	ScriptEditorBase *get_resource_editor(const Ref<Resource> &p_res) const;
	ScriptEditorBase *get_current_editor() const { return _get_current_editor(); }
	bool script_goto_method(Ref<Script> p_script, const String &p_method);
	void edited_scene_changed();
	void goto_help(const String &p_desc) { _help_class_goto(p_desc); }
	void update_doc(const String &p_name);
	void clear_docs_from_script(const Ref<Script> &p_script);
	void update_docs_from_script(const Ref<Script> &p_script);
	void trigger_live_script_reload(const String &p_script_path);
	void set_live_auto_reload_running_scripts(bool p_enabled);
	void register_syntax_highlighter(const Ref<EditorSyntaxHighlighter> &p_syntax_highlighter);
	void unregister_syntax_highlighter(const Ref<EditorSyntaxHighlighter> &p_syntax_highlighter);
	static void register_create_script_editor_function(CreateScriptEditorFunc p_func);

protected:
	void _notification(int p_what);

private:
	enum MenuOptions {
		// File.
		FILE_MENU_NEW_SCRIPT,
		FILE_MENU_NEW_TEXTFILE,
		FILE_MENU_NEW_SHADER,
		FILE_MENU_NEW_INCLUDE,

		FILE_MENU_OPEN,
		FILE_MENU_OPEN_SHADER,
		FILE_MENU_OPEN_INCLUDE,
		FILE_MENU_REOPEN_CLOSED,
		FILE_MENU_OPEN_RECENT,

		FILE_MENU_SAVE,
		FILE_MENU_SAVE_AS,
		FILE_MENU_SAVE_ALL,

		FILE_MENU_INSPECT,
		FILE_MENU_INSPECT_NATIVE_SHADER_CODE,

		FILE_MENU_SOFT_RELOAD_TOOL,
		FILE_MENU_COPY_PATH,
		FILE_MENU_COPY_UID,
		FILE_MENU_SHOW_IN_FILE_SYSTEM,

		FILE_MENU_THEME_SUBMENU,

		FILE_MENU_CLOSE,
		FILE_MENU_CLOSE_ALL,
		FILE_MENU_CLOSE_OTHER_TABS,
		FILE_MENU_CLOSE_TABS_BELOW,
		FILE_MENU_CLOSE_DOCS,

		FILE_MENU_RUN,

		FILE_MENU_MOVE_UP,
		FILE_MENU_MOVE_DOWN,
		FILE_MENU_SORT,

		// Search.
		HELP_SEARCH_FIND,
		HELP_SEARCH_FIND_NEXT,
		HELP_SEARCH_FIND_PREVIOUS,

		SEARCH_IN_FILES,
		REPLACE_IN_FILES,

		SEARCH_HELP,
		SEARCH_WEBSITE,

		// Theme.
		THEME_IMPORT,
		THEME_RELOAD,
		THEME_SAVE_AS,
	};
	enum DocumentSortMode {
		SORT_BY_NAME,
		SORT_BY_PATH,
		SORT_BY_NONE,
	};
	enum DocumentNameDisplayMode {
		DISPLAY_NAME,
		DISPLAY_DIR_AND_NAME,
		DISPLAY_FULL_PATH,
	};
	friend class DocumentList;
	static constexpr int SCRIPT_EDITOR_FUNC_MAX = 32;
	struct ScriptHistory {
		Control *control = nullptr;
		Dictionary state;
	};
	void _menu_option(int p_option);
	void _theme_option(int p_option);
	void _show_save_theme_as_dialog();
	bool _has_docs_tab() const;
	bool _has_script_tab() const;
	void _resave_scripts(const String &p_str);
	bool _test_script_times_on_disk(Ref<Resource> p_for_script = Ref<Resource>());
	bool _script_exists(const String &p_path) const;
	void _add_recent_script(const String &p_path);
	void _update_recent_scripts();
	void _open_recent_script(int p_idx);
	void _show_error_dialog(const String &p_path);
	void _close_tab(int p_idx, bool p_save = true);
	void _update_find_replace_bar();
	void _close_current_tab(bool p_save = true);
	void _close_discard_current_tab(const String &p_str);
	void _close_docs_tab();
	void _close_other_tabs();
	void _close_tabs_below();
	void _close_all_tabs();
	void _queue_close_tabs();
	void _copy_script_path();
	void _copy_script_uid();
	void _ask_close_current_unsaved_tab(ScriptEditorBase *p_seb);
	void _live_auto_reload_running_scripts();
	void _update_selected_editor_menu();
	void _add_callback(Object *p_obj, const String &p_function, const PackedStringArray &p_args);
	void _res_saved_callback(const Ref<Resource> &p_res);
	void _scene_saved_callback(const String &p_path);
	void _mark_built_in_scripts_as_saved(const String &p_parent_path);
	void _goto_script_line(Ref<RefCounted> p_script, int p_line);
	void _change_execution(Ref<RefCounted> p_script, int p_line = -1, bool p_set = false);
	void _set_execution(Ref<RefCounted> p_script, int p_line) { _change_execution(p_script, p_line, true); }
	void _clear_execution(Ref<RefCounted> p_script) { _change_execution(p_script); }
	String _get_debug_tooltip(const String &p_text, Node *p_se);
	void _resource_created(Ref<Resource> p_res);
	void _set_breakpoint(Ref<RefCounted> p_script, int p_line, bool p_enabled);
	void _clear_breakpoints();
	Array _get_cached_breakpoints_for_script(const String &p_path) const;
	ScriptEditorBase *_get_current_editor() const;
	void _save_editor_state(ScriptEditorBase *p_editor);
	void _save_layout();
	void _apply_editor_settings();
	void _update_filenames();
	void _files_moved(const String &p_old_file, const String &p_new_file);
	void _file_removed(const String &p_file);
	void _update_autosave_timer();
	void _reload_scripts(bool p_refresh_only = false);
	void _auto_format_text(ScriptEditorBase *p_seb);
	void _connect_to_scene();
	void _connect_to_scene_recursive(Node *p_current);
	void _update_document_list();
	void _queue_update_list();
	void _update_online_doc();
	virtual void input(const Ref<InputEvent> &p_event) override;
	virtual void shortcut_input(const Ref<InputEvent> &p_event) override;
	void _prepare_file_menu();
	void _file_menu_closed();
	void _calculate_script_name_button_size();
	void _calculate_script_name_button_ratio();
	void _update_document_name_button();
	void _help_search(const String &p_text = String());
	void _history_forward();
	void _history_back();
	void _roll_back_to_pre_tab();
	void _help_class_open(const String &p_class);
	void _help_class_goto(const String &p_desc);
	void _update_history_arrows();
	void _save_new_history(const Dictionary &p_state, Control *p_control);
	void _save_previous_state(const Dictionary &p_state, Control *p_control);
	void _compress_history_patterns(bool p_once);
	void _go_to_tab(int p_idx, bool p_save_history = false);
	void _update_history_pos(int p_new_pos);
	void _update_modified_scripts_for_external_editor(Ref<Script> p_for_script = Ref<Script>());
	void _validation_updated();
	void _file_dialog_action(const String &p_file);
	List<String> _get_recognized_extensions();
	Ref<TextFile> _load_text_file(const String &p_path, Error *r_error) const;
	Error _save_text_file(Ref<TextFile> p_text_file, const String &p_path);
	void _set_script_zoom_factor(float p_zoom_factor);
	void _update_code_editor_zoom_factor(CodeTextEditor *p_code_text_editor);
	void _close_builtin_scripts_from_scene(const String &p_scene);
	void _toggle_files_pressed(bool p_pressed);
	HBoxContainer *_menu_hbox = nullptr;
	Button *_toggle_files_btn = nullptr;
	MenuButton *_file_menu = nullptr;
	MenuButton *_script_search_menu = nullptr;
	Timer *_autosave_timer = nullptr;
	LocalVector<ScriptEditorBase::EditMenusBase *> _editor_menus;
	PopupMenu *_recent_scripts = nullptr;
	PopupMenu *_theme_submenu = nullptr;
	Button *_help_search_btn = nullptr;
	Button *_site_search_btn = nullptr;
	DocumentList *_document_list = nullptr;
	HSplitContainer *_script_split = nullptr;
	DocumentOutline *_document_outline = nullptr;
	VSplitContainer *_list_split = nullptr;
	TabContainer *_tab_container = nullptr;
	Dialog *_error_dialog = nullptr;
	Dialog *_erase_tab_confirm = nullptr;
	FindReplaceBar *_find_replace_bar = nullptr;
	EditorFileDialog *_file_dialog = nullptr;
	ScriptCreateDialog *_script_create_dialog = nullptr;
	ShaderCreateDialog *_shader_create_dialog = nullptr;
	float _zoom_factor = 1.0f;
	HBoxContainer *_script_name_button_hbox = nullptr;
	Control *_script_name_button_left_spacer = nullptr;
	Control *_script_name_button_right_spacer = nullptr;
	Button *_script_name_btn = nullptr;
	int _script_name_width = 0;
	bool _list_update_queued = false;
	bool _document_sort_group_help_pages = false;
	DocumentSortMode _document_sort_by_mode = SORT_BY_NAME;
	DocumentNameDisplayMode _document_display_name_mode = DISPLAY_NAME;
	bool _highlight_scene_scripts = false;
	Button *_script_back_btn = nullptr;
	Button *_script_forward_btn = nullptr;
#ifdef ANDROID_ENABLED
	Control *_virtual_keyboard_spacer = nullptr;
	int _last_kb_height = -1;
#endif
	static int _script_editor_func_count;
	static CreateScriptEditorFunc _script_editor_funcs[SCRIPT_EDITOR_FUNC_MAX];
	Vector<Ref<EditorSyntaxHighlighter>> _syntax_highlighters;
	Vector<ScriptHistory> _history;
	int _history_pos;
	List<String> _previous_scripts;
	List<int> _script_close_queue;
	bool _restoring_layout = false;
	bool _grab_focus_block = false;
	int _edit_pass = 0;
	bool _is_main_editor = false;
	Tree *_disk_changed_list = nullptr;
	Dialog *_disk_changed = nullptr;
	bool _pending_auto_reload = false;
	bool _auto_reload_running_scripts = true;
	Vector<String> _script_paths_to_reload;
	bool _trim_trailing_whitespace_on_save;
	bool _trim_final_newlines_on_save;
	bool _convert_indent_on_save;
	bool _external_editor_active;
	Ref<ConfigFile> _script_editor_cache;
	String _cache_path;
	int _file_dialog_option;
	String _config_section;
	HashSet<String> _handled_resource_types;
	HashSet<String> _textfile_extensions;
};
