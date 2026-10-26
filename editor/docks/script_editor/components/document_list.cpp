/**************************************************************************/
/*  document_list.cpp                                                     */
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

#include "document_list.h"

#include "core/io/resource_loader.h"
#include "core/io/resource_uid.h"
#include "core/object/callable_mp.h"
#include "core/object/script_language.h"
#include "core/string/fuzzy_search.h"
#include "editor/doc/editor_help.h"
#include "editor/docks/script_editor/components/document_editor_container.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/inspector/editor_context_menu_plugin.h"
#include "editor/script/script_editor_base.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/texture_rect.h"

namespace {

void _find_scripts(Node *p_base, Node *p_current, HashSet<Ref<Script>> &r_used_scripts) {
	if (p_current != p_base && p_current->get_owner() != p_base) {
		return;
	}

	if (p_current->get_script_instance()) {
		Ref<Script> scr = p_current->get_script();
		if (scr.is_valid()) {
			r_used_scripts.insert(scr);
		}
	}

	for (Node *child : p_current->iterate_children()) {
		_find_scripts(p_base, child, r_used_scripts);
	}
}

} // namespace

DocumentList::DocumentList(DocumentEditorContainer *p_document_editor_container) {
	_document_editor_container = p_document_editor_container;
	set_v_size_flags(SIZE_EXPAND_FILL);

	_filter = memnew(LineEdit);
	_filter->connect(SceneStringName(text_changed), callable_mp(this, &DocumentList::update_list).unbind(1));
	_filter->set_clear_button_enabled(true);
	_filter->set_placeholder(TTRC("Filter Documents"));
	add_child(_filter);

	_item_list = memnew(ItemList);
	_item_list->connect("item_clicked", callable_mp(this, &DocumentList::_document_clicked));
	_item_list->connect(SceneStringName(item_selected), callable_mp(this, &DocumentList::_document_selected));
	_item_list->set_allow_rmb_select(true);
	_item_list->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	_item_list->set_custom_minimum_size(Size2(60, 40) * EDSCALE);
	_item_list->set_theme_type_variation("ItemListSecondary");
	_item_list->set_v_size_flags(SIZE_EXPAND_FILL);
	add_child(_item_list);
	SET_DRAG_FORWARDING_GCDU(_item_list, DocumentList);

	_context_menu = memnew(PopupMenu);
	add_child(_context_menu);
	_context_menu->connect(SceneStringName(id_pressed), callable_mp(_document_editor_container, &DocumentEditorContainer::_menu_option));
}

void DocumentList::update_list() {
	TabContainer *tab_container = _document_editor_container->_tab_container;
	const int num_open_editors = tab_container->get_tab_count();
	const Ref<Texture2D> &help_icon = get_editor_theme_icon(SNAME("gui_help"));

	HashSet<Ref<Script>> scripts_in_scene;
	if (_document_editor_container->_highlight_scene_scripts && num_open_editors > 0) {
		Node *edited = EditorNode::get_singleton()->get_edited_scene();
		if (edited) {
			_find_scripts(edited, edited, scripts_in_scene);
		}
	}

	_items.clear();
	_items.reserve(num_open_editors);

	for (int i = 0; i < num_open_editors; i++) {
		ItemData item;
		item.index = i;
		item.display_name = tab_container->get_tab_title(i);

		Control *editor = tab_container->get_tab_control(i);
		ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(editor);
		EditorHelp *eh = Object::cast_to<EditorHelp>(editor);
		if (seb) {
			const String &path = seb->get_edited_resource()->get_path();

			item.tooltip = path.is_empty() ? TTR("Unsaved file.") : path;
			item.icon = seb->get_theme_icon();

			TextEditorBase *teb = Object::cast_to<TextEditorBase>(seb);
			if (teb) {
				item.has_error = !teb->get_validation_success();
			}

			const Ref<Script> &scr = seb->get_edited_resource();
			if (scr.is_valid()) {
				item.used_in_scene = scripts_in_scene.has(scr);
				item.tool = scr->is_tool();
			}
		}
		if (eh) {
			const String name = eh->get_class().is_empty() ? TTR("Unknown Help Class") : eh->get_class().unquote();
			const String tooltip = vformat(TTR("%s Class Reference"), name);

			item.tooltip = tooltip;
			item.icon = help_icon;
		}

		_items.push_back(item);
	}

	const String &filter_text = _filter->get_text();
	if (!filter_text.is_empty()) {
		PackedStringArray search_names;
		for (ItemData &item : _items) {
			item.hidden = true;
			search_names.append(item.display_name);
		}

		FuzzySearch fuzzy;
		fuzzy.set_case_sensitive(false);
		Vector<Ref<FuzzySearchMatch>> results = fuzzy.search_all(filter_text, search_names);

		for (const Ref<FuzzySearchMatch> &res : results) {
			_items[res->get_original_index()].hidden = false;
		}
	}

	_update_item_list();
}

