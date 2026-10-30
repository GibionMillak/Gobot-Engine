/**************************************************************************/
/*  document_editor_container.cpp                                         */
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

#include "document_editor_container.h"

#include "core/config/project_settings.h"
#include "core/io/config_file.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/io/resource_uid.h"
#include "core/object/callable_mp.h"
#include "core/object/script_language.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "core/os/thread.h"
#include "core/variant/dictionary.h"
#include "core/version.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/doc/editor_help.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/docks/inspector_dock.h"
#include "editor/docks/script_editor/components/document_list.h"
#include "editor/docks/script_editor/components/document_outline.h"
#include "editor/docks/script_editor/script_editor_dock.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/file_system/editor_paths.h"
#include "editor/gui/code_editor.h"
#include "editor/gui/containers/editor_toaster_container.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/inspector/editor_context_menu_plugin.h"
#include "editor/script/find_in_files.h"
#include "editor/script/script_create_dialog.h"
#include "editor/script/script_editor_base.h"
#include "editor/script/script_editor_navigation_marker.h"
#include "editor/script/script_text_editor.h"
#include "editor/script/syntax_highlighters.h"
#include "editor/settings/editor_settings.h"
#include "editor/shader/shader_create_dialog.h"
#include "editor/shader/shader_text_editor.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme_manager.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/dialog.h"
#include "scene/gui/label.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/separator.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/tree.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "scene/main/timer.h"
#include "scene/main/window.h"
#include "scene/resources/shader.h"
#include "scene/resources/shader_include.h"
#include "scene/resources/text_file.h"
#include "servers/display/display_server.h"

namespace {

struct SortItemData {
	String sort_key;
	int category = 0;
	int index = 0;
	Node *editor = nullptr;

	bool operator<(const SortItemData &p_item) const {
		if (category == p_item.category) {
			if (sort_key == p_item.sort_key) {
				return index < p_item.index;
			} else {
				return sort_key.filenocasecmp_to(p_item.sort_key) < 0;
			}
		} else {
			return category < p_item.category;
		}
	}
};

void _find_changed_scripts_for_external_editor(Node *p_base, Node *p_current, HashSet<Ref<Script>> &r_scripts) {
	if (p_current->get_owner() != p_base && p_base != p_current) {
		return;
	}
	Ref<Script> c = p_current->get_script();

	if (c.is_valid()) {
		r_scripts.insert(c);
	}

	for (int i = 0; i < p_current->get_child_count(); i++) {
		_find_changed_scripts_for_external_editor(p_base, p_current->get_child(i), r_scripts);
	}
}

void _import_text_editor_theme(const String &p_file) {
	if (p_file.get_extension() != "tet") {
		EditorToasterContainer::get_singleton()->popup_str(TTR("Importing theme failed. File is not a text editor theme file (.tet)."), EditorToasterContainer::ERROR);
		return;
	}
	const String theme_name = p_file.get_file().get_basename();
	if (EditorSettings::is_default_text_editor_theme(theme_name.to_lower())) {
		EditorToasterContainer::get_singleton()->popup_str(TTR("Importing theme failed. File name cannot be 'Default', 'Custom', or 'Godot 2'."), EditorToasterContainer::ERROR);
		return;
	}

	const String theme_dir = EditorPaths::get_singleton()->get_text_editor_themes_dir();
	Ref<DirAccess> d = DirAccess::open(theme_dir);
	Error err = FAILED;
	if (d.is_valid()) {
		err = d->copy(p_file, theme_dir.path_join(p_file.get_file()));
	}

	if (err != OK) {
		EditorToasterContainer::get_singleton()->popup_str(TTR("Importing theme failed. Failed to copy theme file."), EditorToasterContainer::ERROR);
		return;
	}

	// Reload themes and switch to new theme.
	EditorSettings::get_singleton()->update_text_editor_themes_list();
	EditorSettings::get_singleton()->set_manually("text_editor/theme/color_theme", theme_name, true);
	EditorSettings::get_singleton()->notify_changes();
}

void _save_text_editor_theme_as(const String &p_file) {
	String file = p_file;
	if (p_file.get_extension() != "tet") {
		file += ".tet";
	}

	const String theme_name = file.get_file().get_basename();
	if (EditorSettings::is_default_text_editor_theme(theme_name.to_lower())) {
		EditorToasterContainer::get_singleton()->popup_str(TTR("Saving theme failed. File name cannot be 'Default', 'Custom', or 'Godot 2'."), EditorToasterContainer::ERROR);
		return;
	}

	const String theme_section = "color_theme";
	const Ref<ConfigFile> cf = memnew(ConfigFile);

	// Use the keys from the Godot 2 theme to know which settings to save.
	HashMap<StringName, Color> text_colors = EditorSettings::get_godot2_text_editor_theme();
	text_colors.sort();
	for (const KeyValue<StringName, Color> &text_color : text_colors) {
		const Color val = EditorSettings::get_singleton()->get_setting(text_color.key);
		const String &key = text_color.key.string().replace("text_editor/theme/highlighting/", "");
		cf->set_value(theme_section, key, val.to_html());
	}

	const Error err = cf->save(file);
	if (err != OK) {
		EditorToasterContainer::get_singleton()->popup_str(TTR("Saving theme failed."), EditorToasterContainer::ERROR);
		return;
	}

	// Reload themes and switch to saved theme.
	EditorSettings::get_singleton()->update_text_editor_themes_list();
	if (p_file.get_base_dir() == EditorPaths::get_singleton()->get_text_editor_themes_dir()) {
		// Don't need to emit signal or notify changes as the colors are already set.
		EditorSettings::get_singleton()->set_manually("text_editor/theme/color_theme", theme_name, false);
	}
}

} // namespace

int DocumentEditorContainer::_script_editor_func_count = 0;
CreateScriptEditorFunc DocumentEditorContainer::_script_editor_funcs[DocumentEditorContainer::SCRIPT_EDITOR_FUNC_MAX];

