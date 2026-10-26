/**************************************************************************/
/*  document_outline.cpp                                                  */
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

#include "document_outline.h"

#include "core/object/callable_mp.h"
#include "core/string/fuzzy_search.h"
#include "editor/doc/editor_help.h"
#include "editor/docks/script_editor/components/document_editor_container.h"
#include "editor/editor_string_names.h"
#include "editor/gui/filter_line_edit.h"
#include "editor/script/script_editor_base.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/tree.h"

DocumentOutline::DocumentOutline(DocumentEditorContainer *p_document_editor_container) {
	_document_editor_container = p_document_editor_container;
	_buttons_hbox = memnew(HBoxContainer);
	add_child(_buttons_hbox);

	{
		_filter = memnew(FilterLineEdit);
		_filter->set_clear_button_enabled(true);
		_filter->set_h_size_flags(SIZE_EXPAND_FILL);
		_filter->set_placeholder(TTRC("Filter Methods"));
		_buttons_hbox->add_child(_filter);
		_filter->connect(SceneStringName(text_changed), callable_mp(this, &DocumentOutline::update_outline).unbind(1));

		_sort_btn = memnew(Button);
		_sort_btn->set_theme_type_variation(SceneStringName(FlatButton));
		_sort_btn->set_toggle_mode(true);
		_sort_btn->set_tooltip_text(TTRC("Toggle alphabetical sorting of the method list."));
		_buttons_hbox->add_child(_sort_btn);
		_sort_btn->set_pressed_no_signal(EDITOR_GET("text_editor/script_list/sort_members_outline_alphabetically"));
		_sort_btn->connect(SceneStringName(toggled), callable_mp(this, &DocumentOutline::_toggle_sort));
	}

	_tree = memnew(Tree);
	_tree->add_theme_constant_override("h_separation", 0);
	_tree->set_allow_reselect(true);
	_tree->set_allow_rmb_select(true);
	_tree->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	_tree->set_custom_minimum_size(Size2(0, 40 * EDSCALE));
	_tree->set_hide_folding(true);
	_tree->set_hide_root(true);
	_tree->set_theme_type_variation("TreeSecondary");
	_tree->set_v_size_flags(SIZE_EXPAND_FILL);
	add_child(_tree);
	_tree->connect(SceneStringName(item_selected), callable_mp(this, &DocumentOutline::_tree_selected));
	_filter->set_forward_control(_tree);
}

void DocumentOutline::update_editor_settings() {
	_members_overview_enabled = EDITOR_GET("text_editor/script_list/show_members_overview");
	_help_overview_enabled = EDITOR_GET("text_editor/help/show_help_index");
	update_visibility();
}

void DocumentOutline::update_outline() {
	String selected;
	if (_tree->is_anything_selected()) {
		selected = _tree->get_selected()->get_meta("function");
	}

	Control *active_editor = _document_editor_container->get_active_editor();
	if (_current_editor != active_editor) {
		selected = ""; // Script/doc changed, forget the last selection.
	}
	_current_editor = active_editor;

	_updating_outline = true;

	_tree->clear();
	_tree->create_item();

	if (CodeEditorBase *ceb = Object::cast_to<CodeEditorBase>(active_editor)) {
		PackedStringArray functions = ceb->get_functions();
		if (EDITOR_GET("text_editor/script_list/sort_members_outline_alphabetically")) {
			functions.sort();
		}

		const String &filter_text = _filter->get_text();
		HashMap<String, TreeItem *> parents;
		const Color disabled_color = get_theme_color(SNAME("font_disabled_color"), EditorStringName(Editor));

		if (filter_text.is_empty()) {
			for (const String &function_name : functions) {
				_add_function(function_name, selected, disabled_color, parents);
			}
		} else {
			PackedStringArray search_names;
			for (const String &function_name : functions) {
				search_names.append(function_name.get_slicec(':', 0));
			}

			FuzzySearch fuzzy;
			fuzzy.set_case_sensitive(false);
			Vector<Ref<FuzzySearchMatch>> results = fuzzy.search_all(filter_text, search_names);

			for (const Ref<FuzzySearchMatch> &res : results) {
				_add_function(functions[res->get_original_index()], selected, disabled_color, parents);
			}
		}
	} else if (EditorHelp *eh = Object::cast_to<EditorHelp>(active_editor)) {
		const Vector<Pair<String, int>> sections = eh->get_sections();
		for (const Pair<String, int> &section : sections) {
			TreeItem *ti = _tree->create_item();
			ti->set_text(0, section.first);
			ti->set_metadata(0, section.second);
			ti->set_meta("function", section.first);
			if (selected == section.first) {
				ti->select(0);
			}
		}
	}
	_updating_outline = false;
}