void DocumentList::ensure_current_is_visible() {
	_item_list->ensure_current_is_visible();
}

void DocumentList::goto_next_document(bool p_previous) {
	if (_item_list->get_item_count() == 0) {
		return;
	}
	int next_tab = _item_list->get_current() + (p_previous ? -1 : 1);
	next_tab = Math::wrapi(next_tab, 0, _item_list->get_item_count());
	_document_editor_container->_go_to_tab(_item_list->get_item_metadata(next_tab), true);
}

void DocumentList::update_editor_settings() {
	_script_temperature_enabled = EDITOR_GET("text_editor/script_list/script_temperature_enabled");
	_temperature_history_size = EDITOR_GET("text_editor/script_list/script_temperature_history_size");

	update_list();
}

void DocumentList::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			_filter->set_right_icon(get_editor_theme_icon(SNAME("action_search")));
			_item_list->set_fixed_icon_size(Vector2i(1, 1) * get_theme_constant("class_icon_size", EditorStringName(Editor)));

			const Ref<Texture2D> &help_icon = get_editor_theme_icon(SNAME("gui_help"));
			for (ItemData &item : _items) {
				Control *editor = _document_editor_container->_tab_container->get_tab_control(item.index);
				ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(editor);
				EditorHelp *eh = Object::cast_to<EditorHelp>(editor);
				if (seb) {
					item.icon = seb->get_theme_icon();
				} else if (eh) {
					item.icon = help_icon;
				}
			}
			_update_item_list();
		} break;
	}
}

void DocumentList::_document_selected(int p_index) {
	_document_editor_container->_go_to_tab(_item_list->get_item_metadata(p_index), true);
}

void DocumentList::_document_clicked(int p_index, Vector2 p_local_mouse_pos, MouseButton p_mouse_button_index) {
	if (p_mouse_button_index == MouseButton::MIDDLE) {
		_document_selected(p_index);
		_document_editor_container->_menu_option(DocumentEditorContainer::FILE_MENU_CLOSE);
	}

	if (p_mouse_button_index == MouseButton::RIGHT) {
		_show_context_menu();
	}
}

