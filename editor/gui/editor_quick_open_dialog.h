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

#include "core/string/fuzzy_search.h"
#include "core/templates/a_hash_map.h"
#include "scene/gui/box_container.h"
#include "scene/gui/dialog.h"
#include "scene/gui/margin_container.h"

class Button;
class CheckButton;
class ConfigFile;
class EditorFileSystemDirectory;
class LineEdit;
class HFlowContainer;
class MarginContainer;
class PanelContainer;
class PopupMenu;
class ScrollContainer;
class StringName;
class Texture2D;
class TextureRect;
class VBoxContainer;

class QuickOpenResultItem;

enum class QuickOpenDisplayMode {
	GRID,
	LIST,
};

struct QuickOpenResultCandidate {
	ResourceUID::ID uid;
	Ref<Texture2D> thumbnail;
	Ref<FuzzySearchMatch> result;

	static QuickOpenResultCandidate from_uid(const ResourceUID::ID &p_uid, bool &r_success);
	static QuickOpenResultCandidate from_result(const Ref<FuzzySearchMatch> p_result, bool &r_success);
};

class HighlightedLabel : public Label {
	GDCLASS(HighlightedLabel, Label)

	Vector<Vector2i> highlights;

	void draw_substr_rects(const Vector2i &p_substr, Vector2 p_offset, int p_line_limit, int line_spacing);

public:
	void add_highlight(const Vector2i &p_interval);
	void reset_highlights();

protected:
	void _notification(int p_notification);
};

class QuickOpenResultContainer : public VBoxContainer {
	GDCLASS(QuickOpenResultContainer, VBoxContainer)

	enum {
		FILE_SHOW_IN_FILESYSTEM,
		FILE_SHOW_IN_FILE_MANAGER
	};

public:
	void init(const Vector<StringName> &p_base_types);
	void handle_search_box_input(const Ref<InputEvent> &p_ie);
	void set_query_and_update(const String &p_query);
	void update_results();

	bool has_nothing_selected() const;
	ResourceUID::ID get_selected() const;
	String get_selected_path() const;
	const Vector<StringName> &get_base_types() const;

	bool is_instant_preview_enabled() const;
	void set_instant_preview_toggle_visible(bool p_visible);

	void save_selected_item();
	void cleanup();

	QuickOpenResultContainer();

protected:
	void _notification(int p_what);

private:
	static constexpr int MAX_HISTORY_SIZE = 20;

	Vector<StringName> base_types;
	LocalVector<ResourceUID::ID> uids;
	AHashMap<ResourceUID::ID, StringName> filetypes;
	Vector<QuickOpenResultCandidate> candidates;
	HashSet<ResourceUID::ID> candidates_uids;

	AHashMap<StringName, Vector<ResourceUID::ID>> selected_history;
	HashSet<ResourceUID::ID> history_set;

	String query;
	int selection_index = -1;
	int num_visible_results = 0;
	int max_total_results = 0;

	bool never_opened = true;
	Ref<ConfigFile> history_file;

	QuickOpenDisplayMode content_display_mode = QuickOpenDisplayMode::LIST;
	Vector<QuickOpenResultItem *> result_items;

	ScrollContainer *scroll_container = nullptr;
	VBoxContainer *list = nullptr;
	HFlowContainer *grid = nullptr;
	PopupMenu *file_context_menu = nullptr;

	PanelContainer *no_results_container = nullptr;
	Label *no_results_label = nullptr;

	Label *file_details_path = nullptr;
	Label *_result_count_lbl = nullptr;
	Button *display_mode_toggle = nullptr;
	CheckButton *instant_preview_toggle = nullptr;
	CheckButton *include_addons_toggle = nullptr;
	CheckButton *_include_editor_content_toggle = nullptr;
	CheckButton *fuzzy_search_toggle = nullptr;

	AHashMap<StringName, Ref<Texture2D>> file_type_icons;

	static QuickOpenDisplayMode get_adaptive_display_mode(const Vector<StringName> &p_base_types);

	void _ensure_result_vector_capacity();
	void _sort_uids(int p_max_results);
	void _create_initial_results();
	void _find_uids_in_folder(EditorFileSystemDirectory *p_directory, bool p_include_addons);
	void _find_editor_content_resources();