DocumentEditorContainer::DocumentEditorContainer(bool p_is_main_editor, const String &p_config_section, const String &p_cache_path) {
	_is_main_editor = p_is_main_editor;
	_config_section = p_config_section;
	_cache_path = p_cache_path;

	_script_editor_cache.instantiate();
	_script_editor_cache->load(EditorPaths::get_singleton()->get_project_settings_dir().path_join(_cache_path));

	ED_SHORTCUT("script_editor/sort_documents", TTRC("Sort Documents"));
	ED_SHORTCUT("script_editor/move_document_up", TTRC("Move Document Up"));
	ED_SHORTCUT("script_editor/move_document_down", TTRC("Move Document Down"));
	// FIXME: These should be `Key::GREATER` and `Key::LESS` but those don't work.
	ED_SHORTCUT("script_editor/next_script", TTRC("Next Script"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::PERIOD);
	ED_SHORTCUT("script_editor/prev_script", TTRC("Previous Script"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::COMMA);

	ED_SHORTCUT("script_editor/save", TTRC("Save"), KeyModifierMask::ALT | KeyModifierMask::CMD_OR_CTRL | Key::S);
	ED_SHORTCUT("script_editor/save_as", TTRC("Save As..."));
	ED_SHORTCUT("script_editor/save_all", TTRC("Save All"), KeyModifierMask::SHIFT | KeyModifierMask::ALT | Key::S);
	ED_SHORTCUT_OVERRIDE("script_editor/save_all", "macos", KeyModifierMask::META | KeyModifierMask::CTRL | Key::S);

	ED_SHORTCUT("script_editor/reload_script_soft", TTRC("Soft Reload Tool Script"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::ALT | Key::R);
	ED_SHORTCUT("script_editor/run_file", TTRC("Run"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::X);

	ED_SHORTCUT("script_editor/copy_path", TTRC("Copy File Path"));
	ED_SHORTCUT("script_editor/copy_uid", TTRC("Copy File UID"));
	ED_SHORTCUT("script_editor/show_in_file_system", TTRC("Show in FileSystem"));

	ED_SHORTCUT_ARRAY("script_editor/history_previous", TTRC("History Previous"), { int32_t(KeyModifierMask::ALT | Key::LEFT), int32_t(Key::BACK) });
	ED_SHORTCUT_ARRAY("script_editor/history_next", TTRC("History Next"), { int32_t(KeyModifierMask::ALT | Key::RIGHT), int32_t(Key::FORWARD) });
	ED_SHORTCUT_OVERRIDE("script_editor/history_previous", "macos", KeyModifierMask::ALT | KeyModifierMask::META | Key::LEFT);
	ED_SHORTCUT_OVERRIDE("script_editor/history_next", "macos", KeyModifierMask::ALT | KeyModifierMask::META | Key::RIGHT);

	ED_SHORTCUT("script_editor/close_file", TTRC("Close"), KeyModifierMask::CMD_OR_CTRL | Key::W);
	ED_SHORTCUT("script_editor/close_all", TTRC("Close All"));
	ED_SHORTCUT("script_editor/close_other_tabs", TTRC("Close Other Tabs"));
	ED_SHORTCUT("script_editor/close_tabs_below", TTRC("Close Tabs Below"));
	ED_SHORTCUT("script_editor/close_docs", TTRC("Close Docs"));

	ED_SHORTCUT("script_editor/toggle_files_panel", TTRC("Toggle Files Panel"), KeyModifierMask::CMD_OR_CTRL | Key::BACKSLASH);

	set_process_input(true);
	set_process_shortcut_input(true);

	VBoxContainer *main_vbox = memnew(VBoxContainer);
	add_child(main_vbox);

	{
		_menu_hbox = memnew(HBoxContainer);
		main_vbox->add_child(_menu_hbox);

		{
			_toggle_files_btn = memnew(Button);
			_toggle_files_btn->connect(SceneStringName(toggled), callable_mp(this, &DocumentEditorContainer::_toggle_files_pressed));
			_toggle_files_btn->set_flat(true);
			_toggle_files_btn->set_shortcut(ED_GET_SHORTCUT("script_editor/toggle_files_panel"));
			_toggle_files_btn->set_shortcut_context(this);
			_toggle_files_btn->set_theme_type_variation(SceneStringName(FlatButton));
			_toggle_files_btn->set_toggle_mode(true);
			_toggle_files_btn->set_tooltip_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
			_toggle_files_btn->set_v_size_flags(SIZE_EXPAND | SIZE_SHRINK_CENTER);
			_menu_hbox->add_child(_toggle_files_btn);
			VSeparator *sep = memnew(VSeparator);
			_menu_hbox->add_child(sep);

			_file_menu = memnew(MenuButton);
			_file_menu->get_popup()->connect(SceneStringName(id_pressed), callable_mp(this, &DocumentEditorContainer::_menu_option));
			_file_menu->get_popup()->connect("about_to_popup", callable_mp(this, &DocumentEditorContainer::_prepare_file_menu));
			_file_menu->get_popup()->connect("popup_hide", callable_mp(this, &DocumentEditorContainer::_file_menu_closed));
			_file_menu->set_flat(false);
			_file_menu->set_shortcut_context(this);
			_file_menu->set_switch_on_hover(true);
			_file_menu->set_text(TTRC("File"));
			_file_menu->set_theme_type_variation("FlatMenuButton");
			_menu_hbox->add_child(_file_menu);

			{
				_recent_scripts = memnew(PopupMenu);
				_recent_scripts->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
				_recent_scripts->connect(SceneStringName(id_pressed), callable_mp(this, &DocumentEditorContainer::_open_recent_script));
				_update_recent_scripts();

				_theme_submenu = memnew(PopupMenu);
				_theme_submenu->connect(SceneStringName(id_pressed), callable_mp(this, &DocumentEditorContainer::_theme_option));
				_theme_submenu->add_shortcut(ED_SHORTCUT("script_editor/import_theme", TTRC("Import Theme...")), THEME_IMPORT);
				_theme_submenu->add_shortcut(ED_SHORTCUT("script_editor/reload_theme", TTRC("Reload Theme")), THEME_RELOAD);
				_theme_submenu->add_separator();
				_theme_submenu->add_shortcut(ED_SHORTCUT("script_editor/save_theme_as", TTRC("Save Theme As...")), THEME_SAVE_AS);

				if (_is_main_editor) {
					_file_menu->get_popup()->add_shortcut(ED_SHORTCUT("script_editor/new_script", TTRC("New Script..."), KeyModifierMask::CMD_OR_CTRL | Key::N), FILE_MENU_NEW_SCRIPT);
					_file_menu->get_popup()->add_shortcut(ED_SHORTCUT("script_editor/new_textfile", TTRC("New Text File..."), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::N), FILE_MENU_NEW_TEXTFILE);
					_file_menu->get_popup()->add_shortcut(ED_SHORTCUT("script_editor/open", TTRC("Open...")), FILE_MENU_OPEN);
				} else {
					_file_menu->get_popup()->add_shortcut(ED_SHORTCUT("script_editor/new_shader", TTRC("New Shader..."), KeyModifierMask::CMD_OR_CTRL | Key::N), FILE_MENU_NEW_SHADER);
					_file_menu->get_popup()->add_shortcut(ED_SHORTCUT("script_editor/new_include", TTRC("New Shader Include..."), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::N), FILE_MENU_NEW_INCLUDE);
					_file_menu->get_popup()->add_shortcut(ED_SHORTCUT("script_editor/open_shader", TTRC("Load Shader File..."), KeyModifierMask::CMD_OR_CTRL | Key::O), FILE_MENU_OPEN_SHADER);
					_file_menu->get_popup()->add_shortcut(ED_SHORTCUT("script_editor/open_include", TTRC("Load Shader Include File..."), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::O), FILE_MENU_OPEN_INCLUDE);
				}
				_file_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_editor/reopen_closed_script"), FILE_MENU_REOPEN_CLOSED);
				_file_menu->get_popup()->add_submenu_node_item(TTRC("Open Recent"), _recent_scripts, FILE_MENU_OPEN_RECENT);
				_file_menu->get_popup()->add_separator();
				_file_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_editor/save"), FILE_MENU_SAVE);
				_file_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_editor/save_as"), FILE_MENU_SAVE_AS);
				_file_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_editor/save_all"), FILE_MENU_SAVE_ALL);
				_file_menu->get_popup()->add_separator();
				_file_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_editor/close_file"), FILE_MENU_CLOSE);
				_file_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_editor/close_all"), FILE_MENU_CLOSE_ALL);
				_file_menu->get_popup()->add_separator();
				if (_is_main_editor) {
					_file_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_editor/reload_script_soft"), FILE_MENU_SOFT_RELOAD_TOOL);
					_file_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_editor/run_file"), FILE_MENU_RUN);
					_file_menu->get_popup()->add_separator();
				}
				_file_menu->get_popup()->add_submenu_node_item(TTRC("Theme"), _theme_submenu, FILE_MENU_THEME_SUBMENU);
			}

			_script_search_menu = memnew(MenuButton);
			_script_search_menu->get_popup()->connect(SceneStringName(id_pressed), callable_mp(this, &DocumentEditorContainer::_menu_option));
			_script_search_menu->set_flat(false);
			_script_search_menu->set_shortcut_context(this);
			_script_search_menu->set_switch_on_hover(true);
			_script_search_menu->set_text(TTRC("Search"));
			_script_search_menu->set_theme_type_variation("FlatMenuButton");
			_menu_hbox->add_child(_script_search_menu);

			if (_is_main_editor) {
				MenuButton *debug_menu_btn = memnew(MenuButton);
				debug_menu_btn->set_flat(false);
				debug_menu_btn->set_theme_type_variation("FlatMenuButton");
				_menu_hbox->add_child(debug_menu_btn);
				debug_menu_btn->hide(); // Handled by EditorDebuggerNode below.

				EditorDebuggerNode *debugger = EditorDebuggerNode::get_singleton();
				debugger->set_script_debug_button(debug_menu_btn);
				debugger->connect("goto_script_line", callable_mp(this, &DocumentEditorContainer::_goto_script_line));
				debugger->connect("set_execution", callable_mp(this, &DocumentEditorContainer::_set_execution));
				debugger->connect("clear_execution", callable_mp(this, &DocumentEditorContainer::_clear_execution));
				debugger->connect("breakpoint_set_in_tree", callable_mp(this, &DocumentEditorContainer::_set_breakpoint));
				debugger->connect("breakpoints_cleared_in_tree", callable_mp(this, &DocumentEditorContainer::_clear_breakpoints));
			}

			_script_name_button_hbox = memnew(HBoxContainer);
			_script_name_button_hbox->add_theme_constant_override("separation", 0);
			_script_name_button_hbox->connect(SceneStringName(item_rect_changed), callable_mp(this, &DocumentEditorContainer::_calculate_script_name_button_ratio));
			_script_name_button_hbox->set_h_size_flags(SIZE_EXPAND_FILL);
			_menu_hbox->add_child(_script_name_button_hbox);

			{
				_script_name_button_left_spacer = memnew(Control);
				_script_name_button_left_spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
				_script_name_button_hbox->add_child(_script_name_button_left_spacer);

				_script_name_btn = memnew(Button);
				_script_name_btn->set_flat(true);
				_script_name_btn->set_h_size_flags(SIZE_EXPAND_FILL);
				_script_name_btn->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
				_script_name_btn->set_tooltip_text(TTRC("Navigate to document list."));
				_script_name_button_hbox->add_child(_script_name_btn);

				_script_name_button_right_spacer = memnew(Control);
				_script_name_button_right_spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
				_script_name_button_hbox->add_child(_script_name_button_right_spacer);
			}

			_site_search_btn = memnew(Button);
			_site_search_btn->connect(SceneStringName(pressed), callable_mp(this, &DocumentEditorContainer::_menu_option).bind(SEARCH_WEBSITE));
			_site_search_btn->set_theme_type_variation(SceneStringName(FlatButton));
			_menu_hbox->add_child(_site_search_btn);

			if (_is_main_editor) {
				_help_search_btn = memnew(Button);
				_help_search_btn->connect(SceneStringName(pressed), callable_mp(this, &DocumentEditorContainer::_menu_option).bind(SEARCH_HELP));
				_help_search_btn->set_text(TTRC("Search Help"));
				_help_search_btn->set_theme_type_variation(SceneStringName(FlatButton));
				_menu_hbox->add_child(_help_search_btn);
				_help_search_btn->set_tooltip_text(TTRC("Search the reference documentation."));
			}

			_menu_hbox->add_child(memnew(VSeparator));

			_script_back_btn = memnew(Button);
			_script_back_btn->set_disabled(true);
			_script_back_btn->set_shortcut(ED_GET_SHORTCUT("script_editor/history_previous"));
			_script_back_btn->set_shortcut_context(this);
			_script_back_btn->set_theme_type_variation(SceneStringName(FlatButton));
			_script_back_btn->set_tooltip_text(TTRC("Go to previous edited document."));
			_menu_hbox->add_child(_script_back_btn);
			_script_back_btn->connect(SceneStringName(pressed), callable_mp(this, &DocumentEditorContainer::_history_back));

			_script_forward_btn = memnew(Button);
			_script_forward_btn->set_disabled(true);
			_script_forward_btn->set_shortcut(ED_GET_SHORTCUT("script_editor/history_next"));
			_script_forward_btn->set_shortcut_context(this);
			_script_forward_btn->set_theme_type_variation(SceneStringName(FlatButton));
			_script_forward_btn->set_tooltip_text(TTRC("Go to next edited document."));
			_menu_hbox->add_child(_script_forward_btn);
			_script_forward_btn->connect(SceneStringName(pressed), callable_mp(this, &DocumentEditorContainer::_history_forward));
		}

		_script_split = memnew(HSplitContainer);
		_script_split->set_drag_nested_intersections(true);
		_script_split->set_split_offset(200 * EDSCALE);
		_script_split->set_v_size_flags(SIZE_EXPAND_FILL);
		main_vbox->add_child(_script_split);

		{
			_list_split = memnew(VSplitContainer);
			_list_split->set_drag_nested_intersections(true);
			_list_split->set_v_size_flags(SIZE_EXPAND_FILL);
			_script_split->add_child(_list_split);

			{
				_document_list = memnew(DocumentList(this));
				_list_split->add_child(_document_list);

				_document_outline = memnew(DocumentOutline(this));
				_document_outline->set_custom_minimum_size(Size2(0, 90));
				_document_outline->set_v_size_flags(SIZE_EXPAND_FILL);
				_list_split->add_child(_document_outline);
			}

			VBoxContainer *code_editor_vbox = memnew(VBoxContainer);
			_script_split->add_child(code_editor_vbox);

			{
				_tab_container = memnew(TabContainer);
				_tab_container->connect("tab_changed", callable_mp(this, &DocumentEditorContainer::ensure_select_current).unbind(1));
				_tab_container->set_custom_minimum_size(Size2(200, 0) * EDSCALE);
				_tab_container->set_h_size_flags(SIZE_EXPAND_FILL);
				_tab_container->set_tabs_visible(false);
				_tab_container->set_v_size_flags(SIZE_EXPAND_FILL);
				code_editor_vbox->add_child(_tab_container);

				_find_replace_bar = memnew(FindReplaceBar);
				_find_replace_bar->hide();
				code_editor_vbox->add_child(_find_replace_bar);
			}
		}

#ifdef ANDROID_ENABLED
		_virtual_keyboard_spacer = memnew(Control);
		_virtual_keyboard_spacer->set_h_size_flags(SIZE_EXPAND_FILL);
		main_vbox->add_child(_virtual_keyboard_spacer);
#endif
	}

	const bool initial_state = EditorSettings::get_singleton()->get_project_metadata("show_files_panel", _config_section, true);
	_toggle_files_btn->set_pressed_no_signal(initial_state);
	_toggle_files_pressed(initial_state);

	_script_name_btn->connect(SceneStringName(pressed), callable_mp(_document_list, &DocumentList::ensure_current_is_visible));

	_erase_tab_confirm = memnew(Dialog(true));
	_erase_tab_confirm->add_button(TTRC("Discard"), false, "discard");
	_erase_tab_confirm->connect(SceneStringName(confirmed), callable_mp(this, &DocumentEditorContainer::_close_current_tab).bind(true));
	_erase_tab_confirm->connect("custom_action", callable_mp(this, &DocumentEditorContainer::_close_discard_current_tab));
	_erase_tab_confirm->set_flag(Window::FLAG_RESIZE_DISABLED, true);
	_erase_tab_confirm->set_ok_button_text(TTRC("Save"));
	add_child(_erase_tab_confirm);

	_script_create_dialog = memnew(ScriptCreateDialog);
	_script_create_dialog->set_title(TTRC("Create Script"));
	add_child(_script_create_dialog);
	_script_create_dialog->connect("script_created", callable_mp(this, &DocumentEditorContainer::_resource_created));

	_file_dialog_option = -1;
	_file_dialog = memnew(EditorFileDialog);
	add_child(_file_dialog);
	_file_dialog->connect("file_selected", callable_mp(this, &DocumentEditorContainer::_file_dialog_action));

	_shader_create_dialog = memnew(ShaderCreateDialog);
	_shader_create_dialog->connect("shader_created", callable_mp(this, &DocumentEditorContainer::_resource_created));
	_shader_create_dialog->connect("shader_include_created", callable_mp(this, &DocumentEditorContainer::_resource_created));
	add_child(_shader_create_dialog);

	_error_dialog = memnew(Dialog);
	add_child(_error_dialog);

	_disk_changed = memnew(Dialog(true));
	_disk_changed->add_button(TTRC("Ignore external changes"), true, "resave");
	_disk_changed->connect(SceneStringName(confirmed), callable_mp(this, &DocumentEditorContainer::reload_scripts).bind(false));
	_disk_changed->connect("custom_action", callable_mp(this, &DocumentEditorContainer::_resave_scripts));
	_disk_changed->set_ok_button_text(TTRC("Reload from disk"));
	_disk_changed->set_title(TTRC("Files have been modified outside Godot"));
	add_child(_disk_changed);

	{
		VBoxContainer *disk_changed_vbox = memnew(VBoxContainer);
		_disk_changed->add_child(disk_changed_vbox);

		{
			Label *files_are_newer_lbl = memnew(Label);
			files_are_newer_lbl->set_text(TTRC("The following files are newer on disk:"));
			disk_changed_vbox->add_child(files_are_newer_lbl);

			_disk_changed_list = memnew(Tree);
			_disk_changed_list->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
			_disk_changed_list->set_hide_root(true);
			_disk_changed_list->set_v_size_flags(SIZE_EXPAND_FILL);
			disk_changed_vbox->add_child(_disk_changed_list);

			Label *what_action_lbl = memnew(Label);
			what_action_lbl->set_text(TTRC("What action should be taken?"));
			disk_changed_vbox->add_child(what_action_lbl);
		}
	}

	_autosave_timer = memnew(Timer);
	_autosave_timer->connect(SceneStringName(tree_entered), callable_mp(this, &DocumentEditorContainer::_update_autosave_timer));
	_autosave_timer->connect("timeout", callable_mp(this, &DocumentEditorContainer::save_all_scripts));
	_autosave_timer->set_one_shot(false);
	add_child(_autosave_timer);

	_history_pos = -1;

	_trim_trailing_whitespace_on_save = EDITOR_GET("text_editor/behavior/files/trim_trailing_whitespace_on_save");
	_trim_final_newlines_on_save = EDITOR_GET("text_editor/behavior/files/trim_final_newlines_on_save");
	_convert_indent_on_save = EDITOR_GET("text_editor/behavior/files/convert_indent_on_save");

	Ref<EditorJSONSyntaxHighlighter> json_syntax_highlighter;
	json_syntax_highlighter.instantiate();
	register_syntax_highlighter(json_syntax_highlighter);

	Ref<EditorMarkdownSyntaxHighlighter> markdown_syntax_highlighter;
	markdown_syntax_highlighter.instantiate();
	register_syntax_highlighter(markdown_syntax_highlighter);

	Ref<EditorConfigFileSyntaxHighlighter> config_file_syntax_highlighter;
	config_file_syntax_highlighter.instantiate();
	register_syntax_highlighter(config_file_syntax_highlighter);

	Ref<GDShaderSyntaxHighlighter> gdshader_syntax_highlighter;
	gdshader_syntax_highlighter.instantiate();
	register_syntax_highlighter(gdshader_syntax_highlighter);
}

DocumentEditorContainer::~DocumentEditorContainer() = default;

Dictionary DocumentEditorContainer::get_context_data(Control *p_tab_control) {
	Ref<Resource> current_resource;
	String res_path;
	ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(p_tab_control);
	if (seb) {
		current_resource = seb->get_edited_resource();
		if (current_resource.is_valid()) {
			res_path = current_resource->get_path();
		}
	} else {
		EditorHelp *help = Object::cast_to<EditorHelp>(p_tab_control);
		if (help) {
			res_path = "help:" + help->get_class().unquote();
		}
	}
	EditorContextMenuPlugin::OptionsData context_data;
	context_data["selected_resource"] = current_resource;
	context_data["path"] = res_path;
	return context_data;
}

void DocumentEditorContainer::apply_scripts() const {
	for (int i = 0; i < _tab_container->get_tab_count(); i++) {
		ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(_tab_container->get_tab_control(i));
		if (!seb) {
			continue;
		}

		if (TextEditorBase *teb = Object::cast_to<TextEditorBase>(seb)) {
			teb->insert_final_newline();
		}
		seb->apply_code();
	}
}

void DocumentEditorContainer::reload_scripts(bool p_refresh_only) {
	// Call deferred to make sure it runs on the main thread.
	if (!Thread::is_main_thread()) {
		callable_mp(this, &DocumentEditorContainer::_reload_scripts).call_deferred(p_refresh_only);
		return;
	}
	_reload_scripts(p_refresh_only);
}

void DocumentEditorContainer::open_script_create_dialog(const String &p_base_name, const String &p_base_path) {
	_menu_option(FILE_MENU_NEW_SCRIPT);
	_script_create_dialog->config(p_base_name, p_base_path);
}

void DocumentEditorContainer::open_text_file_create_dialog(const String &p_base_path, const String &p_base_name) {
	_menu_option(FILE_MENU_NEW_TEXTFILE);
	_file_dialog->set_current_dir(p_base_path);
	_file_dialog->set_current_file(p_base_name);
}

Ref<Resource> DocumentEditorContainer::open_file(const String &p_file) {
	if (_get_recognized_extensions().find(p_file.get_extension())) {
		Ref<Resource> scr = ResourceLoader::load(p_file);
		if (scr.is_null()) {
			EditorNode::get_singleton()->show_warning(TTR("Could not load file at:") + "\n\n" + p_file, TTR("Error!"));
			return Ref<Resource>();
		}

		edit(scr);
		return scr;
	}

	Error error;
	Ref<TextFile> text_file = _load_text_file(p_file, &error);
	if (error != OK) {
		EditorNode::get_singleton()->show_warning(TTR("Could not load file at:") + "\n\n" + p_file, TTR("Error!"));
		return Ref<Resource>();
	}

	if (text_file.is_valid()) {
		edit(text_file);
		return text_file;
	}
	return Ref<Resource>();
}

bool DocumentEditorContainer::can_open_file(const String &p_file) const {
	if (p_file.is_empty() || !FileAccess::exists(p_file)) {
		return false;
	}

	const String &file_type = ResourceLoader::get_resource_type(p_file);
	if (_handled_resource_types.has(file_type)) {
		return true;
	}

	if (_is_main_editor && _textfile_extensions.has(p_file.get_extension())) {
		Error err;
		Ref<TextFile> text_file = _load_text_file(p_file, &err);
		if (text_file.is_valid() && err == OK) {
			return true;
		}
	}
	return false;
}

Error DocumentEditorContainer::close_file(const String &p_file) {
	for (int i = 0; i < _tab_container->get_tab_count(); i++) {
		ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(_tab_container->get_tab_control(i));
		if (seb && seb->get_edited_resource()->get_path() == p_file) {
			if (seb->is_unsaved()) {
				seb->get_edited_resource()->reload_from_file();
			}
			_close_tab(i, false);
			return OK;
		}
	}
	return ERR_FILE_NOT_FOUND;
}

void DocumentEditorContainer::ensure_select_current() {
	if (_restoring_layout) {
		return;
	}
	if (_tab_container->get_current_tab() >= 0) {
		TextEditorBase *teb = Object::cast_to<TextEditorBase>(_get_current_editor());
		if (teb) {
			teb->enable_editor();
		}
		if (_get_current_editor() && !_grab_focus_block && is_visible_in_tree()) {
			_get_current_editor()->ensure_focus();
		}
	}
	_update_find_replace_bar();

	_update_selected_editor_menu();
	_update_online_doc();

	_document_list->update_list();

	_document_outline->update_outline();
	_document_outline->update_visibility();

	_update_document_name_button();
}

bool DocumentEditorContainer::edit(const Ref<Resource> &p_resource, int p_line, int p_col, bool p_grab_focus) {
	if (p_resource.is_null()) {
		return false;
	}

	Ref<Script> scr = p_resource;

	// Don't open dominant script if using an external editor.
	bool use_external_editor = ScriptEditorDock::get_singleton()->should_use_external_editor(p_resource);
	const bool open_dominant = EDITOR_GET("text_editor/behavior/files/open_dominant_script_on_scene_change");

	const bool should_open = (open_dominant && !use_external_editor) || !EditorNode::get_singleton()->is_changing_scene();

	if (use_external_editor &&
			(EditorDebuggerNode::get_singleton()->get_dump_stack_script() != p_resource || (EditorDebuggerNode::get_singleton()->get_debug_with_external_editor() && scr.is_valid())) &&
			p_resource->get_path().is_resource_file()) {
		if (ScriptEditorDock::open_in_external_editor(ProjectSettings::get_singleton()->globalize_path(p_resource->get_path()), p_line, p_col)) {
			return false;
		} else {
			ERR_PRINT("Couldn't open external text editor, falling back to the internal editor. Review your `text_editor/external/` editor settings.");
		}
	}

	for (int i = 0; i < _tab_container->get_tab_count(); i++) {
		ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(_tab_container->get_tab_control(i));
		if (!seb) {
			continue;
		}

		if ((scr.is_valid() && seb->get_edited_resource() == p_resource) || seb->get_edited_resource()->get_path() == p_resource->get_path()) {
			if (should_open) {
				TextEditorBase *teb = Object::cast_to<TextEditorBase>(seb);
				if (teb) {
					teb->enable_editor();
				}

				if (_tab_container->get_current_tab() != i) {
					_grab_focus_block = !p_grab_focus;
					_go_to_tab(i, !teb);
					_grab_focus_block = false;
				} else if (is_visible_in_tree() && p_grab_focus) {
					seb->ensure_focus();
				}

				if (teb) {
					ScriptEditorNavigationMarker::get_singleton()->locate_begin();
					if (p_line >= 0) {
						teb->goto_line_and_center_if_necessary(p_line, p_col);
					} else {
						teb->trigger_history_save_on_navigate();
					}
					ScriptEditorNavigationMarker::get_singleton()->locate_end();
				}
				_document_list->ensure_current_is_visible();
			}
			return true;
		}
	}

	// doesn't have it, make a new one
	ScriptEditorBase *seb = nullptr;

	for (int i = _script_editor_func_count - 1; i >= 0; i--) {
		seb = _script_editor_funcs[i](p_resource);
		if (seb) {
			break;
		}
	}
	ERR_FAIL_NULL_V(seb, false);

	seb->set_edited_resource(p_resource);

	_grab_focus_block = !p_grab_focus;
	_tab_container->add_child(seb);
	_grab_focus_block = false;

	if (TextEditorBase *teb = Object::cast_to<TextEditorBase>(seb)) {
		if (p_grab_focus) {
			teb->enable_editor();
		}

		ScriptEditorBase::EditMenusBase *editor_edit_menu = nullptr;
		for (ScriptEditorBase::EditMenusBase *editor_menu : _editor_menus) {
			if (editor_menu->handles(teb)) {
				editor_edit_menu = editor_menu;
				break;
			}
		}
		if (!editor_edit_menu) {
			editor_edit_menu = teb->create_edit_menu(this);
		}
		if (!editor_edit_menu->get_parent()) {
			editor_edit_menu->hide();
			_editor_menus.push_back(editor_edit_menu);
			_menu_hbox->add_child(editor_edit_menu);
			_menu_hbox->move_child(editor_edit_menu, 3);
			for (int i = 0; i < editor_edit_menu->get_child_count(); ++i) {
				Control *c = Object::cast_to<Control>(editor_edit_menu->get_child(i));
				if (c) {
					c->set_shortcut_context(this);
				}
			}
		}
	}

	_update_filenames();
	_grab_focus_block = !p_grab_focus;
	_go_to_tab(_tab_container->get_tab_count() - 1);
	ensure_select_current();
	_grab_focus_block = false;
	_add_recent_script(p_resource->get_path());

	// If we delete a script within the filesystem, the original resource path
	// is lost, so keep it as `edited_file_data` to figure out the exact tab to delete.
	seb->edited_file_data.path = p_resource->get_path();
	seb->edited_file_data.last_modified_time = FileAccess::get_modified_time(p_resource->get_path());

	seb->connect("name_changed", callable_mp(this, &DocumentEditorContainer::_update_filenames));
	seb->connect("_validation_updated", callable_mp(this, &DocumentEditorContainer::_validation_updated));

	if (TextEditorBase *teb = Object::cast_to<TextEditorBase>(seb)) {
		// Syntax highlighting.
		bool highlighter_set = false;
		for (int i = 0; i < _syntax_highlighters.size(); i++) {
			Ref<EditorSyntaxHighlighter> highlighter = _syntax_highlighters[i]->_create();
			if (highlighter.is_null()) {
				continue;
			}
			teb->add_syntax_highlighter(highlighter);

			if (highlighter_set) {
				continue;
			}

			PackedStringArray languages = highlighter->_get_supported_languages();
			// If script try language, else use extension.
			if (scr.is_valid()) {
				if (languages.has(scr->get_language()->get_name())) {
					teb->set_syntax_highlighter(highlighter);
					highlighter_set = true;
				}
				continue;
			}

			if (languages.has(p_resource->get_path().get_extension())) {
				teb->set_syntax_highlighter(highlighter);
				highlighter_set = true;
				continue;
			}

			// For built-in shaders, which do not use file extensions in their path.
			ShaderTextEditor *ste = Object::cast_to<ShaderTextEditor>(teb);
			if (ste && languages.has("gdshader")) {
				teb->set_syntax_highlighter(highlighter);
				highlighter_set = true;
			}
		}

		teb->set_tooltip_request_func(callable_mp(this, &DocumentEditorContainer::_get_debug_tooltip));

		teb->connect("request_help", callable_mp(this, &DocumentEditorContainer::_help_search));
		teb->connect("request_open_script_at_line", callable_mp(this, &DocumentEditorContainer::_goto_script_line));
		teb->connect("go_to_help", callable_mp(this, &DocumentEditorContainer::_help_class_goto));
		teb->connect("_request_save_new_history", callable_mp(this, &DocumentEditorContainer::_save_new_history).bind(teb));
		teb->connect("_request_save_previous_state", callable_mp(this, &DocumentEditorContainer::_save_previous_state).bind(teb));
		teb->connect("go_to_method", callable_mp(this, &DocumentEditorContainer::script_goto_method));

		if (_script_editor_cache->has_section(p_resource->get_path())) {
			teb->set_edit_state(_script_editor_cache->get_value(p_resource->get_path(), "state"), p_grab_focus);
			if (ScriptTextEditor *ste = Object::cast_to<ScriptTextEditor>(teb)) {
				ste->store_previous_state();
			}
		}

		if (CodeTextEditor *cte = teb->get_code_editor()) {
			cte->set_zoom_factor(_zoom_factor);
			cte->connect("zoomed", callable_mp(this, &DocumentEditorContainer::_set_script_zoom_factor));
			cte->connect(SceneStringName(visibility_changed), callable_mp(this, &DocumentEditorContainer::_update_code_editor_zoom_factor).bind(cte));
		}
	}

	sort_document_editors();
	_save_layout();

	//test for modification, maybe the script was not edited but was loaded

	_test_script_times_on_disk(p_resource);
	_update_modified_scripts_for_external_editor(p_resource);

	if (TextEditorBase *teb = Object::cast_to<TextEditorBase>(seb)) {
		ScriptEditorNavigationMarker::get_singleton()->locate_begin();
		if (p_line >= 0) {
			teb->goto_line_and_center_if_necessary(p_line, p_col);
		} else {
			teb->trigger_history_save_on_navigate();
		}
		ScriptEditorNavigationMarker::get_singleton()->locate_end();
	}

	ScriptEditorDock::get_singleton()->notify_script_changed(p_resource);
	return true;
}

Control *DocumentEditorContainer::get_active_editor() const {
	return _tab_container->get_current_tab_control();
}

Vector<Control *> DocumentEditorContainer::get_all_editors() const {
	Vector<Control *> editors;
	editors.reserve(_tab_container->get_tab_count());
	for (int i = 0; i < _tab_container->get_tab_count(); i++) {
		editors.push_back(_tab_container->get_tab_control(i));
	}
	return editors;
}

void DocumentEditorContainer::focus_active_editor() {
	ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(get_active_editor());
	EditorHelp *eh = Object::cast_to<EditorHelp>(get_active_editor());
	if (seb && seb->get_base_editor()) {
		seb->get_base_editor()->grab_focus();
	} else if (eh) {
		eh->set_focused();
	}
}

Vector<String> DocumentEditorContainer::_get_breakpoints() {
	List<String> bpoints_list;
	get_breakpoints(&bpoints_list);

	Vector<String> ret;
	for (const String &E : bpoints_list) {
		ret.push_back(E);
	}

	return ret;
}

void DocumentEditorContainer::get_breakpoints(List<String> *r_breakpoints) {
	HashSet<String> loaded_scripts;
	for (int i = 0; i < _tab_container->get_tab_count(); i++) {
		CodeEditorBase *ceb = Object::cast_to<CodeEditorBase>(_tab_container->get_tab_control(i));
		if (!ceb) {
			continue;
		}

		Ref<Script> scr = ceb->get_edited_resource();
		if (scr.is_null()) {
			continue;
		}

		String base = scr->get_path();
		loaded_scripts.insert(base);
		if (base.is_empty() || base.begins_with("local://")) {
			continue;
		}

		PackedInt32Array bpoints = ceb->get_breakpoints();
		for (int32_t bpoint : bpoints) {
			r_breakpoints->push_back(base + ":" + itos((int)bpoint + 1));
		}
	}

	// Load breakpoints that are in closed scripts.
	Vector<String> cached_editors = _script_editor_cache->get_sections();
	for (const String &E : cached_editors) {
		if (loaded_scripts.has(E)) {
			continue;
		}

		Array breakpoints = _get_cached_breakpoints_for_script(E);
		for (int breakpoint : breakpoints) {
			r_breakpoints->push_back(E + ":" + itos((int)breakpoint + 1));
		}
	}
}

void DocumentEditorContainer::reload_open_files() {
	_test_script_times_on_disk();
	_update_modified_scripts_for_external_editor();
}

PackedStringArray DocumentEditorContainer::get_unsaved_files() const {
	PackedStringArray unsaved_list;

	for (int i = 0; i < _tab_container->get_tab_count(); i++) {
		ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(_tab_container->get_tab_control(i));
		if (seb && seb->is_unsaved()) {
			unsaved_list.append(seb->get_edited_resource()->get_path());
		}
	}
	return unsaved_list;
}

PackedStringArray DocumentEditorContainer::get_unsaved_scripts() const {
	PackedStringArray unsaved_list;

	for (int i = 0; i < _tab_container->get_tab_count(); i++) {
		ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(_tab_container->get_tab_control(i));
		if (seb && seb->is_unsaved()) {
			unsaved_list.append(seb->get_document_name());
		}
	}
	return unsaved_list;
}

void DocumentEditorContainer::save_current_script() {
	ScriptEditorBase *current = _get_current_editor();
	if (!current || _test_script_times_on_disk()) {
		return;
	}

	_auto_format_text(current);

	Ref<Resource> resource = current->get_edited_resource();
	Ref<TextFile> text_file = resource;
	Ref<Script> scr = resource;

	if (text_file.is_valid()) {
		current->apply_code();
		_save_text_file(text_file, text_file->get_path());
		return;
	}

	if (scr.is_valid()) {
		clear_docs_from_script(scr);
	}

	EditorNode::get_singleton()->save_resource(resource);

	if (scr.is_valid()) {
		update_docs_from_script(scr);
	}
}

void DocumentEditorContainer::save_all_scripts() {
	HashSet<String> scenes_to_save;

	for (int i = 0; i < _tab_container->get_tab_count(); i++) {
		ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(_tab_container->get_tab_control(i));
		if (!seb) {
			continue;
		}

		_auto_format_text(seb);

		if (!seb->is_unsaved()) {
			continue;
		}

		Ref<Resource> edited_res = seb->get_edited_resource();
		if (edited_res.is_valid()) {
			seb->apply_code();
		}

		Ref<Script> scr = edited_res;

		if (scr.is_valid()) {
			clear_docs_from_script(scr);
		}

		if (!edited_res->is_built_in()) {
			Ref<TextFile> text_file = edited_res;
			if (text_file.is_valid()) {
				_save_text_file(text_file, text_file->get_path());
				continue;
			}

			// External script, save it.
			EditorNode::get_singleton()->save_resource(edited_res);
		} else {
			// For built-in scripts, save their scenes instead.
			const String scene_path = edited_res->get_path().get_slice("::", 0);
			if (!scene_path.is_empty() && !scenes_to_save.has(scene_path)) {
				scenes_to_save.insert(scene_path);
			}
		}

		if (scr.is_valid()) {
			update_docs_from_script(scr);
		}
	}

	if (!scenes_to_save.is_empty()) {
		EditorNode::get_singleton()->save_scene_list(scenes_to_save);
	}

	_update_filenames();
}

void DocumentEditorContainer::update_script_times() {
	for (int i = 0; i < _tab_container->get_tab_count(); i++) {
		if (ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(_tab_container->get_tab_control(i))) {
			seb->edited_file_data.last_modified_time = FileAccess::get_modified_time(seb->edited_file_data.path);
		}
	}
}

void DocumentEditorContainer::sort_document_editors() {
	if (_restoring_layout) {
		return;
	}
	if (_document_sort_by_mode == SORT_BY_NONE && !_document_sort_group_help_pages) {
		return;
	}
	const int num_open_editors = _tab_container->get_tab_count();

	LocalVector<SortItemData> sort_items;
	sort_items.reserve(num_open_editors);

	for (int i = 0; i < num_open_editors; i++) {
		Control *editor = _tab_container->get_tab_control(i);
		SortItemData item;
		item.index = i;
		item.editor = editor;

		ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(editor);
		EditorHelp *eh = Object::cast_to<EditorHelp>(editor);
		if (seb) {
			item.category = 0;

			switch (_document_sort_by_mode) {
				case SORT_BY_NAME: {
					const String &name = seb->get_document_name();
					item.sort_key = name.to_lower();
				} break;
				case SORT_BY_PATH: {
					const String &path = seb->get_edited_resource()->get_path();
					item.sort_key = path;
				} break;
				default:
					break;
			}
		}
		if (eh) {
			const String name = eh->get_class().is_empty() ? TTR("Unknown Help Class") : eh->get_class().unquote();

			item.category = _document_sort_group_help_pages ? 1 : 0;
			item.sort_key = name.to_lower();
		}
		sort_items.push_back(item);
	}

	sort_items.sort();

	for (int i = 0; i < (int)sort_items.size(); i++) {
		_tab_container->move_child(sort_items[i].editor, i);
	}

	_document_list->update_list();
}

void DocumentEditorContainer::set_window_layout(Ref<ConfigFile> p_layout) {
	if (!p_layout->has_section_key(_config_section, "open_scripts") && !p_layout->has_section_key(_config_section, "open_help")) {
		return;
	}

	Array scripts = p_layout->get_value(_config_section, "open_scripts");
	Array helps;
	if (p_layout->has_section_key(_config_section, "open_help")) {
		helps = p_layout->get_value(_config_section, "open_help");
	}

	_restoring_layout = true;

	HashSet<String> loaded_scripts;
	List<String> extensions = _get_recognized_extensions();

	ScriptEditorNavigationMarker::get_singleton()->init_begin();

	_set_script_zoom_factor(p_layout->get_value(_config_section, "zoom_factor", 1.0f));

	for (const Variant &v : scripts) {
		String path = v;

		Dictionary script_info = v;
		if (!script_info.is_empty()) {
			path = script_info["path"];
		}

		if (!_script_exists(path)) {
			if (_script_editor_cache->has_section(path)) {
				_script_editor_cache->erase_section(path);
			}
			continue;
		} else if (!path.is_resource_file() && !EditorNode::get_singleton()->is_scene_open(path.get_slice("::", 0))) {
			continue;
		}
		loaded_scripts.insert(path);

		bool is_script = false;
		if (path.is_resource_file()) {
			is_script = extensions.find(path.get_extension());
		} else {
			Ref<Script> scr = ResourceCache::get_ref(path);
			if (scr.is_valid()) {
				is_script = true;
			} else {
				continue;
			}
		}

		if (is_script) {
			Ref<Resource> scr = ResourceLoader::load(path);
			if (scr.is_null()) {
				continue;
			}
			if (!edit(scr, false)) {
				continue;
			}
		} else {
			Error error;
			Ref<TextFile> text_file = _load_text_file(path, &error);
			if (error != OK || text_file.is_null()) {
				continue;
			}
			if (!edit(text_file, false)) {
				continue;
			}
		}

		if (!script_info.is_empty()) {
			if (TextEditorBase *teb = Object::cast_to<TextEditorBase>(_tab_container->get_tab_control(_tab_container->get_tab_count() - 1))) {
				teb->set_edit_state(script_info["state"]);
			}
		}
	}

	for (int i = 0; i < helps.size(); i++) {
		String path = helps[i];
		if (path.is_empty()) { // invalid, skip
			continue;
		}
		_help_class_open(path);
	}

	for (int i = 0; i < _tab_container->get_tab_count(); i++) {
		_tab_container->get_tab_control(i)->set_meta("__editor_pass", Variant());
	}

	if (p_layout->has_section_key(_config_section, "script_split_offset")) {
		_script_split->set_split_offset(p_layout->get_value(_config_section, "script_split_offset"));
	}

	if (p_layout->has_section_key(_config_section, "list_split_offset")) {
		_list_split->set_split_offset(p_layout->get_value(_config_section, "list_split_offset"));
	}

	// Remove any deleted editors that have been removed between launches.
	// and if a Script, register breakpoints with the debugger.
	Vector<String> cached_editors = _script_editor_cache->get_sections();
	for (const String &E : cached_editors) {
		if (loaded_scripts.has(E)) {
			continue;
		}

		if (!_script_exists(E)) {
			_script_editor_cache->erase_section(E);
			continue;
		}

		Array breakpoints = _get_cached_breakpoints_for_script(E);
		for (int breakpoint : breakpoints) {
			EditorDebuggerNode::get_singleton()->set_breakpoint(E, (int)breakpoint + 1, true);
		}
	}

	_restoring_layout = false;

	_update_filenames();
	sort_document_editors();
	ScriptEditorNavigationMarker::get_singleton()->init_end();

	bool selected_saved_history = false;
	if (p_layout->has_section_key(_config_section, "selected_script")) {
		String selected_script = p_layout->get_value(_config_section, "selected_script");
		// If the selected script is not in the list of open scripts, select nothing.
		for (int i = 0; i < _tab_container->get_tab_count() && !selected_script.is_empty(); i++) {
			ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(_tab_container->get_tab_control(i));
			if (seb && seb->get_edited_resource()->get_path() == selected_script) {
				_go_to_tab(i, true);
				selected_saved_history = true;
				break;
			}
		}
	}
	ensure_select_current();

	if (!selected_saved_history && _tab_container->get_child_count() > 0) {
		ScriptEditorNavigationMarker::get_singleton()->locate_begin();
		Control *tselected = _tab_container->get_current_tab_control();
		TextEditorBase *teb = Object::cast_to<TextEditorBase>(tselected);
		if (teb && teb->get_code_editor()) {
			teb->get_code_editor()->trigger_history_save_on_navigate();
		}

		EditorHelp *eh = Object::cast_to<EditorHelp>(tselected);
		if (eh) {
			eh->trigger_history_save_on_navigate();
		}
		ScriptEditorNavigationMarker::get_singleton()->locate_end();
	}
}

void DocumentEditorContainer::get_window_layout(Ref<ConfigFile> p_layout) {
	Array scripts;
	Array helps;
	String selected_script;
	for (int i = 0; i < _tab_container->get_tab_count(); i++) {
		if (ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(_tab_container->get_tab_control(i))) {
			const String path = seb->get_edited_resource()->get_path();
			if (path.is_empty()) {
				continue;
			}

			if (_tab_container->get_current_tab_control() == _tab_container->get_tab_control(i)) {
				selected_script = path;
			}

			_save_editor_state(seb);
			scripts.push_back(path);
		}

		if (EditorHelp *eh = Object::cast_to<EditorHelp>(_tab_container->get_tab_control(i))) {
			helps.push_back(eh->get_class());
		}
	}

	p_layout->set_value(_config_section, "open_scripts", scripts);
	p_layout->set_value(_config_section, "selected_script", selected_script);
	p_layout->set_value(_config_section, "open_help", helps);
	p_layout->set_value(_config_section, "script_split_offset", _script_split->get_split_offset());
	p_layout->set_value(_config_section, "list_split_offset", _list_split->get_split_offset());
	p_layout->set_value(_config_section, "zoom_factor", _zoom_factor);

	// Save the cache.
	_script_editor_cache->save(EditorPaths::get_singleton()->get_project_settings_dir().path_join(_cache_path));
}

Vector<Ref<Script>> DocumentEditorContainer::get_open_scripts() const {
	Vector<Ref<Script>> out_scripts = Vector<Ref<Script>>();

	for (int i = 0; i < _tab_container->get_tab_count(); i++) {
		ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(_tab_container->get_tab_control(i));
		if (!seb) {
			continue;
		}

		Ref<Script> scr = seb->get_edited_resource();
		if (scr.is_valid()) {
			out_scripts.push_back(scr);
		}
	}

	return out_scripts;
}

ScriptEditorBase *DocumentEditorContainer::get_resource_editor(const Ref<Resource> &p_res) const {
	for (int i = 0; i < _tab_container->get_tab_count(); i++) {
		ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(_tab_container->get_tab_control(i));
		if (seb && seb->get_edited_resource() == p_res) {
			return seb;
		}
	}
	return nullptr;
}

bool DocumentEditorContainer::script_goto_method(Ref<Script> p_script, const String &p_method) {
	int line = p_script->get_member_line(p_method);

	if (line == -1) {
		return false;
	}

	return edit(p_script, line, 0);
}

void DocumentEditorContainer::edited_scene_changed() {
	_update_modified_scripts_for_external_editor();
}

void DocumentEditorContainer::update_doc(const String &p_name) {
	for (int i = 0; i < _tab_container->get_tab_count(); i++) {
		EditorHelp *eh = Object::cast_to<EditorHelp>(_tab_container->get_tab_control(i));
		if (eh && eh->get_class() == p_name) {
			eh->update_doc();
			return;
		}
	}
}

void DocumentEditorContainer::clear_docs_from_script(const Ref<Script> &p_script) {
	ERR_FAIL_COND(p_script.is_null());

	for (const DocData::ClassDoc &cd : p_script->get_documentation()) {
		if (EditorHelp::get_doc_data()->has_doc(cd.name)) {
			EditorHelp::get_doc_data()->remove_doc(cd.name);
		}
	}
}

void DocumentEditorContainer::update_docs_from_script(const Ref<Script> &p_script) {
	ERR_FAIL_COND(p_script.is_null());

	for (const DocData::ClassDoc &cd : p_script->get_documentation()) {
		EditorHelp::get_doc_data()->add_doc(cd);
		update_doc(cd.name);
	}
}

void DocumentEditorContainer::trigger_live_script_reload(const String &p_script_path) {
	if (!_script_paths_to_reload.has(p_script_path)) {
		Ref<Script> reloaded_script = ResourceCache::get_ref(p_script_path);
		if (reloaded_script.is_null()) {
			reloaded_script = ResourceLoader::load(p_script_path);
		}
		if (reloaded_script.is_valid()) {
			if (!reloaded_script->get_language()->get_editor_language()->validate(reloaded_script->get_source_code(), p_script_path, nullptr, nullptr, nullptr, nullptr)) {
				// Script has errors, don't live reload.
				return;
			}
		}

		_script_paths_to_reload.append(p_script_path);
	}
	if (!_pending_auto_reload && _auto_reload_running_scripts) {
		callable_mp(this, &DocumentEditorContainer::_live_auto_reload_running_scripts).call_deferred();
		_pending_auto_reload = true;
	}
}

void DocumentEditorContainer::set_live_auto_reload_running_scripts(bool p_enabled) {
	_auto_reload_running_scripts = p_enabled;
}

void DocumentEditorContainer::register_syntax_highlighter(const Ref<EditorSyntaxHighlighter> &p_syntax_highlighter) {
	ERR_FAIL_COND(p_syntax_highlighter.is_null());

	if (!_syntax_highlighters.has(p_syntax_highlighter)) {
		_syntax_highlighters.push_back(p_syntax_highlighter);
	}
}

void DocumentEditorContainer::unregister_syntax_highlighter(const Ref<EditorSyntaxHighlighter> &p_syntax_highlighter) {
	ERR_FAIL_COND(p_syntax_highlighter.is_null());

	_syntax_highlighters.erase(p_syntax_highlighter);
}

void DocumentEditorContainer::register_create_script_editor_function(CreateScriptEditorFunc p_func) {
	ERR_FAIL_COND(_script_editor_func_count == SCRIPT_EDITOR_FUNC_MAX);
	_script_editor_funcs[_script_editor_func_count++] = p_func;
}

void DocumentEditorContainer::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_apply_editor_settings();
			[[fallthrough]];
		}

		case NOTIFICATION_TRANSLATION_CHANGED: {
			_update_filenames();
			_update_online_doc();
			[[fallthrough]];
		}
		case NOTIFICATION_THEME_CHANGED: {
			_tab_container->add_theme_style_override(SceneStringName(panel), get_theme_stylebox(SNAME("ScriptEditorDock"), EditorStringName(EditorStyles)));

			_calculate_script_name_button_size();

			if (_is_main_editor) {
				_help_search_btn->set_button_icon(get_editor_theme_icon(SNAME("action_help_search")));
			}
			_site_search_btn->set_button_icon(get_editor_theme_icon(SNAME("navigation_external_link")));

			_script_forward_btn->set_button_icon(get_editor_theme_icon(SNAME("navigation_forward")));
			_script_back_btn->set_button_icon(get_editor_theme_icon(SNAME("navigation_back")));

			_recent_scripts->reset_size();

			_toggle_files_btn->set_button_icon(get_editor_theme_icon(SNAME("gui_sidebar_left")));
		} break;

		case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
			if (EditorThemeManager::is_generated_theme_outdated() ||
					EditorSettings::get_singleton()->check_changed_settings_in_group("interface/editor/fonts") ||
					EditorSettings::get_singleton()->check_changed_settings_in_group("text_editor") ||
					EditorSettings::get_singleton()->check_changed_settings_in_group("docks/filesystem")) {
				_apply_editor_settings();
			}
		} break;

		case NOTIFICATION_READY: {
			if (_is_main_editor) {
				InspectorDock::get_singleton()->connect("request_help", callable_mp(this, &DocumentEditorContainer::_help_class_open));
				EditorNode::get_singleton()->connect("request_help_search", callable_mp(this, &DocumentEditorContainer::_help_search));
				EditorNode::get_singleton()->connect("script_add_function_request", callable_mp(this, &DocumentEditorContainer::_add_callback));
			}
			EditorNode::get_singleton()->connect("scene_closed", callable_mp(this, &DocumentEditorContainer::_close_builtin_scripts_from_scene));
			EditorNode::get_singleton()->connect("resource_saved", callable_mp(this, &DocumentEditorContainer::_res_saved_callback));
			EditorNode::get_singleton()->connect("scene_saved", callable_mp(this, &DocumentEditorContainer::_scene_saved_callback));
			// Connect to scene_root child entered instead of scene_changed for new scenes.
			EditorNode::get_singleton()->get_scene_root()->connect("child_entered_tree", callable_mp(this, &DocumentEditorContainer::_connect_to_scene).unbind(1));
			FileSystemDock::get_singleton()->connect("files_moved", callable_mp(this, &DocumentEditorContainer::_files_moved));
			FileSystemDock::get_singleton()->connect("file_removed", callable_mp(this, &DocumentEditorContainer::_file_removed));

			_script_split->connect("dragged", callable_mp(this, &DocumentEditorContainer::_save_layout).unbind(1));
			_list_split->connect("dragged", callable_mp(this, &DocumentEditorContainer::_save_layout).unbind(1));

			EditorFileSystem::get_singleton()->connect("filesystem_changed", callable_mp(this, &DocumentEditorContainer::_update_filenames));
#ifdef ANDROID_ENABLED
			set_process(true);
#endif
		} break;

#ifdef ANDROID_ENABLED
		case NOTIFICATION_VISIBILITY_CHANGED: {
			set_process(is_visible_in_tree());
		} break;

		case NOTIFICATION_PROCESS: {
			if (!is_visible()) {
				break;
			}

			const int kb_height = DisplayServer::get_singleton()->virtual_keyboard_get_height();
			if (kb_height == _last_kb_height) {
				break;
			}

			_last_kb_height = kb_height;
			if (EditorNode::get_singleton()->is_portrait()) {
				_virtual_keyboard_spacer->set_custom_minimum_size(Size2());
			} else {
				const float status_bar_height = 28 * EDSCALE;
				const float control_bottom = get_global_position().y + get_size().y;
				const float extra_bottom = get_viewport_rect().size.y - control_bottom;
				const float spacer_height = MAX(0.0f, float(kb_height) - extra_bottom - status_bar_height);
				_virtual_keyboard_spacer->set_custom_minimum_size(Size2(0, spacer_height));
			}

			// Make sure caret is visible after resize.
			if (TextEditorBase *editor = Object::cast_to<TextEditorBase>(_get_current_editor())) {
				if (CodeTextEditor *code_editor = editor->get_code_editor()) {
					if (CodeEdit *text_editor = code_editor->get_text_editor()) {
						if (text_editor->has_focus()) {
							text_editor->adjust_viewport_to_caret();
						}
					}
				}
			}
		} break;
#endif
		case NOTIFICATION_APPLICATION_FOCUS_IN: {
			if (is_inside_tree()) {
				_test_script_times_on_disk();
				_update_modified_scripts_for_external_editor();
			}
		} break;
	}
}