void DocumentList::_show_context_menu() {
	_context_menu->clear();

	int selected = _document_editor_container->_tab_container->get_current_tab();
	if (selected < 0) {
		return;
	}

	ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(_document_editor_container->_tab_container->get_tab_control(selected));
	const Ref<Resource> res = seb ? seb->get_edited_resource() : Ref<Resource>();
	const Ref<Script> script = res;
	{
		if (seb) {
			_context_menu->add_shortcut(ED_GET_SHORTCUT("script_editor/save"), DocumentEditorContainer::FILE_MENU_SAVE);
			_context_menu->add_separator();
		}
		_context_menu->add_shortcut(ED_GET_SHORTCUT("script_editor/close_file"), DocumentEditorContainer::FILE_MENU_CLOSE);
		_context_menu->add_shortcut(ED_GET_SHORTCUT("script_editor/close_other_tabs"), DocumentEditorContainer::FILE_MENU_CLOSE_OTHER_TABS);
		_context_menu->add_shortcut(ED_GET_SHORTCUT("script_editor/close_tabs_below"), DocumentEditorContainer::FILE_MENU_CLOSE_TABS_BELOW);
		_context_menu->add_shortcut(ED_GET_SHORTCUT("script_editor/close_all"), DocumentEditorContainer::FILE_MENU_CLOSE_ALL);
		if (_document_editor_container->_is_main_editor) {
			_context_menu->add_shortcut(ED_GET_SHORTCUT("script_editor/close_docs"), DocumentEditorContainer::FILE_MENU_CLOSE_DOCS);
		}
		_context_menu->add_separator();

		if (_document_editor_container->_is_main_editor && script.is_valid() && script->is_tool()) {
			_context_menu->add_shortcut(ED_GET_SHORTCUT("script_editor/reload_script_soft"), DocumentEditorContainer::FILE_MENU_SOFT_RELOAD_TOOL);
			_context_menu->add_shortcut(ED_GET_SHORTCUT("script_editor/run_file"), DocumentEditorContainer::FILE_MENU_RUN);
			_context_menu->add_separator();
		}

		if (seb) {
			_context_menu->add_shortcut(ED_GET_SHORTCUT("script_editor/copy_path"), DocumentEditorContainer::FILE_MENU_COPY_PATH);
			_context_menu->add_shortcut(ED_GET_SHORTCUT("script_editor/copy_uid"), DocumentEditorContainer::FILE_MENU_COPY_UID);
			_context_menu->add_shortcut(ED_GET_SHORTCUT("script_editor/show_in_file_system"), DocumentEditorContainer::FILE_MENU_SHOW_IN_FILE_SYSTEM);
			if (!_document_editor_container->_is_main_editor) {
				_context_menu->add_shortcut(ED_SHORTCUT("script_editor/open_in_inspector", TTRC("Open File in Inspector")), DocumentEditorContainer::FILE_MENU_INSPECT);
				_context_menu->add_shortcut(ED_SHORTCUT("script_editor/inspect_native_code", TTRC("Inspect Native Shader Code...")), DocumentEditorContainer::FILE_MENU_INSPECT_NATIVE_SHADER_CODE);
			}
			_context_menu->add_separator();
		}

		_context_menu->add_shortcut(ED_GET_SHORTCUT("script_editor/move_document_up"), DocumentEditorContainer::FILE_MENU_MOVE_UP);
		_context_menu->add_shortcut(ED_GET_SHORTCUT("script_editor/move_document_down"), DocumentEditorContainer::FILE_MENU_MOVE_DOWN);
		_context_menu->add_shortcut(ED_GET_SHORTCUT("script_editor/sort_documents"), DocumentEditorContainer::FILE_MENU_SORT);
	}

	{
		_context_menu->set_item_disabled(_context_menu->get_item_index(DocumentEditorContainer::FILE_MENU_SAVE), !seb);
		_context_menu->set_item_disabled(_context_menu->get_item_index(DocumentEditorContainer::FILE_MENU_CLOSE), _document_editor_container->_tab_container->get_tab_count() < 1);
		_context_menu->set_item_disabled(_context_menu->get_item_index(DocumentEditorContainer::FILE_MENU_CLOSE_OTHER_TABS), _document_editor_container->_tab_container->get_tab_count() <= 1);
		_context_menu->set_item_disabled(_context_menu->get_item_index(DocumentEditorContainer::FILE_MENU_CLOSE_TABS_BELOW), _document_editor_container->_tab_container->get_current_tab() >= _document_editor_container->_tab_container->get_tab_count() - 1);
		_context_menu->set_item_disabled(_context_menu->get_item_index(DocumentEditorContainer::FILE_MENU_CLOSE_ALL), _document_editor_container->_tab_container->get_tab_count() < 1);
		if (_document_editor_container->_is_main_editor) {
			_context_menu->set_item_disabled(_context_menu->get_item_index(DocumentEditorContainer::FILE_MENU_CLOSE_DOCS), !_document_editor_container->_has_docs_tab());
			_context_menu->set_item_disabled(_context_menu->get_item_index(DocumentEditorContainer::FILE_MENU_SOFT_RELOAD_TOOL), script.is_null() || !script->is_tool());
			_context_menu->set_item_disabled(_context_menu->get_item_index(DocumentEditorContainer::FILE_MENU_RUN), script.is_null());
		}
		_context_menu->set_item_disabled(_context_menu->get_item_index(DocumentEditorContainer::FILE_MENU_COPY_PATH), res.is_null() || res->get_path().is_empty());
		_context_menu->set_item_disabled(_context_menu->get_item_index(DocumentEditorContainer::FILE_MENU_COPY_UID), res.is_null() || ResourceLoader::get_resource_uid(res->get_path()) == ResourceUID::INVALID_ID);
		_context_menu->set_item_disabled(_context_menu->get_item_index(DocumentEditorContainer::FILE_MENU_SHOW_IN_FILE_SYSTEM), res.is_null() || res->get_path().is_empty());
		if (!_document_editor_container->_is_main_editor) {
			_context_menu->set_item_disabled(_context_menu->get_item_index(DocumentEditorContainer::FILE_MENU_INSPECT), !seb);
			_context_menu->set_item_disabled(_context_menu->get_item_index(DocumentEditorContainer::FILE_MENU_INSPECT_NATIVE_SHADER_CODE), !seb);
		}
		_context_menu->set_item_disabled(_context_menu->get_item_index(DocumentEditorContainer::FILE_MENU_MOVE_UP), _document_editor_container->_tab_container->get_current_tab() <= 0);
		_context_menu->set_item_disabled(_context_menu->get_item_index(DocumentEditorContainer::FILE_MENU_MOVE_DOWN), _document_editor_container->_tab_container->get_current_tab() >= _document_editor_container->_tab_container->get_tab_count() - 1);
		_context_menu->set_item_disabled(_context_menu->get_item_index(DocumentEditorContainer::FILE_MENU_SORT), _document_editor_container->_tab_container->get_tab_count() <= 1);
	}

	// Context menu plugin.
	if (EditorContextMenuPluginManager::get_singleton()->has_plugins_for_slot(EditorContextMenuPlugin::CONTEXT_SLOT_SCRIPT_EDITOR)) {
		EditorContextMenuPlugin::OptionsData context_data = DocumentEditorContainer::get_context_data(_document_editor_container->_tab_container->get_tab_control(selected));
		EditorContextMenuPluginManager::get_singleton()->add_options_from_plugins(_context_menu, EditorContextMenuPlugin::CONTEXT_SLOT_SCRIPT_EDITOR, context_data);

	}

	_context_menu->set_position(get_screen_position() + get_local_mouse_position());
	_context_menu->reset_size();
	_context_menu->popup();
}

