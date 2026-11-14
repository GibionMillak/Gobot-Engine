/**************************************************************************/
/*  editor_quick_open_dialog.h                                            */
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

#include "core/io/config_file.h"
#include "core/os/keyboard.h"
#include "core/string/string_name.h"
#include "core/templates/a_hash_map.h"
#include "core/templates/hash_set.h"
#include "core/templates/local_vector.h"
#include "core/templates/vector.h"
#include "core/variant/callable.h"
#include "core/variant/variant.h"
#include "editor/gui/data/quick_open_result_candidate.h"
#include "scene/gui/dialog.h"

class Button;
class EditorFileSystemDirectory;
class HFlowContainer;
class HSlider;
class InputEvent;
class Label;
class LineEdit;
class MenuButton;
class PanelContainer;
class PopupMenu;
class QuickOpenResultItemContainer;
class ScrollContainer;
class VBoxContainer;

class EditorQuickOpenDialog : public Dialog {
	GDCLASS(EditorQuickOpenDialog, Dialog);

public:
	enum class DisplayMode {
		GRID,
		LIST,
	};

	EditorQuickOpenDialog();

	void popup_dialog(const Vector<StringName> &p_base_types, const Callable &p_item_selected_callback, bool p_allow_type_switching = false);
	void popup_dialog_for_property(const Vector<StringName> &p_base_types, Object *p_obj, const StringName &p_path, const Callable &p_item_selected_callback);

protected:
	void _notification(int p_what);
	void _on_canceled() override;
	void _on_confirmed() override;
	void shortcut_input(const Ref<InputEvent> &p_event) override;

private:
	enum FileMenuOption {
		SHOW_IN_FILESYSTEM,
		SHOW_IN_FILE_MANAGER,
	};

	enum SourceFilter {
		ADDONS,
		EDITOR_CONTENT,
	};

	static constexpr int MAX_HISTORY_SIZE = 20;

	static String _get_dialog_title(const Vector<StringName> &p_base_types);
	static DisplayMode _get_adaptive_display_mode(const Vector<StringName> &p_base_types);

	void _prepare_search(const Vector<StringName> &p_base_types);
	void _finish_dialog_setup(const Vector<StringName> &p_base_types);
	void _search_box_text_changed(const String &p_query);
	void _handle_search_box_input(const Ref<InputEvent> &p_ie);
	void _item_pressed(bool p_double_click);
	void _selection_changed();
	bool _is_instant_preview_active() const;
	void _preview_property();
	void _update_property();

	bool _has_nothing_selected() const;
	ResourceUID::ID _get_selected() const;
	String _get_selected_path() const;
	void _save_selected_item();
	void _cleanup_results();

	void _update_results();
	void _ensure_result_vector_capacity();
	QuickOpenResultItemContainer *_create_result_item(int p_index);
	void _sort_uids(int p_max_results);
	void _create_initial_results();
	void _find_uids_in_folder(EditorFileSystemDirectory *p_directory, bool p_include_addons);
	void _find_editor_content_resources();
	Vector<ResourceUID::ID> *_get_history();
	void _add_candidate(QuickOpenResultCandidate &r_candidate);
	void _editor_content_preview_ready(const String &p_path, const Ref<Texture2D> &p_preview, const Ref<Texture2D> &p_small_preview);
	Vector<Ref<FuzzySearchMatch>> _get_fuzzy_search_results();
	void _use_default_candidates();
	void _score_and_sort_candidates();

	void _update_result_items(int p_new_visible_results_count, int p_new_selection_index);
	void _update_result_count();
	void _move_selection_index(Key p_key);
	void _select_item(int p_index);
	void _item_input(const Ref<InputEvent> &p_ev, int p_index);
	CanvasItem *_get_result_root();

	void _set_display_mode(DisplayMode p_display_mode);
	void _toggle_display_mode();
	void _toggle_instant_preview(bool p_pressed);
	void _toggle_fuzzy_search(bool p_pressed);
	void _filter_option_selected(int p_option);
	void _item_size_changed(double p_value);
	void _menu_option(int p_option);
	String _get_cache_file_path() const;

	Callable _item_selected_callback;
	Object *_property_object = nullptr;
	StringName _property_path;
	Variant _initial_property_value;
	bool _initial_selection_performed = false;
	bool _allow_type_switching = false;
	bool _is_cycling_items = false;

	Vector<StringName> _base_types;
	LocalVector<ResourceUID::ID> _uids;
	AHashMap<ResourceUID::ID, StringName> _filetypes;
	Vector<QuickOpenResultCandidate> _candidates;
	HashSet<ResourceUID::ID> _candidates_uids;
	AHashMap<StringName, Vector<ResourceUID::ID>> _selected_history;
	HashSet<ResourceUID::ID> _history_set;
	String _query;
	int _selection_index = -1;
	int _num_visible_results = 0;
	int _max_total_results = 0;
	bool _never_opened = true;
	Ref<ConfigFile> _history_file;
	DisplayMode _content_display_mode = DisplayMode::LIST;
	Vector<QuickOpenResultItemContainer *> _result_items;
	ScrollContainer *_scroll_container = nullptr;
	VBoxContainer *_list_vbox = nullptr;
	HFlowContainer *_grid_hflow = nullptr;
	PopupMenu *_file_context_menu = nullptr;
	PanelContainer *_no_results_pc = nullptr;
	Label *_no_results_lbl = nullptr;
	Label *_result_count_lbl = nullptr;
	AHashMap<StringName, Ref<Texture2D>> _file_type_icons;
	LineEdit *_search_box = nullptr;
	Button *_display_mode_btn = nullptr;
	Button *_instant_preview_btn = nullptr;
	Button *_fuzzy_search_btn = nullptr;
	MenuButton *_filter_menu_btn = nullptr;
	HSlider *_item_size_slider = nullptr;
};