void DocumentEditorContainer::_menu_option(int p_option) {
	Node *current_editor = _tab_container->get_current_tab_control();
	ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(current_editor);
	EditorHelp *eh = Object::cast_to<EditorHelp>(current_editor);
	switch (p_option) {
		case FILE_MENU_NEW_SCRIPT: {
			_script_create_dialog->config("Node", "new_script", false, false);
			_script_create_dialog->popup_centered();
		} break;
		case FILE_MENU_NEW_TEXTFILE: {
			_file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_SAVE_FILE);
			_file_dialog->set_access(EditorFileDialog::ACCESS_FILESYSTEM);
			_file_dialog_option = FILE_MENU_NEW_TEXTFILE;

			_file_dialog->clear_filters();
			for (const String &E : _textfile_extensions) {
				_file_dialog->add_filter("*." + E, E.to_upper());
			}
			_file_dialog->set_title(TTRC("New Text File..."));
			_file_dialog->popup_file_dialog();
		} break;
		case FILE_MENU_NEW_SHADER: {
			String base_path = FileSystemDock::get_singleton()->get_current_path().get_base_dir();
			_shader_create_dialog->config(base_path.path_join("new_shader"), false, false, "Shader");
			_shader_create_dialog->popup_centered();
		} break;
		case FILE_MENU_NEW_INCLUDE: {
			String base_path = FileSystemDock::get_singleton()->get_current_path().get_base_dir();
			_shader_create_dialog->config(base_path.path_join("new_shader"), false, false, "ShaderInclude");
			_shader_create_dialog->popup_centered();
		} break;
		case FILE_MENU_OPEN: {
			_file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILE);
			_file_dialog->set_access(EditorFileDialog::ACCESS_FILESYSTEM);
			_file_dialog_option = FILE_MENU_OPEN;

			List<String> extensions;
			ResourceLoader::get_recognized_extensions_for_type("Script", &extensions);
			_file_dialog->clear_filters();
			for (const String &extension : extensions) {
				_file_dialog->add_filter("*." + extension, extension.to_upper());
			}

			for (const String &E : _textfile_extensions) {
				_file_dialog->add_filter("*." + E, E.to_upper());
			}

			_file_dialog->set_title(TTRC("Open File"));
			_file_dialog->popup_file_dialog();
			return;
		} break;
		case FILE_MENU_OPEN_SHADER: {
			InspectorDock::get_singleton()->open_resource("Shader");
		} break;
		case FILE_MENU_OPEN_INCLUDE: {
			InspectorDock::get_singleton()->open_resource("ShaderInclude");
		} break;
		case FILE_MENU_REOPEN_CLOSED: {
			if (_previous_scripts.is_empty()) {
				return;
			}

			String path = _previous_scripts.back()->get();
			_previous_scripts.pop_back();

			bool built_in = !path.is_resource_file();

			if (_get_recognized_extensions().find(path.get_extension()) || built_in) {
				if (built_in) {
					String res_path = path.get_slice("::", 0);
					EditorNode::get_singleton()->load_scene_or_resource(res_path, false, false);
				}

				Ref<Resource> scr = ResourceLoader::load(path);
				if (scr.is_null()) {
					EditorNode::get_singleton()->show_warning(TTR("Could not load file at:") + "\n\n" + path, TTR("Error!"));
					_file_dialog_option = -1;
					return;
				}

				edit(scr);
				_file_dialog_option = -1;
			} else {
				Error error;
				Ref<TextFile> text_file = _load_text_file(path, &error);
				if (error != OK) {
					EditorNode::get_singleton()->show_warning(TTR("Could not load file at:") + "\n\n" + path, TTR("Error!"));
				}

				if (text_file.is_valid()) {
					edit(text_file);
					_file_dialog_option = -1;
				}
			}
		} break;
		case FILE_MENU_SAVE_ALL: {
			if (_test_script_times_on_disk()) {
				return;
			}

			save_all_scripts();
		} break;
		case FILE_MENU_INSPECT: {
			if (seb && seb->get_edited_resource().is_valid()) {
				EditorNode::get_singleton()->push_item(seb->get_edited_resource().ptr());
			}
		} break;
		case FILE_MENU_INSPECT_NATIVE_SHADER_CODE: {
			if (seb) {
				Ref<Shader> shader = seb->get_edited_resource();
				if (shader.is_valid()) {
					shader->inspect_native_shader_code();
				}
			}
		} break;
		case SEARCH_IN_FILES:
		case REPLACE_IN_FILES: {
			FindInFiles::get_singleton()->open_dock("", REPLACE_IN_FILES == p_option);
		}; break;
		case SEARCH_HELP: {
			_help_search();
		} break;
		case SEARCH_WEBSITE: {
			bool native_class_doc = false;
			if (eh) {
				const HashMap<String, DocData::ClassDoc>::ConstIterator E = EditorHelp::get_doc_data()->class_list.find(eh->get_class());
				native_class_doc = E && !E->value.is_script_doc;
			}
			if (native_class_doc) {
				String name = eh->get_class().to_lower();
				String doc_url = vformat(GODOT_VERSION_DOCS_URL "/classes/class_%s.html", name);
				OS::get_singleton()->shell_open(doc_url);
			} else if (seb) {
				String doc_url = vformat(GODOT_VERSION_DOCS_URL "%s", seb->get_doc_url_path());
				OS::get_singleton()->shell_open(doc_url);
			} else {
				OS::get_singleton()->shell_open(GODOT_VERSION_DOCS_URL "/");
			}
		} break;
		case FILE_MENU_SORT: {
			sort_document_editors();
		} break;
		case FILE_MENU_CLOSE: {
			if (seb && seb->is_unsaved()) {
				_ask_close_current_unsaved_tab(seb);
			} else {
				_close_current_tab(false);
			}
		} break;
		case FILE_MENU_CLOSE_DOCS: {
			_close_docs_tab();
		} break;
		case FILE_MENU_CLOSE_OTHER_TABS: {
			_close_other_tabs();
		} break;
		case FILE_MENU_CLOSE_TABS_BELOW: {
			_close_tabs_below();
		} break;
		case FILE_MENU_CLOSE_ALL: {
			_close_all_tabs();
		} break;
		case FILE_MENU_MOVE_UP: {
			if (_tab_container->get_current_tab() > 0) {
				_tab_container->move_child(current_editor, _tab_container->get_current_tab() - 1);
				_document_list->update_list();
			}
		} break;
		case FILE_MENU_MOVE_DOWN: {
			if (_tab_container->get_current_tab() < _tab_container->get_tab_count() - 1) {
				_tab_container->move_child(current_editor, _tab_container->get_current_tab() + 1);
				_document_list->update_list();
			}
		} break;
	}

	if (p_option >= EditorContextMenuPlugin::BASE_ID) {
		EditorContextMenuPluginManager::get_singleton()->activate_custom_option(EditorContextMenuPlugin::CONTEXT_SLOT_SCRIPT_EDITOR, p_option);
		return;
	}

	if (seb) {
		switch (p_option) {
			case FILE_MENU_SAVE: {
				save_current_script();
			} break;
			case FILE_MENU_SAVE_AS: {
				_auto_format_text(seb);

				Ref<Resource> resource = seb->get_edited_resource();
				Ref<TextFile> text_file = resource;
				Ref<Script> scr = resource;

				if (text_file.is_valid()) {
					_file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_SAVE_FILE);
					_file_dialog->set_access(EditorFileDialog::ACCESS_FILESYSTEM);
					_file_dialog_option = FILE_MENU_SAVE_AS;

					List<String> extensions;
					ResourceLoader::get_recognized_extensions_for_type("Script", &extensions);
					_file_dialog->clear_filters();
					_file_dialog->set_current_dir(text_file->get_path().get_base_dir());
					_file_dialog->set_current_file(text_file->get_path().get_file());
					_file_dialog->set_title(TTRC("Save File As..."));
					_file_dialog->popup_file_dialog();
					break;
				}

				if (scr.is_valid()) {
					clear_docs_from_script(scr);
				}

				EditorNode::get_singleton()->push_item(resource.ptr());
				EditorNode::get_singleton()->save_resource_as(resource);

				if (scr.is_valid()) {
					update_docs_from_script(scr);
				}
			} break;

			case FILE_MENU_SOFT_RELOAD_TOOL: {
				Ref<Script> scr = seb->get_edited_resource();
				if (scr.is_null()) {
					EditorNode::get_singleton()->show_warning(TTR("Can't obtain the script for reloading."));
					break;
				}
				if (!scr->is_tool()) {
					EditorNode::get_singleton()->show_warning(TTR("Reload only takes effect on tool scripts."));
					return;
				}
				scr->reload(true);

			} break;

			case FILE_MENU_RUN: {
				Ref<Script> scr = seb->get_edited_resource();
				if (scr.is_null()) {
					EditorToasterContainer::get_singleton()->popup_str(TTR("Cannot run the edited file because it's not a script."), EditorToasterContainer::WARNING);
					break;
				}

				seb->apply_code();

				EditorNode::get_singleton()->run_editor_script(scr);
			} break;
			case FILE_MENU_COPY_PATH: {
				_copy_script_path();
			} break;
			case FILE_MENU_COPY_UID: {
				_copy_script_uid();
			} break;
			case FILE_MENU_SHOW_IN_FILE_SYSTEM: {
				const Ref<Resource> res = seb->get_edited_resource();
				String path = res->get_path();
				if (!path.is_empty()) {
					if (res->is_built_in()) {
						path = path.get_slice("::", 0); // Show the scene instead.
					}

					FileSystemDock::get_singleton()->navigate_to_path(path);
				}
			} break;
		}
	} else if (eh) {
		switch (p_option) {
			case HELP_SEARCH_FIND: {
				eh->popup_search();
			} break;
			case HELP_SEARCH_FIND_NEXT: {
				eh->search_again();
			} break;
			case HELP_SEARCH_FIND_PREVIOUS: {
				eh->search_again(true);
			} break;
		}
	}
}