void DocumentOutline::update_visibility() {
	Control *active_editor = _document_editor_container->get_active_editor();
	ScriptEditorBase *seb = Object::cast_to<ScriptEditorBase>(active_editor);
	EditorHelp *eh = Object::cast_to<EditorHelp>(active_editor);

	bool members_overview_visible = _members_overview_enabled && seb && seb->show_members_overview();
	bool help_overview_visible = _help_overview_enabled && eh;

	_buttons_hbox->set_visible(members_overview_visible);
	set_visible(members_overview_visible || help_overview_visible);

	bool use_monospace_font = members_overview_visible && EDITOR_GET("interface/theme/use_monospace_font_for_editor_symbols");
	if (use_monospace_font) {
		Ref<Font> monospace_font = get_theme_font(SNAME("source"), EditorStringName(EditorFonts));
		if (_tree->get_theme_font(SceneStringName(font)) != monospace_font) {
			_tree->add_theme_font_override(SceneStringName(font), monospace_font);
		}
	} else if (_tree->has_theme_font_override(SceneStringName(font))) {
		_tree->remove_theme_font_override(SceneStringName(font));
	}
}

void DocumentOutline::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			_sort_btn->set_button_icon(get_editor_theme_icon(SNAME("action_sort")));

			update_visibility();
		} break;
	}
}

void DocumentOutline::_add_function(const String &p_function_name, const String &p_selected, const Color &p_disabled_color, HashMap<String, TreeItem *> &r_parents) {
	TreeItem *parent = _tree->get_root();
	String name = p_function_name.get_slicec(':', 0);
	const Vector<String> subclasses = name.split(".");
	if (!subclasses.is_empty()) {
		const int last = subclasses.size() - 1;
		name = subclasses[last];
		String subclass_path;
		for (int i = 0; i < last; i++) {
			const String &subclass = subclasses[i];
			if (!subclass_path.is_empty()) {
				subclass_path += ".";
			}
			subclass_path += subclass;
			if (r_parents.has(subclass_path)) {
				parent = r_parents[subclass_path];
			} else {
				parent = _tree->create_item(parent);
				parent->set_text(0, subclass);
				parent->set_selectable(0, false);
				parent->set_custom_color(0, p_disabled_color);
				r_parents[subclass_path] = parent;
			}
		}
	}

	TreeItem *function_item = _tree->create_item(parent);
	function_item->set_text(0, name);
	function_item->set_metadata(0, p_function_name.get_slicec(':', 1).to_int() - 1);
	function_item->set_meta("function", p_function_name);
	if (p_selected == p_function_name) {
		function_item->select(0);
	}
}

void DocumentOutline::_toggle_sort(bool p_alphabetic_sort) {
	EditorSettings::get_singleton()->set("text_editor/script_list/sort_members_outline_alphabetically", p_alphabetic_sort);
	update_outline();
}

void DocumentOutline::_tree_selected() {
	if (_updating_outline) {
		return;
	}
	Control *active_editor = _document_editor_container->get_active_editor();
	int line = _tree->get_selected()->get_metadata(0);
	if (TextEditorBase *teb = Object::cast_to<TextEditorBase>(active_editor)) {
		teb->goto_line_centered(line);
	} else if (EditorHelp *eh = Object::cast_to<EditorHelp>(active_editor)) {
		eh->scroll_to_section(line);
	}
}