	Vector<ResourceUID::ID> *_get_history();
	void _add_candidate(QuickOpenResultCandidate &p_candidate);
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
	void _layout_result_item(QuickOpenResultItem *p_item);
	void _set_display_mode(QuickOpenDisplayMode p_display_mode);
	void _toggle_display_mode();
	void _toggle_instant_preview(bool p_pressed);
	void _toggle_include_addons(bool p_pressed);
	void _toggle_include_editor_content(bool p_pressed);
	void _toggle_fuzzy_search(bool p_pressed);
	void _menu_option(int p_option);

	String _get_cache_file_path() const;

	static void _bind_methods();
};

class QuickOpenResultGridItem : public MarginContainer {
	GDCLASS(QuickOpenResultGridItem, MarginContainer)

public:
	QuickOpenResultGridItem();

	void reset();
	void set_content(const QuickOpenResultCandidate &p_candidate, bool p_highlight);
	void highlight_item(const Color &p_color);
	void remove_highlight();

private:
	VBoxContainer *vbc = nullptr;
	TextureRect *thumbnail = nullptr;
	HighlightedLabel *name = nullptr;
};

class QuickOpenResultListItem : public MarginContainer {
	GDCLASS(QuickOpenResultListItem, MarginContainer)

public:
	QuickOpenResultListItem();

	void reset();
	void set_content(const QuickOpenResultCandidate &p_candidate, bool p_highlight);
	void highlight_item(const Color &p_color);
	void remove_highlight();

protected:
	void _notification(int p_what);

private:
	TextureRect *thumbnail = nullptr;
	HighlightedLabel *name = nullptr;
	HighlightedLabel *path = nullptr;
};

class QuickOpenResultItem : public MarginContainer {
	GDCLASS(QuickOpenResultItem, MarginContainer)

public:
	QuickOpenResultItem();

	void reset();
	void set_content(const QuickOpenResultCandidate &p_candidate);
	void set_display_mode(QuickOpenDisplayMode p_display_mode);
	void highlight_item(bool p_enabled);

	bool enable_highlights = true;

protected:
	void _notification(int p_what);

private:
	enum class ContentSource {
		PROJECT,
		ADDON,
		EDITOR,
	};

	void _set_enabled(bool p_enabled);
	void _update_source_badge();

	QuickOpenResultListItem *list_item = nullptr;
	QuickOpenResultGridItem *grid_item = nullptr;
	TextureRect *_source_badge = nullptr;
	ContentSource _content_source = ContentSource::PROJECT;

	Ref<StyleBox> selected_stylebox;
	Ref<StyleBox> hovering_stylebox;
	Color highlighted_font_color;

	bool is_hovering = false;
	bool is_selected = false;
};

class EditorQuickOpenDialog : public Dialog {
	GDCLASS(EditorQuickOpenDialog, Dialog);

public:
	void popup_dialog(const Vector<StringName> &p_base_types, const Callable &p_item_selected_callback, bool p_allow_type_switching = false);
	void popup_dialog_for_property(const Vector<StringName> &p_base_types, Object *p_obj, const StringName &p_path, const Callable &p_item_selected_callback);
	EditorQuickOpenDialog();

protected:
	virtual void _on_canceled() override;
	virtual void _on_confirmed() override;
	virtual void shortcut_input(const Ref<InputEvent> &p_event) override;
	void item_pressed(bool p_double_click);
	void selection_changed();

private:
	static String get_dialog_title(const Vector<StringName> &p_base_types);

	LineEdit *search_box = nullptr;
	QuickOpenResultContainer *container = nullptr;

	Callable item_selected_callback;

	Object *property_object = nullptr;
	StringName property_path;
	Variant initial_property_value;
	bool initial_selection_performed = false;
	bool allow_type_switching = false;
	bool is_cycling_items = false;
	bool _is_instant_preview_active() const;
	void _search_box_text_changed(const String &p_query);
	void _finish_dialog_setup(const Vector<StringName> &p_base_types);

	void preview_property();
	void update_property();
};