void DocumentEditorContainer::_theme_option(int p_option) {
	switch (p_option) {
		case THEME_IMPORT: {
			_file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILE);
			_file_dialog->set_access(EditorFileDialog::ACCESS_FILESYSTEM);
			_file_dialog_option = THEME_IMPORT;
			_file_dialog->clear_filters();
			_file_dialog->add_filter("*.tet");
			_file_dialog->set_title(TTRC("Import Theme"));
			_file_dialog->popup_file_dialog();
		} break;
		case THEME_RELOAD: {
			EditorSettings::get_singleton()->mark_setting_changed("text_editor/theme/color_theme");
			EditorSettings::get_singleton()->notify_changes();
		} break;
		case THEME_SAVE_AS: {
			DocumentEditorContainer::_show_save_theme_as_dialog();
		} break;
	}
}

void DocumentEditorContainer::_show_save_theme_as_dialog() {
	_file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_SAVE_FILE);
	_file_dialog->set_access(EditorFileDialog::ACCESS_FILESYSTEM);
	_file_dialog_option = THEME_SAVE_AS;
	_file_dialog->clear_filters();
	_file_dialog->add_filter("*.tet");
	_file_dialog->set_current_path(EditorPaths::get_singleton()->get_text_editor_themes_dir().path_join(EDITOR_GET("text_editor/theme/color_theme")) + " New");
	_file_dialog->set_title(TTRC("Save Theme As..."));
	_file_dialog->popup_file_dialog();
}