void DocumentList::_update_item_list() {
	_item_list->clear();
	if (_items.is_empty()) {
		return;
	}

	TabContainer *tab_container = _document_editor_container->_tab_container;
	const int current_index = tab_container->get_current_tab();

	Color tool_color = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
	tool_color.set_s(tool_color.get_s() * 1.5);
	Color used_item_bg_color = Color(0.5, 0.5, 0.5, 0.125);
	const Ref<Texture2D> &error_icon = get_editor_theme_icon(SNAME("status_error"));

	for (const ItemData &item : _items) {
		if (item.hidden) {
			continue;
		}
		_item_list->add_item(item.display_name, item.icon);
		if (item.tool) {
			_item_list->set_item_icon_modulate(-1, tool_color);
		}

		_item_list->set_item_tooltip(-1, item.tooltip);
		_item_list->set_item_metadata(-1, item.index); // Save the script's index in the tab container and not the filtered one.
		if (item.used_in_scene) {
			_item_list->set_item_custom_bg_color(-1, used_item_bg_color);
		}
		if (item.has_error) {
			_item_list->set_item_tag_icon(-1, error_icon);
		}
		if (item.index == current_index) {
			_item_list->select(_item_list->get_item_count() - 1);
		}
	}
	_item_list->ensure_current_is_visible();

	// Update colors.
	if (!_script_temperature_enabled) {
		return;
	}

	Color hot_color = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
	hot_color.set_s(hot_color.get_s() * 0.9);
	Color cold_color = get_theme_color(SceneStringName(font_color), EditorStringName(Editor));

	int edit_pass = _document_editor_container->_edit_pass;

	for (int i = 0; i < _item_list->get_item_count(); i++) {
		int index = _item_list->get_item_metadata(i);
		Node *open_editor = tab_container->get_tab_control(index);
		if (!open_editor) {
			continue;
		}

		int pass = open_editor->get_meta("__editor_pass", -1);
		if (pass < 0) {
			continue;
		}

		int h = edit_pass - pass;
		if (h > _temperature_history_size) {
			continue;
		}
		int non_zero_hist_size = (_temperature_history_size == 0) ? 1 : _temperature_history_size;
		float v = Math::ease(h / float(non_zero_hist_size), 0.4);

		_item_list->set_item_custom_fg_color(i, hot_color.lerp(cold_color, v));
	}
}