bool DocumentEditorContainer::_has_docs_tab() const {
	const int child_count = _tab_container->get_tab_count();
	for (int i = 0; i < child_count; i++) {
		if (Object::cast_to<EditorHelp>(_tab_container->get_tab_control(i))) {
			return true;
		}
	}
	return false;
}

bool DocumentEditorContainer::_has_script_tab() const {
	const int child_count = _tab_container->get_tab_count();
	for (int i = 0; i < child_count; i++) {
		if (Object::cast_to<ScriptEditorBase>(_tab_container->get_tab_control(i))) {
			return true;
		}
	}
	return false;
}

void DocumentEditorContainer::_resave_scripts(const String &p_str) {
	apply_scripts();

	for (int i = 0; i < _tab_container->get_tab_count(); i++) {
		ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(_tab_container->get_tab_control(i));
		if (!seb) {
			continue;
		}

		Ref<Resource> scr = seb->get_edited_resource();

		if (scr->is_built_in()) {
			continue; // Internal script, who cares.
		}

		_auto_format_text(seb);

		Ref<TextFile> text_file = scr;
		if (text_file.is_valid()) {
			seb->apply_code();
			_save_text_file(text_file, text_file->get_path());
			break;
		} else {
			EditorNode::get_singleton()->save_resource(scr);
		}
		seb->tag_saved_version();
	}

	_disk_changed->hide();
}

bool DocumentEditorContainer::_test_script_times_on_disk(Ref<Resource> p_for_script) {
	_disk_changed_list->clear();
	TreeItem *r = _disk_changed_list->create_item();

	bool need_ask = false;
	bool need_reload = false;
	bool use_autoreload = EDITOR_GET("text_editor/behavior/files/auto_reload_scripts_on_external_change");

	for (int i = 0; i < _tab_container->get_tab_count(); i++) {
		ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(_tab_container->get_tab_control(i));
		if (!seb) {
			continue;
		}

		Ref<Resource> edited_res = seb->get_edited_resource();
		if (p_for_script.is_valid() && edited_res.is_valid() && p_for_script != edited_res) {
			continue;
		}

		if (edited_res->is_built_in()) {
			continue; // Internal script, who cares.
		}

		uint64_t last_date = seb->edited_file_data.last_modified_time;
		uint64_t date = FileAccess::get_modified_time(seb->edited_file_data.path);

		if (last_date != date) {
			TreeItem *ti = _disk_changed_list->create_item(r);
			ti->set_text(0, seb->edited_file_data.path.get_file());

			if (!use_autoreload || seb->is_unsaved()) {
				need_ask = true;
			}
			need_reload = true;
		}
	}

	if (need_reload) {
		if (!need_ask) {
			reload_scripts();
			need_reload = false;
		} else {
			callable_mp((Window *)_disk_changed, &Window::popup_centered_ratio).call_deferred(0.3);
		}
	}

	return need_reload;
}

bool DocumentEditorContainer::_script_exists(const String &p_path) const {
	if (p_path.is_empty()) {
		return false;
	} else if (p_path.is_resource_file()) {
		return FileAccess::exists(p_path);
	} else {
		return FileAccess::exists(p_path.get_slice("::", 0));
	}
}

void DocumentEditorContainer::_add_recent_script(const String &p_path) {
	if (p_path.is_empty()) {
		return;
	}

	Array rc = EditorSettings::get_singleton()->get_project_metadata("recent_files", _config_section, Array());
	if (rc.has(p_path)) {
		rc.erase(p_path);
	}
	rc.push_front(p_path);
	if (rc.size() > 10) {
		rc.resize(10);
	}

	EditorSettings::get_singleton()->set_project_metadata("recent_files", _config_section, rc);
	_update_recent_scripts();
}

void DocumentEditorContainer::_update_recent_scripts() {
	Array rc = EditorSettings::get_singleton()->get_project_metadata("recent_files", _config_section, Array());
	_recent_scripts->clear();

	String path;
	for (int i = 0; i < rc.size(); i++) {
		path = rc[i];
		_recent_scripts->add_item(path.replace("res://", ""));
	}

	_recent_scripts->add_separator();
	_recent_scripts->add_shortcut(ED_GET_SHORTCUT("script_editor/clear_recent"));
	_recent_scripts->set_item_auto_translate_mode(-1, AUTO_TRANSLATE_MODE_ALWAYS);
	_recent_scripts->set_item_disabled(-1, rc.is_empty());

	_recent_scripts->reset_size();
}

void DocumentEditorContainer::_open_recent_script(int p_idx) {
	// clear button
	if (p_idx == _recent_scripts->get_item_count() - 1) {
		EditorSettings::get_singleton()->set_project_metadata("recent_files", _config_section, Array());
		callable_mp(this, &DocumentEditorContainer::_update_recent_scripts).call_deferred();
		return;
	}

	Array rc = EditorSettings::get_singleton()->get_project_metadata("recent_files", _config_section, Array());
	ERR_FAIL_INDEX(p_idx, rc.size());

	String path = rc[p_idx];
	// if its not on disk its a help file or deleted
	if (FileAccess::exists(path)) {
		if (_get_recognized_extensions().find(path.get_extension())) {
			Ref<Resource> scr = ResourceLoader::load(path);
			if (scr.is_valid()) {
				edit(scr, true);
				return;
			}
		}

		Error err;
		Ref<TextFile> text_file = _load_text_file(path, &err);
		if (text_file.is_valid()) {
			edit(text_file, true);
			return;
		}
		// if it's a path then it's most likely a deleted file not help
	} else if (path.contains("::")) {
		// built-in script
		String res_path = path.get_slice("::", 0);
		EditorNode::get_singleton()->load_scene_or_resource(res_path, false, false);

		Ref<Script> scr = ResourceLoader::load(path);
		if (scr.is_valid()) {
			edit(scr, true);
			return;
		}
	} else if (!path.is_resource_file()) {
		_help_class_open(path);
		return;
	}

	rc.remove_at(p_idx);
	EditorSettings::get_singleton()->set_project_metadata("recent_files", _config_section, rc);
	_update_recent_scripts();
	_show_error_dialog(path);
}

void DocumentEditorContainer::_show_error_dialog(const String &p_path) {
	_error_dialog->set_text(vformat(TTR("Can't open '%s'. The file could have been moved or deleted."), p_path));
	_error_dialog->popup_centered();
}

void DocumentEditorContainer::_close_tab(int p_idx, bool p_save) {
	int selected = p_idx;
	if (selected < 0 || selected >= _tab_container->get_tab_count()) {
		return;
	}

	Node *tselected = _tab_container->get_tab_control(selected);

	if (ScriptEditorBase *current = Object::cast_to<ScriptEditorBase>(tselected)) {
		Ref<Resource> file = current->get_edited_resource();
		if (p_save && file.is_valid()) {
			// Do not try to save internal scripts, but prompt to save in-memory
			// scripts which are not saved to disk yet (have empty path).
			if (!file->is_built_in()) {
				save_current_script();
			}
		}
		if (file.is_valid()) {
			if (!file->get_path().is_empty()) {
				// Only saved scripts can be restored.
				_previous_scripts.push_back(file->get_path());
			}

			Ref<Script> scr = file;
			if (scr.is_valid()) {
				ScriptEditorDock::get_singleton()->notify_script_close(scr);
			}
		}
	}

	// Roll back to previous tab instead of just previous history state
	_roll_back_to_pre_tab();

	// Remove all history states related to the closed tab.
	for (int i = 0; i < _history.size(); i++) {
		if (_history[i].control == tselected) {
			_history.remove_at(i);
			i--;
			_history_pos--;
		}
	}

	_compress_history_patterns(false);

	int idx = _tab_container->get_current_tab();
	if (TextEditorBase *current = Object::cast_to<TextEditorBase>(tselected)) {
		_save_editor_state(current);
	}
	memdelete(tselected);

	if (_script_close_queue.is_empty()) {
		if (idx >= _tab_container->get_tab_count()) {
			idx = _tab_container->get_tab_count() - 1;
		}
		if (idx >= 0) {
			if (_history_pos >= 0) {
				idx = _tab_container->get_tab_idx_from_control(_history[_history_pos].control);
			}
			_go_to_tab(idx, true);
		} else {
			_update_history_arrows();
		}

		_document_list->update_list();
		_save_layout();
		_update_find_replace_bar();
	}
}

void DocumentEditorContainer::_update_find_replace_bar() {
	if (TextEditorBase *teb = Object::cast_to<TextEditorBase>(_get_current_editor())) {
		teb->set_find_replace_bar(_find_replace_bar);
	} else {
		_find_replace_bar->set_text_edit(nullptr);
		_find_replace_bar->hide();
	}
}

void DocumentEditorContainer::_close_current_tab(bool p_save) {
	_close_tab(_tab_container->get_current_tab(), p_save);
}

void DocumentEditorContainer::_close_discard_current_tab(const String &p_str) {
	ScriptEditorBase *seb = _get_current_editor();
	if (seb) {
		Ref<Script> scr = seb->get_edited_resource();
		if (scr.is_valid()) {
			scr->reload_from_file();
		}
	}
	_close_tab(_tab_container->get_current_tab(), false);
	_erase_tab_confirm->hide();
}

void DocumentEditorContainer::_close_docs_tab() {
	int child_count = _tab_container->get_tab_count();
	for (int i = child_count - 1; i >= 0; i--) {
		if (Object::cast_to<EditorHelp>(_tab_container->get_tab_control(i))) {
			_close_tab(i, true);
		}
	}
}

void DocumentEditorContainer::_close_other_tabs() {
	int current_idx = _tab_container->get_current_tab();
	for (int i = _tab_container->get_tab_count() - 1; i >= 0; i--) {
		if (i != current_idx) {
			_script_close_queue.push_back(i);
		}
	}
	_queue_close_tabs();
}

void DocumentEditorContainer::_close_tabs_below() {
	int current_idx = _tab_container->get_current_tab();
	for (int i = _tab_container->get_tab_count() - 1; i > current_idx; i--) {
		_script_close_queue.push_back(i);
	}
	_go_to_tab(current_idx, true);
	_queue_close_tabs();
}

void DocumentEditorContainer::_close_all_tabs() {
	for (int i = _tab_container->get_tab_count() - 1; i >= 0; i--) {
		_script_close_queue.push_back(i);
	}
	_queue_close_tabs();
}

void DocumentEditorContainer::_queue_close_tabs() {
	while (!_script_close_queue.is_empty()) {
		int idx = _script_close_queue.front()->get();
		_script_close_queue.pop_front();

		_tab_container->set_current_tab(idx);
		ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(_tab_container->get_tab_control(idx));
		if (seb) {
			// Maybe there are unsaved changes.
			if (seb->is_unsaved()) {
				_ask_close_current_unsaved_tab(seb);
				_erase_tab_confirm->connect(SceneStringName(visibility_changed), callable_mp(this, &DocumentEditorContainer::_queue_close_tabs), CONNECT_ONE_SHOT);
				break;
			}
		}

		_close_current_tab(false);
	}
	_update_find_replace_bar();
}

void DocumentEditorContainer::_copy_script_path() {
	if (ScriptEditorBase *seb = _get_current_editor()) {
		Ref<Resource> scr = seb->get_edited_resource();
		DisplayServer::get_singleton()->clipboard_set(scr->get_path());
	}
}

void DocumentEditorContainer::_copy_script_uid() {
	if (ScriptEditorBase *seb = _get_current_editor()) {
		Ref<Resource> scr = seb->get_edited_resource();
		ResourceUID::ID uid = ResourceLoader::get_resource_uid(scr->get_path());
		DisplayServer::get_singleton()->clipboard_set(ResourceUID::get_singleton()->id_to_text(uid));
	}
}

void DocumentEditorContainer::_ask_close_current_unsaved_tab(ScriptEditorBase *p_seb) {
	_erase_tab_confirm->set_text(TTR("Close and save changes?") + "\n\"" + p_seb->get_document_name() + "\"");
	_erase_tab_confirm->popup_centered();
}

void DocumentEditorContainer::_live_auto_reload_running_scripts() {
	_pending_auto_reload = false;
	EditorDebuggerNode::get_singleton()->reload_scripts(_script_paths_to_reload);
	_script_paths_to_reload.clear();
}

void DocumentEditorContainer::_update_selected_editor_menu() {
	for (ScriptEditorBase::EditMenusBase *editor_menu : _editor_menus) {
		editor_menu->set_visible(editor_menu->handles(_get_current_editor()));
	}

	PopupMenu *search_popup = _script_search_menu->get_popup();
	search_popup->clear();
	if (Object::cast_to<EditorHelp>(_tab_container->get_current_tab_control())) {
		search_popup->add_shortcut(ED_SHORTCUT("script_editor/find", TTRC("Find..."), KeyModifierMask::CMD_OR_CTRL | Key::F), HELP_SEARCH_FIND);
		search_popup->add_shortcut(ED_SHORTCUT("script_editor/find_next", TTRC("Find Next"), Key::F3), HELP_SEARCH_FIND_NEXT);
		search_popup->add_shortcut(ED_SHORTCUT("script_editor/find_previous", TTRC("Find Previous"), KeyModifierMask::SHIFT | Key::F3), HELP_SEARCH_FIND_PREVIOUS);
		search_popup->add_separator();
		search_popup->add_shortcut(ED_GET_SHORTCUT("editor/find_in_files"), SEARCH_IN_FILES);
		search_popup->add_shortcut(ED_GET_SHORTCUT("editor/replace_in_files"), REPLACE_IN_FILES);
		_script_search_menu->show();
	} else {
		if (_tab_container->get_tab_count() == 0) {
			search_popup->add_shortcut(ED_GET_SHORTCUT("editor/find_in_files"), SEARCH_IN_FILES);
			search_popup->add_shortcut(ED_GET_SHORTCUT("editor/replace_in_files"), REPLACE_IN_FILES);
			_script_search_menu->show();
		} else {
			_script_search_menu->hide();
		}
	}
}

void DocumentEditorContainer::_add_callback(Object *p_obj, const String &p_function, const PackedStringArray &p_args) {
	ERR_FAIL_NULL(p_obj);
	Ref<Script> scr = p_obj->get_script();
	ERR_FAIL_COND(scr.is_null());

	EditorNode::get_singleton()->push_item(scr.ptr());

	for (int i = 0; i < _tab_container->get_tab_count(); i++) {
		ScriptTextEditor *ste = Object::cast_to<ScriptTextEditor>(_tab_container->get_tab_control(i));
		if (!ste || ste->get_edited_resource() != scr) {
			continue;
		}

		ste->add_callback(p_function, p_args);

		_go_to_tab(i, true);

		// Save the current script so the changes can be picked up by an external editor.
		if (!scr.ptr()->is_built_in()) { // But only if it's not built-in script.
			save_current_script();
		} else {
			ste->apply_code();
			scr->reload(true);
		}

		break;
	}

	// Move back to the previously edited node to reselect it in the Inspector and the SignalsDock.
	// We assume that the previous item is the node on which the callbacks were added.
	EditorNode::get_singleton()->edit_previous_item();
}

void DocumentEditorContainer::_res_saved_callback(const Ref<Resource> &p_res) {
	for (int i = 0; i < _tab_container->get_tab_count(); i++) {
		if (ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(_tab_container->get_tab_control(i))) {
			if (seb->get_edited_resource() == p_res) {
				seb->tag_saved_version();
			}
		}
	}

	if (p_res.is_valid()) {
		// In case the Resource has built-in scripts.
		_mark_built_in_scripts_as_saved(p_res->get_path());
	}

	_update_filenames();
	Ref<Script> scr = p_res;
	if (scr.is_valid()) {
		trigger_live_script_reload(scr->get_path());
	}
}

void DocumentEditorContainer::_scene_saved_callback(const String &p_path) {
	// If scene was saved, mark all built-in scripts from that scene as saved.
	_mark_built_in_scripts_as_saved(p_path);
}

void DocumentEditorContainer::_mark_built_in_scripts_as_saved(const String &p_parent_path) {
	for (int i = 0; i < _tab_container->get_tab_count(); i++) {
		ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(_tab_container->get_tab_control(i));
		if (!seb) {
			continue;
		}

		Ref<Resource> edited_res = seb->get_edited_resource();
		if (!edited_res->is_built_in()) {
			continue; // External script, who cares.
		}

		if (edited_res->get_path().get_slice("::", 0) != p_parent_path) {
			continue; // Wrong scene.
		}
		seb->tag_saved_version();

		Ref<Script> scr = edited_res;
		if (scr.is_valid()) {
			trigger_live_script_reload(scr->get_path());
			clear_docs_from_script(scr);
			if (scr->is_tool()) {
				scr->get_language()->reload_tool_script(edited_res);
			} else {
				scr->get_language()->reload_scripts({ scr });
			}
			update_docs_from_script(scr);
		}
	}
}

void DocumentEditorContainer::_goto_script_line(Ref<RefCounted> p_script, int p_line) {
	Ref<Script> scr = Object::cast_to<Script>(*p_script);
	if (scr.is_valid() && (scr->has_source_code() || scr->get_path().is_resource_file())) {
		if (edit(p_script, p_line, 0)) {
			EditorNode::get_singleton()->push_item(p_script.ptr());
		}
	}
}

void DocumentEditorContainer::_change_execution(Ref<RefCounted> p_script, int p_line, bool p_set) {
	Ref<Script> scr = Object::cast_to<Script>(*p_script);
	if (scr.is_valid() && (scr->has_source_code() || scr->get_path().is_resource_file())) {
		for (int i = 0; i < _tab_container->get_tab_count(); i++) {
			if (TextEditorBase *teb = Object::cast_to<TextEditorBase>(_tab_container->get_tab_control(i))) {
				if ((scr.is_valid() && teb->get_edited_resource() == p_script) || teb->get_edited_resource()->get_path() == scr->get_path()) {
					if (p_set) {
						teb->set_executing_line(p_line);
					} else {
						teb->clear_executing_line();
					}
				}
			}
		}
	}
}

String DocumentEditorContainer::_get_debug_tooltip(const String &p_text, Node *p_se) {
	if (EDITOR_GET("text_editor/behavior/documentation/enable_tooltips")) {
		return String();
	}

	// NOTE: See also `ScriptTextEditor::_show_symbol_tooltip()` for documentation tooltips enabled.
	String debug_value = EditorDebuggerNode::get_singleton()->get_var_value(p_text);
	if (!debug_value.is_empty()) {
		constexpr int DISPLAY_LIMIT = 1024;
		if (debug_value.size() > DISPLAY_LIMIT) {
			debug_value = debug_value.left(DISPLAY_LIMIT) + "... " + TTR("(truncated)");
		}
		debug_value = TTR("Current value: ") + debug_value;
	}

	return debug_value;
}

void DocumentEditorContainer::_resource_created(Ref<Resource> p_res) {
	EditorNode::get_singleton()->push_item(p_res.ptr());
}

void DocumentEditorContainer::_set_breakpoint(Ref<RefCounted> p_script, int p_line, bool p_enabled) {
	Ref<Script> scr = Object::cast_to<Script>(*p_script);
	if (scr.is_valid() && (scr->has_source_code() || scr->get_path().is_resource_file())) {
		// Update if open.
		for (int i = 0; i < _tab_container->get_tab_count(); i++) {
			CodeEditorBase *ceb = Object::cast_to<CodeEditorBase>(_tab_container->get_tab_control(i));
			if (ceb && ceb->get_edited_resource()->get_path() == scr->get_path()) {
				ceb->set_breakpoint(p_line, p_enabled);
				return;
			}
		}

		// Handle closed.
		Dictionary state = _script_editor_cache->get_value(scr->get_path(), "state");
		Array breakpoints;
		if (state.has("breakpoints")) {
			breakpoints = state["breakpoints"];
		}

		if (breakpoints.has(p_line)) {
			if (!p_enabled) {
				breakpoints.erase(p_line);
			}
		} else if (p_enabled) {
			breakpoints.push_back(p_line);
		}
		state["breakpoints"] = breakpoints;
		_script_editor_cache->set_value(scr->get_path(), "state", state);
		EditorDebuggerNode::get_singleton()->set_breakpoint(scr->get_path(), p_line + 1, p_enabled);
	}
}

void DocumentEditorContainer::_clear_breakpoints() {
	for (int i = 0; i < _tab_container->get_tab_count(); i++) {
		if (CodeEditorBase *ceb = Object::cast_to<CodeEditorBase>(_tab_container->get_tab_control(i))) {
			ceb->clear_breakpoints();
		}
	}

	// Clear from closed scripts.
	Vector<String> cached_editors = _script_editor_cache->get_sections();
	for (const String &E : cached_editors) {
		Array breakpoints = _get_cached_breakpoints_for_script(E);
		for (int breakpoint : breakpoints) {
			EditorDebuggerNode::get_singleton()->set_breakpoint(E, (int)breakpoint + 1, false);
		}

		if (breakpoints.size() > 0) {
			Dictionary state = _script_editor_cache->get_value(E, "state");
			state["breakpoints"] = Array();
			_script_editor_cache->set_value(E, "state", state);
		}
	}
}

Array DocumentEditorContainer::_get_cached_breakpoints_for_script(const String &p_path) const {
	if (!ResourceLoader::exists(p_path, "Script") || p_path.begins_with("local://") || !_script_editor_cache->has_section_key(p_path, "state")) {
		return Array();
	}

	Dictionary state = _script_editor_cache->get_value(p_path, "state");
	if (!state.has("breakpoints")) {
		return Array();
	}
	return state["breakpoints"];
}

ScriptEditorBase *DocumentEditorContainer::_get_current_editor() const {
	return Object::cast_to<ScriptEditorBase>(_tab_container->get_current_tab_control());
}

void DocumentEditorContainer::_save_editor_state(ScriptEditorBase *p_editor) {
	if (_restoring_layout) {
		return;
	}

	const String &path = p_editor->get_edited_resource()->get_path();
	if (path.is_empty()) {
		return;
	}

	if (TextEditorBase *teb = Object::cast_to<TextEditorBase>(p_editor)) {
		_script_editor_cache->set_value(path, "state", teb->get_edit_state());
	}
	// This is saved later when we save the editor layout.
}

void DocumentEditorContainer::_save_layout() {
	if (_restoring_layout) {
		return;
	}

	EditorNode::get_singleton()->save_editor_layout_delayed();
}

void DocumentEditorContainer::_apply_editor_settings() {
	_textfile_extensions.clear();
	const Vector<String> textfile_ext = ((String)(EDITOR_GET("docks/filesystem/textfile_extensions"))).split(",", false);
	for (const String &E : textfile_ext) {
		_textfile_extensions.insert(E);
	}

	_trim_trailing_whitespace_on_save = EDITOR_GET("text_editor/behavior/files/trim_trailing_whitespace_on_save");
	_trim_final_newlines_on_save = EDITOR_GET("text_editor/behavior/files/trim_final_newlines_on_save");
	_convert_indent_on_save = EDITOR_GET("text_editor/behavior/files/convert_indent_on_save");

	_document_outline->update_editor_settings();

	_update_autosave_timer();

	DocumentNameDisplayMode previous_document_display_name_mode = _document_display_name_mode;
	_document_display_name_mode = (DocumentNameDisplayMode)(int)EDITOR_GET("text_editor/script_list/list_script_names_as");
	if (previous_document_display_name_mode != _document_display_name_mode) {
		_update_filenames();
	}

	DocumentSortMode previous_document_sort_by_mode = _document_sort_by_mode;
	bool previous_document_sort_group_help_pages = _document_sort_group_help_pages;
	_document_sort_by_mode = (DocumentSortMode)(int)EDITOR_GET("text_editor/script_list/sort_scripts_by");
	_document_sort_group_help_pages = EDITOR_GET("text_editor/script_list/group_help_pages");
	if (_document_sort_by_mode != previous_document_sort_by_mode || _document_sort_group_help_pages != previous_document_sort_group_help_pages) {
		sort_document_editors();
	}

	bool previous_highlight_scene_scripts = _highlight_scene_scripts;
	_highlight_scene_scripts = EDITOR_GET("text_editor/script_list/highlight_scene_scripts");
	if (_highlight_scene_scripts && !previous_highlight_scene_scripts) {
		_connect_to_scene();
	}

	_document_list->update_editor_settings();

	ScriptServer::set_reload_scripts_on_save(EDITOR_GET("text_editor/behavior/files/auto_reload_and_parse_scripts_on_save"));

	for (int i = 0; i < _tab_container->get_tab_count(); i++) {
		if (TextEditorBase *teb = Object::cast_to<TextEditorBase>(_tab_container->get_tab_control(i))) {
			teb->update_settings();
		}
	}
}