Variant DocumentList::_get_drag_data_fw(const Point2 &p_point, Control *p_from) {
	Node *cur_node = _document_editor_container->_tab_container->get_current_tab_control();
	if (cur_node == nullptr) {
		return Variant();
	}

	String preview_name;
	Ref<Texture2D> preview_icon;

	ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(cur_node);
	if (seb) {
		preview_name = seb->get_document_name();
		preview_icon = seb->get_theme_icon();
	}
	EditorHelp *eh = Object::cast_to<EditorHelp>(cur_node);
	if (eh) {
		preview_name = eh->get_class();
		preview_icon = get_editor_theme_icon(SNAME("gui_help"));
	}

	HBoxContainer *drag_preview_hbox = memnew(HBoxContainer);
	{
		if (preview_icon.is_valid()) {
			TextureRect *preview_texture = memnew(TextureRect);
			preview_texture->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
			preview_texture->set_texture(preview_icon);
			drag_preview_hbox->add_child(preview_texture);
		}
		Label *preview_lbl = memnew(Label(preview_name));
		preview_lbl->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED); // Don't translate script names and class names.
		drag_preview_hbox->add_child(preview_lbl);
	}
	set_drag_preview(drag_preview_hbox);

	Dictionary drag_data;
	drag_data["type"] = "script_list_element";
	drag_data["script_list_element"] = cur_node;
	drag_data["editor"] = _document_editor_container->_config_section;

	return drag_data;
}

bool DocumentList::_can_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from) const {
	Dictionary d = p_data;
	const String type = d.get("type", "");

	if (type == "script_list_element") {
		if (d["editor"] != _document_editor_container->_config_section) {
			return false;
		}
		Node *node = Object::cast_to<Node>(d["script_list_element"]);

		if (Object::cast_to<ScriptEditorBase>(node) || Object::cast_to<EditorHelp>(node)) {
			return true;
		}
	}

	if (type == "files") {
		Vector<String> files = d["files"];

		if (files.is_empty()) {
			return false; // Weird.
		}

		for (const String &file : files) {
			if (_document_editor_container->can_open_file(file)) {
				return true;
			}
		}
		return false;
	}

	return false;
}

void DocumentList::_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from) {
	if (!_can_drop_data_fw(p_point, p_data, p_from)) {
		return;
	}

	Dictionary d = p_data;
	const String type = d.get("type", "");

	if (type == "script_list_element") {
		Node *node = Object::cast_to<Node>(d["script_list_element"]);

		ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(node);
		EditorHelp *eh = Object::cast_to<EditorHelp>(node);
		if (seb || eh) {
			int new_index = 0;
			if (_item_list->get_item_count() > 0) {
				int pos = 0;
				if (p_point == Vector2(Math::INF, Math::INF)) {
					if (_item_list->is_anything_selected()) {
						pos = _item_list->get_selected_items()[0];
					}
				} else {
					pos = _item_list->get_item_at_position(p_point);
				}
				new_index = _item_list->get_item_metadata(pos);
			}
			TabContainer *tab_container = _document_editor_container->_tab_container;
			tab_container->move_child(node, new_index);
			tab_container->set_current_tab(new_index);
			update_list();
		}
	}

	if (type == "files") {
		Vector<String> files = d["files"];

		int new_index = 0;
		if (_item_list->get_item_count() > 0) {
			int pos = 0;
			if (p_point == Vector2(Math::INF, Math::INF)) {
				if (_item_list->is_anything_selected()) {
					pos = _item_list->get_selected_items()[0];
				}
			} else {
				pos = _item_list->get_item_at_position(p_point);
			}
			new_index = _item_list->get_item_metadata(pos);
		}
		TabContainer *tab_container = _document_editor_container->_tab_container;
		int num_tabs_before = tab_container->get_tab_count();
		for (const String &file : files) {
			if (!_document_editor_container->can_open_file(file)) {
				continue;
			}

			Ref<Resource> res = _document_editor_container->open_file(file);
			if (res.is_valid()) {
				const int num_tabs = tab_container->get_tab_count();
				if (num_tabs > num_tabs_before) {
					tab_container->move_child(tab_container->get_tab_control(tab_container->get_tab_count() - 1), new_index);
					num_tabs_before = num_tabs;
				} else if (num_tabs > 0) { // Maybe script was already open.
					tab_container->move_child(tab_container->get_current_tab_control(), new_index);
				}
			}
		}
		if (tab_container->get_tab_count() > 0) {
			tab_container->set_current_tab(new_index);
		}
		update_list();
	}
}