void DocumentEditorContainer::_update_filenames() {
	if (_restoring_layout) {
		return;
	}

	// Disambiguate filenames.
	const int num_open_editors = _tab_container->get_tab_count();

	PackedStringArray disambiguated_names;
	disambiguated_names.reserve(num_open_editors);
	PackedStringArray full_paths;
	full_paths.reserve(num_open_editors);

	LocalVector<bool> unsaved;
	unsaved.reserve(num_open_editors);

	for (int i = 0; i < num_open_editors; i++) {
		Control *editor = _tab_container->get_tab_control(i);
		ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(editor);
		EditorHelp *eh = Object::cast_to<EditorHelp>(editor);
		if (seb) {
			unsaved.push_back(seb->is_unsaved());

			const String &path = seb->get_edited_resource()->get_path();
			const String &name = seb->get_document_name();
			String display_name;

			switch (_document_display_name_mode) {
				case DISPLAY_NAME: {
					display_name = name;
				} break;
				case DISPLAY_DIR_AND_NAME: {
					if (!path.get_base_dir().get_file().is_empty()) {
						display_name = path.get_base_dir().get_file().path_join(name);
					} else {
						display_name = name;
					}
				} break;
				case DISPLAY_FULL_PATH: {
					display_name = path;
				} break;
			}
			full_paths.push_back(path);
			disambiguated_names.push_back(display_name);
		}
		if (eh) {
			const String name = eh->get_class().is_empty() ? TTR("Unknown Help Class") : eh->get_class().unquote();
			unsaved.push_back(false);

			full_paths.push_back(name);
			disambiguated_names.push_back(name);
		}
	}

	EditorNode::disambiguate_filenames(full_paths, disambiguated_names);

	for (int i = 0; i < disambiguated_names.size(); i++) {
		String name = disambiguated_names[i];
		if (unsaved[i]) {
			name += "(*)";
		}
		_tab_container->set_tab_title(i, name);
		_tab_container->get_tab_control(i)->set_name(name);
	}

	_document_list->update_list();
	_update_document_name_button();
}

void DocumentEditorContainer::_files_moved(const String &p_old_file, const String &p_new_file) {
	if (!_script_editor_cache->has_section(p_old_file)) {
		return;
	}

	for (int i = 0; i < _tab_container->get_tab_count(); i++) {
		ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(_tab_container->get_tab_control(i));
		if (seb && seb->edited_file_data.path == p_old_file) {
			seb->edited_file_data.path = p_new_file;
			break;
		}
	}

	Variant state = _script_editor_cache->get_value(p_old_file, "state");
	_script_editor_cache->erase_section(p_old_file);
	_script_editor_cache->set_value(p_new_file, "state", state);

	// If Script, update breakpoints with debugger.
	Array breakpoints = _get_cached_breakpoints_for_script(p_new_file);
	for (int breakpoint : breakpoints) {
		int line = (int)breakpoint + 1;
		EditorDebuggerNode::get_singleton()->set_breakpoint(p_old_file, line, false);
		if (!p_new_file.begins_with("local://") && ResourceLoader::exists(p_new_file, "Script")) {
			EditorDebuggerNode::get_singleton()->set_breakpoint(p_new_file, line, true);
		}
	}
	// This is saved later when we save the editor layout.
}

void DocumentEditorContainer::_file_removed(const String &p_removed_file) {
	for (int i = 0; i < _tab_container->get_tab_count(); i++) {
		ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(_tab_container->get_tab_control(i));
		if (seb && seb->edited_file_data.path == p_removed_file) {
			// The script is deleted with no undo, so just close the tab.
			_close_tab(i, false);
		}
	}

	// Check closed.
	if (_script_editor_cache->has_section(p_removed_file)) {
		Array breakpoints = _get_cached_breakpoints_for_script(p_removed_file);
		for (int breakpoint : breakpoints) {
			EditorDebuggerNode::get_singleton()->set_breakpoint(p_removed_file, (int)breakpoint + 1, false);
		}
		_script_editor_cache->erase_section(p_removed_file);
	}
}

void DocumentEditorContainer::_update_autosave_timer() {
	if (!_autosave_timer->is_inside_tree()) {
		return;
	}

	float autosave_time = EDITOR_GET("text_editor/behavior/files/autosave_interval_secs");
	if (autosave_time > 0) {
		_autosave_timer->set_wait_time(autosave_time);
		_autosave_timer->start();
	} else {
		_autosave_timer->stop();
	}
}

void DocumentEditorContainer::_reload_scripts(bool p_refresh_only) {
	for (int i = 0; i < _tab_container->get_tab_count(); i++) {
		ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(_tab_container->get_tab_control(i));
		if (!seb) {
			continue;
		}

		Ref<Resource> edited_res = seb->get_edited_resource();

		if (edited_res->is_built_in()) {
			continue; // Internal script, who cares.
		}

		if (p_refresh_only) {
			// Make sure the modified time is correct.
			seb->edited_file_data.last_modified_time = FileAccess::get_modified_time(edited_res->get_path());
		} else {
			uint64_t last_date = seb->edited_file_data.last_modified_time;
			uint64_t date = FileAccess::get_modified_time(edited_res->get_path());

			if (last_date == date) {
				continue;
			}
			seb->edited_file_data.last_modified_time = date;

			Ref<Script> scr = edited_res;
			if (scr.is_valid()) {
				Ref<Script> rel_scr = ResourceLoader::load(scr->get_path(), scr->get_class(), ResourceFormatLoader::CACHE_MODE_IGNORE);
				ERR_CONTINUE(rel_scr.is_null());
				scr->set_source_code(rel_scr->get_source_code());
				scr->reload(true);

				update_docs_from_script(scr);
			}

			Ref<JSON> json = edited_res;
			if (json.is_valid()) {
				Ref<JSON> rel_json = ResourceLoader::load(json->get_path(), json->get_class(), ResourceFormatLoader::CACHE_MODE_IGNORE);
				ERR_CONTINUE(rel_json.is_null());
				json->parse(rel_json->get_parsed_text(), true);
			}

			Ref<TextFile> text_file = edited_res;
			if (text_file.is_valid()) {
				text_file->reload_from_file();
			}

			ShaderTextEditor *ste = Object::cast_to<ShaderTextEditor>(seb);
			if (ste) {
				Ref<Shader> shader = edited_res;
				Ref<ShaderInclude> shader_inc = edited_res;
				String new_code;
				if (shader.is_valid()) {
					Ref<Shader> rel_sh = ResourceLoader::load(shader->get_path(), shader->get_class(), ResourceFormatLoader::CACHE_MODE_IGNORE);
					ERR_CONTINUE(rel_sh.is_null());
					new_code = rel_sh->get_code();
				} else if (shader_inc.is_valid()) {
					Ref<ShaderInclude> rel_sh_inc = ResourceLoader::load(shader_inc->get_path(), shader_inc->get_class(), ResourceFormatLoader::CACHE_MODE_IGNORE);
					ERR_CONTINUE(rel_sh_inc.is_null());
					new_code = rel_sh_inc->get_code();
				}
				ste->set_code_block_changed(new_code);
			}
		}

		if (TextEditorBase *teb = Object::cast_to<TextEditorBase>(seb)) {
			Dictionary state = teb->get_edit_state();
			teb->reload_text();
			teb->set_edit_state(state);
		}
	}

	_disk_changed->hide();
}

void DocumentEditorContainer::_auto_format_text(ScriptEditorBase *p_seb) {
	if (TextEditorBase *teb = Object::cast_to<TextEditorBase>(p_seb)) {
		if (_trim_trailing_whitespace_on_save) {
			teb->trim_trailing_whitespace();
		}
		if (_trim_final_newlines_on_save) {
			teb->trim_final_newlines();
		}
		if (_convert_indent_on_save) {
			teb->convert_indent();
		}
	}
}

void DocumentEditorContainer::_connect_to_scene() {
	if (!_highlight_scene_scripts) {
		return;
	}
	Node *edited_scene = EditorNode::get_singleton()->get_edited_scene();
	if (!edited_scene) {
		return;
	}
	_connect_to_scene_recursive(edited_scene);
}

void DocumentEditorContainer::_connect_to_scene_recursive(Node *p_current) {
	_queue_update_list();
	const Callable update_callable = callable_mp(this, &DocumentEditorContainer::_queue_update_list);
	if (p_current->is_connected(CoreStringName(script_changed), update_callable)) {
		return;
	}
	p_current->connect(CoreStringName(script_changed), update_callable);
	p_current->connect(SceneStringName(tree_exited), update_callable);
	// Use `child_entered_tree` to recursively connect to children.
	p_current->connect(SNAME("child_entered_tree"), callable_mp(this, &DocumentEditorContainer::_connect_to_scene_recursive));
}

void DocumentEditorContainer::_update_document_list() {
	_document_list->update_list();
	_list_update_queued = false;
}

void DocumentEditorContainer::_queue_update_list() {
	if (_list_update_queued || !_highlight_scene_scripts) {
		return;
	}
	_list_update_queued = true;
	callable_mp(this, &DocumentEditorContainer::_update_document_list).call_deferred();
}

void DocumentEditorContainer::_update_online_doc() {
	Node *current = _tab_container->get_current_tab_control();

	EditorHelp *eh = Object::cast_to<EditorHelp>(current);
	bool native_class_doc = false;
	if (eh) {
		const HashMap<String, DocData::ClassDoc>::ConstIterator E = EditorHelp::get_doc_data()->class_list.find(eh->get_class());
		native_class_doc = E && !E->value.is_script_doc;
	}
	if (native_class_doc) {
		String name = eh->get_class();
		String tooltip = vformat(TTR("Open '%s' in Godot online documentation."), name);
		_site_search_btn->set_text(TTRC("Open in Online Docs"));
		_site_search_btn->set_tooltip_text(tooltip);
	} else {
		_site_search_btn->set_text(TTRC("Online Docs"));
		_site_search_btn->set_tooltip_text(TTRC("Open Godot online documentation."));
	}
}

void DocumentEditorContainer::input(const Ref<InputEvent> &p_event) {
	// This feature can be disabled to avoid interfering with other uses of the additional
	// mouse buttons, such as push-to-talk in a VoIP program.
	if (EDITOR_GET("interface/editor/input/mouse_extra_buttons_navigate_history")) {
		const Ref<InputEventMouseButton> mb = p_event;

		// Navigate the script history using additional mouse buttons present on some mice.
		// This must be hardcoded as the editor shortcuts dialog doesn't allow assigning
		// more than one shortcut per action.
		Control *focus_owner = get_viewport()->gui_get_focus_owner();
		if (focus_owner && (this == focus_owner || is_ancestor_of(focus_owner))) {
			if (mb.is_valid() && mb->is_pressed() && is_visible_in_tree()) {
				if (mb->get_button_index() == MouseButton::MB_XBUTTON1) {
					_history_back();
				}

				if (mb->get_button_index() == MouseButton::MB_XBUTTON2) {
					_history_forward();
				}
			}
		}
	}
}

void DocumentEditorContainer::shortcut_input(const Ref<InputEvent> &p_event) {
	ERR_FAIL_COND(p_event.is_null());

	if (!is_visible_in_tree() || !p_event->is_pressed()) {
		return;
	}

	Control *focus_owner = get_viewport()->gui_get_focus_owner();
	if (!(focus_owner && (this == focus_owner || is_ancestor_of(focus_owner)))) {
		return;
	}

	if (ED_IS_SHORTCUT("script_editor/next_script", p_event)) {
		_document_list->goto_next_document();
		accept_event();
	}
	if (ED_IS_SHORTCUT("script_editor/prev_script", p_event)) {
		_document_list->goto_next_document(true);
		accept_event();
	}

	static const HashMap<String, int> shortcut_mappings = {
		{ "script_editor/close_other_tabs", FILE_MENU_CLOSE_OTHER_TABS },
		{ "script_editor/close_tabs_below", FILE_MENU_CLOSE_TABS_BELOW },
		{ "script_editor/close_docs", FILE_MENU_CLOSE_DOCS },
		{ "script_editor/copy_path", FILE_MENU_COPY_PATH },
		{ "script_editor/copy_uid", FILE_MENU_COPY_UID },
		{ "script_editor/show_in_file_system", FILE_MENU_SHOW_IN_FILE_SYSTEM },
		{ "script_editor/move_document_up", FILE_MENU_MOVE_UP },
		{ "script_editor/move_document_down", FILE_MENU_MOVE_DOWN },
		{ "script_editor/sort_documents", FILE_MENU_SORT },
	};

	for (const KeyValue<String, int> &E : shortcut_mappings) {
		if (ED_IS_SHORTCUT(E.key, p_event)) {
			_menu_option(E.value);
			accept_event();
			break;
		}
	}

	if (p_event->is_echo()) {
		return;
	}

	const Callable custom_callback = EditorContextMenuPluginManager::get_singleton()->match_custom_shortcut(EditorContextMenuPlugin::CONTEXT_SLOT_SCRIPT_EDITOR, p_event);
	if (custom_callback.is_valid()) {
		EditorContextMenuPlugin::OptionsData context_data = DocumentEditorContainer::get_context_data(_tab_container->get_current_tab_control());
		EditorContextMenuPluginManager::get_singleton()->invoke_callback(custom_callback, context_data);
		accept_event();
	}
}

void DocumentEditorContainer::_prepare_file_menu() {
	PopupMenu *p_menu = _file_menu->get_popup();
	p_menu->set_item_disabled(p_menu->get_item_index(FILE_MENU_REOPEN_CLOSED), _previous_scripts.is_empty());

	ScriptEditorBase *seb = _get_current_editor();
	const Ref<Resource> res = seb ? seb->get_edited_resource() : Ref<Resource>();
	if (res.is_valid()) {
		p_menu->set_item_disabled(p_menu->get_item_index(FILE_MENU_SAVE), res.is_null());
		p_menu->set_item_disabled(p_menu->get_item_index(FILE_MENU_SAVE_AS), res.is_null());
	}
	p_menu->set_item_disabled(p_menu->get_item_index(FILE_MENU_SAVE_ALL), !_has_script_tab());
	p_menu->set_item_disabled(p_menu->get_item_index(FILE_MENU_CLOSE), _tab_container->get_tab_count() < 1);
	p_menu->set_item_disabled(p_menu->get_item_index(FILE_MENU_CLOSE_ALL), _tab_container->get_tab_count() < 1);
	const Ref<Script> script = res;
	p_menu->set_item_disabled(p_menu->get_item_index(FILE_MENU_SOFT_RELOAD_TOOL), script.is_null() || !script->is_tool());
	p_menu->set_item_disabled(p_menu->get_item_index(FILE_MENU_RUN), script.is_null());
}

void DocumentEditorContainer::_file_menu_closed() {
	PopupMenu *menu = _file_menu->get_popup();
	for (int i = 0; i < menu->get_item_count(); i++) {
		menu->set_item_disabled(i, false);
	}
}

void DocumentEditorContainer::_calculate_script_name_button_size() {
	Ref<Font> font = _script_name_btn->get_theme_font(SceneStringName(font), SNAME("Button"));
	HorizontalAlignment alignment = _script_name_btn->get_text_alignment();
	int font_size = _script_name_btn->get_theme_font_size(SceneStringName(font_size), SNAME("Button"));
	String text = _script_name_btn->get_text();
	int jst_flags = TextServer::JUSTIFICATION_WORD_BOUND | TextServer::JUSTIFICATION_KASHIDA | TextServer::JUSTIFICATION_SKIP_LAST_LINE | TextServer::JUSTIFICATION_DO_NOT_SKIP_SINGLE_LINE;
	Vector2 text_size = font->get_string_size(text, alignment, -1, font_size, jst_flags, TextServer::ORIENTATION_HORIZONTAL);

	_script_name_width = text_size.x + _script_name_btn->get_theme_stylebox(CoreStringName(normal))->get_content_inset(SIDE_LEFT) + _script_name_btn->get_theme_stylebox(CoreStringName(normal))->get_content_inset(SIDE_RIGHT);
	_calculate_script_name_button_ratio();
}

void DocumentEditorContainer::_calculate_script_name_button_ratio() {
	const float total_width = _script_name_button_hbox->get_size().width;
	if (total_width <= 0) {
		return;
	}

	// Make the ratios a fraction bigger, to avoid unnecessary trimming.
	const float extra_ratio = 4 / total_width;

	const float script_name_ratio = MIN(1, _script_name_width / total_width + extra_ratio);
	_script_name_btn->set_stretch_ratio(script_name_ratio);

	float ratio_left = 1 - script_name_ratio;
	_script_name_button_left_spacer->set_stretch_ratio(ratio_left / 2);
	_script_name_button_right_spacer->set_stretch_ratio(ratio_left / 2);
}

void DocumentEditorContainer::_update_document_name_button() {
	if (_tab_container->get_current_tab() >= 0) {
		const String &name = _tab_container->get_tab_title(_tab_container->get_current_tab());
		_script_name_btn->set_text(name);
		_script_name_btn->show();
		_calculate_script_name_button_size();
	} else {
		_script_name_btn->hide();
		_script_name_btn->set_text(String());
	}
}

void DocumentEditorContainer::_help_search(const String &p_text) {
	ScriptEditorDock::get_singleton()->open_help_search_dialog(p_text);
}

void DocumentEditorContainer::_history_forward() {
	if (_history_pos < _history.size() - 1) {
		ScriptEditorNavigationMarker::get_singleton()->traverse_begin();
		_update_history_pos(_history_pos + 1);
		ScriptEditorNavigationMarker::get_singleton()->traverse_end();
	}
}

void DocumentEditorContainer::_history_back() {
	if (_history_pos > 0) {
		ScriptEditorNavigationMarker::get_singleton()->traverse_begin();
		_update_history_pos(_history_pos - 1);
		ScriptEditorNavigationMarker::get_singleton()->traverse_end();
	}
}

void DocumentEditorContainer::_roll_back_to_pre_tab() {
	Control *tselected = _tab_container->get_current_tab_control();
	if (_history_pos == -1 || _history[_history_pos].control != tselected) {
		return;
	}

	int pos = -1;
	for (int i = _history_pos - 1; i >= 0; i--) {
		if (_history[i].control != tselected) {
			pos = i;
			break;
		}
	}

	if (pos == -1) {
		_history_pos = -1;
		_history.clear();
	} else {
		_update_history_pos(pos);
		_history.resize(_history_pos + 1);
	}
}

void DocumentEditorContainer::_help_class_open(const String &p_class) {
	if (p_class.is_empty()) {
		return;
	}

	for (int i = 0; i < _tab_container->get_tab_count(); i++) {
		EditorHelp *eh = Object::cast_to<EditorHelp>(_tab_container->get_tab_control(i));

		if (eh && eh->get_class() == p_class) {
			_go_to_tab(i, true);
			return;
		}
	}

	EditorHelp *eh = memnew(EditorHelp);

	eh->connect("go_to_help", callable_mp(this, &DocumentEditorContainer::_help_class_goto));
	eh->connect("_request_save_new_history", callable_mp(this, &DocumentEditorContainer::_save_new_history).bind(eh));

	// Go to the class first, so it has a class name when switching to it.
	ScriptEditorNavigationMarker::get_singleton()->locate_begin();
	eh->go_to_class(p_class);
	ScriptEditorNavigationMarker::get_singleton()->locate_end();

	_tab_container->add_child(eh);
	_update_filenames();
	_go_to_tab(_tab_container->get_tab_count() - 1);
	sort_document_editors();
	_add_recent_script(p_class);
	_save_layout();
}

void DocumentEditorContainer::_help_class_goto(const String &p_desc) {
	const String cname = p_desc.get_slicec(':', 1);

	for (int i = 0; i < _tab_container->get_tab_count(); i++) {
		EditorHelp *eh = Object::cast_to<EditorHelp>(_tab_container->get_tab_control(i));

		if (eh && eh->get_class() == cname) {
			ScriptEditorNavigationMarker::get_singleton()->locate_begin();
			eh->go_to_help(p_desc);
			ScriptEditorNavigationMarker::get_singleton()->locate_end();
			_go_to_tab(i);
			return;
		}
	}

	EditorHelp *eh = memnew(EditorHelp);

	eh->connect("go_to_help", callable_mp(this, &DocumentEditorContainer::_help_class_goto));
	eh->connect("_request_save_new_history", callable_mp(this, &DocumentEditorContainer::_save_new_history).bind(eh));

	ScriptEditorNavigationMarker::get_singleton()->locate_begin();
	eh->go_to_help(p_desc);
	ScriptEditorNavigationMarker::get_singleton()->locate_end();

	_tab_container->add_child(eh);
	_update_filenames();
	_go_to_tab(_tab_container->get_tab_count() - 1);
	sort_document_editors();
	_add_recent_script(eh->get_class());
	_save_layout();
}

void DocumentEditorContainer::_update_history_arrows() {
	_script_back_btn->set_disabled(_history_pos <= 0);
	_script_forward_btn->set_disabled(_history_pos >= _history.size() - 1);
}

void DocumentEditorContainer::_save_new_history(const Dictionary &p_state, Control *p_control) {
	if (_restoring_layout) {
		return;
	}

	if (_history_pos >= 0 && _history_pos < _history.size()) {
		if (_history[_history_pos].control == p_control) {
			if (_history[_history_pos].state == p_state) {
				return;
			}
			if (p_state["row"] == _history[_history_pos].state["row"]) {
				_save_previous_state(p_state, p_control);
				return;
			}
		}
	}

	ScriptHistory sh;
	sh.control = p_control;
	sh.state = p_state;

	_history.resize(_history_pos + 1);
	_history.push_back(sh);
	_compress_history_patterns(true);

	for (int i = _history.size() - 1; i >= 0; i--) {
		if (_history[i].control == _tab_container->get_current_tab_control()) {
			_history_pos = i;
			break;
		}
	}

	_update_history_arrows();
}

void DocumentEditorContainer::_save_previous_state(const Dictionary &p_state, Control *p_control) {
	if (_history_pos < 0 || _history_pos >= _history.size()) {
		return;
	}
	ScriptHistory sh = _history[_history_pos];
	if (sh.control != p_control) {
		return;
	}
	_history.write[_history_pos].state = p_state;
}

// Compress the history and remove duplicate patterns.
// Example 1: If the history is ...ABAB..., it will be compressed to ...AB....
// Example 2: If the history is ...ABCABC..., it will be compressed to ...ABC....
void DocumentEditorContainer::_compress_history_patterns(bool p_once) {
	bool stop = false;
	bool changed = true;
	int iterations = 0;

	while (!stop && changed && _history.size() > 1 && iterations++ < 100) {
		changed = false;
		for (int end_idx = _history.size() - 1; end_idx >= 1; end_idx--) {
			bool found_duplicate = false;
			int max_possible_len = (end_idx + 1) / 2;

			for (int len = 1; len <= max_possible_len; len++) {
				// Compare [first_start, first_start + len) and [second_start, second_start + len)
				int second_start = end_idx - len + 1;
				int first_start = second_start - len;

				if (first_start < 0) {
					continue;
				}

				bool is_match = true;

				for (int k = 0; k < len; k++) {
					const ScriptHistory &h1 = _history[first_start + k];
					const ScriptHistory &h2 = _history[second_start + k];

					if (h1.control != h2.control || h1.state["row"] != h2.state["row"]) {
						is_match = false;
						break;
					}
				}

				if (is_match) {
					for (int r = 0; r < len; r++) {
						_history.remove_at(first_start);
					}

					if (_history_pos >= second_start) {
						_history_pos -= len;
					} else if (first_start <= _history_pos && _history_pos < second_start) {
						_history_pos = first_start;
					}

					found_duplicate = true;
					changed = true;
					stop = p_once;
					break;
				}
			}

			if (found_duplicate) {
				break;
			}
		}
	}

	if (_history.is_empty()) {
		_history_pos = -1;
	} else {
		if (_history_pos >= _history.size()) {
			_history_pos = _history.size() - 1;
		}
		if (_history_pos < -1) {
			_history_pos = -1;
		}
	}
}

void DocumentEditorContainer::_go_to_tab(int p_idx, bool p_save_history) {
	if (_restoring_layout) {
		return;
	}
	if (ScriptEditorBase *current = _get_current_editor()) {
		if (current->is_unsaved()) {
			current->apply_code();
		}
	}

	Control *c = _tab_container->get_tab_control(p_idx);
	if (!c) {
		return;
	}

	_tab_container->set_current_tab(p_idx);

	c = _tab_container->get_current_tab_control();

	if (ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(c)) {
		if (is_visible_in_tree() && !_grab_focus_block) {
			seb->ensure_focus();
		}

		TextEditorBase *teb = Object::cast_to<TextEditorBase>(c);
		if (p_save_history && teb) {
			ScriptEditorNavigationMarker::get_singleton()->locate_begin();
			teb->trigger_history_save_on_navigate();
			ScriptEditorNavigationMarker::get_singleton()->locate_end();
		}

		Ref<Script> scr = seb->get_edited_resource();
		if (scr.is_valid()) {
			ScriptEditorDock::get_singleton()->notify_script_changed(scr);
		}

		seb->validate_script();
	}

	if (EditorHelp *eh = Object::cast_to<EditorHelp>(c)) {
		if (is_visible_in_tree()) {
			eh->set_focused();
		}

		if (p_save_history) {
			ScriptEditorNavigationMarker::get_singleton()->locate_begin();
			eh->trigger_history_save_on_navigate();
			ScriptEditorNavigationMarker::get_singleton()->locate_end();
		}
	}

	c->set_meta("__editor_pass", ++_edit_pass);
	_update_history_arrows();
}

void DocumentEditorContainer::_update_history_pos(int p_new_pos) {
	Node *n = _tab_container->get_current_tab_control();
	_history_pos = p_new_pos;
	_tab_container->set_current_tab(_tab_container->get_tab_idx_from_control(_history[_history_pos].control));

	n = _history[_history_pos].control;

	if (ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(n)) {
		if (TextEditorBase *teb = Object::cast_to<TextEditorBase>(n)) {
			teb->set_edit_state(_history[_history_pos].state);
		}
		seb->ensure_focus();

		Ref<Script> scr = seb->get_edited_resource();
		if (scr.is_valid()) {
			ScriptEditorDock::get_singleton()->notify_script_changed(scr);
		}

		seb->validate_script();
	}

	if (EditorHelp *eh = Object::cast_to<EditorHelp>(n)) {
		eh->set_scroll(_history[_history_pos].state["row"]);
		eh->set_focused();
	}

	n->set_meta("__editor_pass", ++_edit_pass);
	_update_history_arrows();
}

void DocumentEditorContainer::_update_modified_scripts_for_external_editor(Ref<Script> p_for_script) {
	if (!bool(EDITOR_GET("text_editor/external/use_external_editor"))) {
		return;
	}

	ERR_FAIL_NULL(get_tree());

	HashSet<Ref<Script>> scripts;

	Node *base = get_tree()->get_edited_scene_root();
	if (base) {
		_find_changed_scripts_for_external_editor(base, base, scripts);
	}

	for (const Ref<Script> &E : scripts) {
		Ref<Script> scr = E;

		if (p_for_script.is_valid() && p_for_script != scr) {
			continue;
		}

		if (scr->is_built_in()) {
			continue; //internal script, who cares, though weird
		}

		uint64_t last_date = scr->get_last_modified_time();
		uint64_t date = FileAccess::get_modified_time(scr->get_path());

		if (last_date != date) {
			Ref<Script> rel_scr = ResourceLoader::load(scr->get_path(), scr->get_class(), ResourceFormatLoader::CACHE_MODE_IGNORE);
			ERR_CONTINUE(rel_scr.is_null());
			scr->set_source_code(rel_scr->get_source_code());
			scr->set_last_modified_time(rel_scr->get_last_modified_time());
			scr->update_exports();

			trigger_live_script_reload(scr->get_path());
		}
	}
}

void DocumentEditorContainer::_validation_updated() {
	_document_list->update_list();

	_document_outline->update_outline();
	_document_outline->update_visibility();
}

void DocumentEditorContainer::_file_dialog_action(const String &p_file) {
	switch (_file_dialog_option) {
		case FILE_MENU_NEW_TEXTFILE: {
			Error err;
			{
				Ref<FileAccess> file = FileAccess::open(p_file, FileAccess::WRITE, &err);
				if (err) {
					EditorNode::get_singleton()->show_warning(TTR("Error writing TextFile:") + "\n" + p_file, TTR("Error!"));
					break;
				}
			}

			if (EditorFileSystem::get_singleton()) {
				if (_textfile_extensions.has(p_file.get_extension())) {
					EditorFileSystem::get_singleton()->update_file(p_file);
				}
			}
			[[fallthrough]];
		}
		case FILE_MENU_OPEN: {
			open_file(p_file);
		} break;
		case FILE_MENU_SAVE_AS: {
			if (ScriptEditorBase *current = _get_current_editor()) {
				Ref<Resource> resource = current->get_edited_resource();
				String path = ProjectSettings::get_singleton()->localize_path(p_file);
				Error err = _save_text_file(resource, path);

				if (err != OK) {
					EditorNode::get_singleton()->show_warning(TTR("Error saving file!"));
					return;
				}

				resource->set_path(path);
				_update_filenames();
			}
		} break;
		case THEME_SAVE_AS: {
			_save_text_editor_theme_as(p_file);
		} break;
		case THEME_IMPORT: {
			_import_text_editor_theme(p_file);
		} break;
	}
	_file_dialog_option = -1;
}

List<String> DocumentEditorContainer::_get_recognized_extensions() {
	List<String> extensions;
	for (const String &type : _handled_resource_types) {
		ResourceLoader::get_recognized_extensions_for_type(type, &extensions);
	}
	return extensions;
}

Ref<TextFile> DocumentEditorContainer::_load_text_file(const String &p_path, Error *r_error) const {
	if (r_error) {
		*r_error = ERR_FILE_CANT_OPEN;
	}

	String local_path = ProjectSettings::get_singleton()->localize_path(p_path);
	String path = ResourceLoader::path_remap(local_path);

	// Reuse an already-open TextFile resource to avoid creating duplicates.
	// This prevents opening the same file multiple times as "unsaved" tabs.
	for (int i = 0; i < _tab_container->get_tab_count(); i++) {
		ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(_tab_container->get_tab_control(i));
		if (!seb) {
			continue;
		}

		if (seb->edited_file_data.path == local_path) {
			Ref<TextFile> existing_text_file = seb->get_edited_resource();
			if (existing_text_file.is_valid()) {
				if (r_error) {
					*r_error = OK;
				}
				return existing_text_file;
			}
		}
	}

	Ref<TextFile> text_file;
	text_file.instantiate();
	Error err = text_file->load_text(path);

	ERR_FAIL_COND_V_MSG(err != OK, Ref<Resource>(), "Cannot load text file '" + path + "'.");

	text_file->set_file_path(local_path);
	text_file->set_path(local_path, true);

	if (ResourceLoader::get_timestamp_on_load()) {
		text_file->set_last_modified_time(FileAccess::get_modified_time(path));
	}

	if (r_error) {
		*r_error = OK;
	}

	return text_file;
}

Error DocumentEditorContainer::_save_text_file(Ref<TextFile> p_text_file, const String &p_path) {
	Ref<TextFile> sqscr = p_text_file;
	ERR_FAIL_COND_V(sqscr.is_null(), ERR_INVALID_PARAMETER);

	String source = sqscr->get_text();

	Error err;
	{
		Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE, &err);

		ERR_FAIL_COND_V_MSG(err, err, "Cannot save text file '" + p_path + "'.");

		file->store_string(source);
		if (file->get_error() != OK && file->get_error() != ERR_FILE_EOF) {
			return ERR_CANT_CREATE;
		}
	}

	if (ResourceSaver::get_timestamp_on_save()) {
		p_text_file->set_last_modified_time(FileAccess::get_modified_time(p_path));
	}

	EditorFileSystem::get_singleton()->update_file(p_path);

	_res_saved_callback(sqscr);
	return OK;
}

void DocumentEditorContainer::_set_script_zoom_factor(float p_zoom_factor) {
	if (_zoom_factor == p_zoom_factor) {
		return;
	}

	_zoom_factor = p_zoom_factor;
}

void DocumentEditorContainer::_update_code_editor_zoom_factor(CodeTextEditor *p_code_text_editor) {
	if (p_code_text_editor && p_code_text_editor->is_visible_in_tree() && _zoom_factor != p_code_text_editor->get_zoom_factor()) {
		p_code_text_editor->set_zoom_factor(_zoom_factor);
	}
}

void DocumentEditorContainer::_close_builtin_scripts_from_scene(const String &p_scene) {
	for (int i = 0; i < _tab_container->get_tab_count(); i++) {
		if (ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(_tab_container->get_tab_control(i))) {
			Ref<Resource> res = seb->get_edited_resource();
			if (res.is_valid() && res->is_built_in() && res->get_path().begins_with(p_scene)) { // Is an internal script and belongs to scene being closed.
				_close_tab(i, false);
				i--;
			}
		}
	}
}

void DocumentEditorContainer::_toggle_files_pressed(bool p_pressed) {
	ERR_FAIL_NULL(_list_split);
	_list_split->set_visible(p_pressed);
	EditorSettings::get_singleton()->set_project_metadata("show_files_panel", _config_section, _list_split->is_visible());
}
