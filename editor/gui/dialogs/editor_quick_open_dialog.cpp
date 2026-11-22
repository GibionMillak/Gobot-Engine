/**************************************************************************/
/*  editor_quick_open_dialog.cpp                                          */
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

#include "editor_quick_open_dialog.h"

#include "core/config/project_settings.h"
#include "core/input/input_event.h"
#include "core/io/config_file.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_uid.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/os/os.h"
#include "core/string/fuzzy_search.h"
#include "core/templates/fixed_vector.h"
#include "core/templates/hash_set.h"
#include "editor/content/editor_content_resources.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/editor_data.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/file_system/editor_paths.h"
#include "editor/gui/containers/editor_toaster_container.h"
#include "editor/gui/containers/quick_open_result_item_container.h"
#include "editor/inspector/editor_resource_preview.h"
#include "editor/inspector/multi_node_edit.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_icons.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/slider.h"
#include "scene/scene_string_names.h"

EditorQuickOpenDialog::EditorQuickOpenDialog() {
	_history_file.instantiate();

	VBoxContainer *dialog_vbox = memnew(VBoxContainer);
	add_child(dialog_vbox);

	{
		HBoxContainer *search_hbox = memnew(HBoxContainer);
		dialog_vbox->add_child(search_hbox);

		{
			_fuzzy_search_btn = memnew(Button);
			_fuzzy_search_btn->connect(SceneStringName(toggled), callable_mp(this, &EditorQuickOpenDialog::_toggle_fuzzy_search));
			_fuzzy_search_btn->set_focus_mode(Control::FOCUS_NONE);
			_fuzzy_search_btn->set_text(TTR("Fuzzy Search"));
			_fuzzy_search_btn->set_toggle_mode(true);
			_fuzzy_search_btn->set_tooltip_text(TTR("Fuzzy Search: Include approximate matches."));
			search_hbox->add_child(_fuzzy_search_btn);

			_search_box = memnew(LineEdit);
			_search_box->set_clear_button_enabled(true);
			_search_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			_search_box->set_placeholder(TTR("Search files"));
			search_hbox->add_child(_search_box);

			_filter_menu_btn = memnew(MenuButton);
			_filter_menu_btn->set_focus_mode(Control::FOCUS_NONE);
			_filter_menu_btn->set_tooltip_text(TTR("Filter content sources"));
			search_hbox->add_child(_filter_menu_btn);

			{
				PopupMenu *filter_popup = _filter_menu_btn->get_popup();
				filter_popup->add_check_item(TTR("Addons"), ADDONS);
				filter_popup->add_check_item(TTR("Editor Content"), EDITOR_CONTENT);
				filter_popup->connect(SceneStringName(id_pressed), callable_mp(this, &EditorQuickOpenDialog::_filter_option_selected));
				filter_popup->set_hide_on_checkable_item_selection(false);
				filter_popup->set_item_tooltip(filter_popup->get_item_index(ADDONS), TTR("Include files from addons."));
				filter_popup->set_item_tooltip(filter_popup->get_item_index(EDITOR_CONTENT), TTR("Include read-only assets and resources supplied with the editor."));
			}
		}

		MarginContainer *results_margin = memnew(MarginContainer);
		results_margin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		results_margin->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		dialog_vbox->add_child(results_margin);

		{
			_no_results_pc = memnew(PanelContainer);
			_no_results_pc->hide();
			results_margin->add_child(_no_results_pc);

			{
				_no_results_lbl = memnew(Label);
				_no_results_lbl->add_theme_font_size_override(SceneStringName(font_size), 24 * EDSCALE);
				_no_results_lbl->set_h_size_flags(Control::SIZE_SHRINK_CENTER);
				_no_results_lbl->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
				_no_results_pc->add_child(_no_results_lbl);
			}

			_scroll_container = memnew(ScrollContainer);
			_scroll_container->hide();
			_scroll_container->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
			_scroll_container->set_scroll_hint_mode(ScrollContainer::SCROLL_HINT_MODE_ALL);
			results_margin->add_child(_scroll_container);

			{
				_list_vbox = memnew(VBoxContainer);
				_list_vbox->hide();
				_list_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
				_scroll_container->add_child(_list_vbox);

				_grid_hflow = memnew(HFlowContainer);
				_grid_hflow->hide();
				_grid_hflow->set_h_size_flags(Control::SIZE_EXPAND_FILL);
				_grid_hflow->set_v_size_flags(Control::SIZE_EXPAND_FILL);
				_scroll_container->add_child(_grid_hflow);

				_file_context_menu = memnew(PopupMenu);
				_file_context_menu->add_item(TTR("Show in FileSystem"), SHOW_IN_FILESYSTEM);
				_file_context_menu->add_item(OS::get_singleton()->get_platform_string(OS::PLATFORM_STRING_FILE_MANAGER_SHOW), SHOW_IN_FILE_MANAGER);
				_file_context_menu->connect(SceneStringName(id_pressed), callable_mp(this, &EditorQuickOpenDialog::_menu_option));
				_file_context_menu->hide();
				_scroll_container->add_child(_file_context_menu);
			}
		}

		HBoxContainer *bottom_hbox = memnew(HBoxContainer);
		bottom_hbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		dialog_vbox->add_child(bottom_hbox);

		{
			_instant_preview_btn = memnew(Button);
			_instant_preview_btn->connect(SceneStringName(toggled), callable_mp(this, &EditorQuickOpenDialog::_toggle_instant_preview));
			_instant_preview_btn->set_focus_mode(Control::FOCUS_NONE);
			_instant_preview_btn->set_text(TTR("Instant Preview"));
			_instant_preview_btn->set_toggle_mode(true);
			_instant_preview_btn->set_tooltip_text(TTR("Instant Preview: Preview the selected resource in the editor before accepting."));
			bottom_hbox->add_child(_instant_preview_btn);

			_result_count_lbl = memnew(Label);
			_result_count_lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			bottom_hbox->add_child(_result_count_lbl);
			_update_result_count();

			_item_size_slider = memnew(HSlider);
			_item_size_slider->set_custom_minimum_size(Size2(128 * EDSCALE, 0));
			_item_size_slider->set_max(192);
			_item_size_slider->set_min(64);
			_item_size_slider->set_step(16);
			_item_size_slider->set_tooltip_text(TTR("Grid item size"));
			_item_size_slider->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
			_item_size_slider->set_value_no_signal(EditorSettings::get_singleton()->get_project_metadata("quick_open_dialog", "item_size", 100));
			_item_size_slider->set_visible(_content_display_mode == DisplayMode::GRID);
			bottom_hbox->add_child(_item_size_slider);
			_item_size_slider->connect(SceneStringName(value_changed), callable_mp(this, &EditorQuickOpenDialog::_item_size_changed));

			_display_mode_btn = memnew(Button);
			_display_mode_btn->connect(SceneStringName(pressed), callable_mp(this, &EditorQuickOpenDialog::_toggle_display_mode));
			_display_mode_btn->set_focus_mode(Control::FOCUS_NONE);
			bottom_hbox->add_child(_display_mode_btn);
		}
	}

	_search_box->connect(SceneStringName(gui_input), callable_mp(this, &EditorQuickOpenDialog::_handle_search_box_input));
	_search_box->connect(SceneStringName(text_changed), callable_mp(this, &EditorQuickOpenDialog::_search_box_text_changed));
	register_text_enter(_search_box);
	get_ok_button()->hide();
}

void EditorQuickOpenDialog::popup_dialog(const Vector<StringName> &p_base_types, const Callable &p_item_selected_callback, bool p_allow_type_switching) {
	ERR_FAIL_COND(p_base_types.is_empty());
	ERR_FAIL_COND(!p_item_selected_callback.is_valid());

	_property_object = nullptr;
	_property_path = "";
	_item_selected_callback = p_item_selected_callback;
	_allow_type_switching = p_allow_type_switching;

	_is_cycling_items = false;
	_prepare_search(p_base_types);
	_instant_preview_btn->set_visible(false);
	_finish_dialog_setup(p_base_types);
}

void EditorQuickOpenDialog::popup_dialog_for_property(const Vector<StringName> &p_base_types, Object *p_obj, const StringName &p_path, const Callable &p_item_selected_callback) {
	ERR_FAIL_NULL(p_obj);
	ERR_FAIL_COND(p_base_types.is_empty());
	ERR_FAIL_COND(!p_item_selected_callback.is_valid());

	_property_object = p_obj;
	_property_path = p_path;
	_item_selected_callback = p_item_selected_callback;
	_initial_property_value = _property_object->get(_property_path);
	_allow_type_switching = false;

	// Reset this, so that the property isn't updated immediately upon opening
	// the window.
	_initial_selection_performed = false;

	_prepare_search(p_base_types);
	_instant_preview_btn->set_visible(true);
	_finish_dialog_setup(p_base_types);
}

void EditorQuickOpenDialog::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			Color text_color = get_theme_color("font_readonly_color", EditorStringName(Editor));
			_result_count_lbl->add_theme_color_override(SceneStringName(font_normal_color), text_color);
			_no_results_lbl->add_theme_color_override(SceneStringName(font_normal_color), text_color);

			_file_context_menu->set_item_icon(SHOW_IN_FILESYSTEM, get_editor_theme_icon(SNAME("gui_show_in_file_system")));
			_file_context_menu->set_item_icon(SHOW_IN_FILE_MANAGER, get_editor_theme_icon(SNAME("gui_filesystem")));

			_instant_preview_btn->set_button_icon(get_editor_theme_icon(SNAME("gui_eye_open")));
			_fuzzy_search_btn->set_button_icon(get_editor_theme_icon(SNAME("action_search")));
			_filter_menu_btn->set_button_icon(get_editor_theme_icon(SNAME("animation_filter")));
			PopupMenu *filter_popup = _filter_menu_btn->get_popup();
			filter_popup->set_item_icon(filter_popup->get_item_index(ADDONS), get_editor_theme_icon(SNAME("action_plugin")));
			filter_popup->set_item_icon(filter_popup->get_item_index(EDITOR_CONTENT), get_editor_theme_icon(SNAME("branding_logo_monochrome")));

			Ref<StyleBox> tree_style = get_theme_stylebox(SceneStringName(panel), SNAME("Tree"));
			_no_results_pc->add_theme_style_override(SceneStringName(panel), tree_style);
			_scroll_container->add_theme_style_override(SceneStringName(panel), tree_style);

			if (_content_display_mode == DisplayMode::LIST) {
				_display_mode_btn->set_button_icon(get_editor_theme_icon(SNAME("file_thumbnail")));
			} else {
				_display_mode_btn->set_button_icon(get_editor_theme_icon(SNAME("gui_file_list")));
			}
		} break;
	}
}

void EditorQuickOpenDialog::_on_canceled() {
	if (_property_object) {
		if (Object::cast_to<MultiNodeEdit>(_property_object)) {
			Object::cast_to<MultiNodeEdit>(_property_object)->_set_impl(_property_path, _initial_property_value, "", false);
		} else {
			_property_object->set(_property_path, _initial_property_value);
		}
	}
	_cleanup_results();
	_search_box->clear();
}

void EditorQuickOpenDialog::_on_confirmed() {
	_save_selected_item();

	_update_property();
	_cleanup_results();
	_search_box->clear();
	hide();
}

void EditorQuickOpenDialog::shortcut_input(const Ref<InputEvent> &p_event) {
	// If the user is cycling through items (with up/down arrows), confirm selection when releasing the keys.
	Ref<InputEventWithModifiers> iewm = p_event;
	if (_is_cycling_items && iewm.is_valid() && p_event->is_released() && iewm->get_modifiers_mask().is_empty()) {
		_on_confirmed();
		return;
	}

	if (p_event.is_null() || !p_event->is_pressed() || p_event->is_echo()) {
		return;
	}

	Vector<StringName> new_base_types;
	if (EditorSettings *settings = EditorSettings::get_singleton()) {
		if (settings->is_shortcut("editor/quick_open", p_event)) {
			new_base_types.push_back("Resource");
		} else if (settings->is_shortcut("editor/quick_open_scene", p_event)) {
			new_base_types.push_back("PackedScene");
		} else if (settings->is_shortcut("editor/quick_open_script", p_event)) {
			new_base_types.push_back("Script");
		}
	}

	if (new_base_types.size() != 1) {
		return;
	}

	// Check if we're already showing this dialog type.
	const Vector<StringName> &current_base_types = _base_types;
	if (current_base_types.size() == 1 && current_base_types[0] == new_base_types[0]) {
		// Already showing the requested dialog type, move next.
		Ref<InputEventKey> down_event = memnew(InputEventKey);
		down_event->set_keycode(Key::DOWN);
		down_event->set_pressed(true);
		_handle_search_box_input(down_event);
		_is_cycling_items = true;
	} else {
		// Switch to the new dialog type.
		_prepare_search(new_base_types);
		_instant_preview_btn->set_visible(false);
		_is_cycling_items = false;
		set_title(_get_dialog_title(new_base_types));
		_search_box->clear();
		_search_box->grab_focus();
	}

	set_input_as_handled();
}

String EditorQuickOpenDialog::_get_dialog_title(const Vector<StringName> &p_base_types) {
	if (p_base_types.size() > 1) {
		return TTR("Select Resource");
	}

	if (p_base_types[0] == SNAME("PackedScene")) {
		return TTR("Select Scene");
	}

	return vformat(TTR("Select %s"), p_base_types[0]);
}

EditorQuickOpenDialog::DisplayMode EditorQuickOpenDialog::_get_adaptive_display_mode(const Vector<StringName> &p_base_types) {
	static const Vector<StringName> grid_preferred_types = {
		StringName("Font", true),
		StringName("Texture2D", true),
		StringName("Material", true),
		StringName("Mesh", true),
	};

	for (const StringName &type : grid_preferred_types) {
		for (const StringName &base_type : p_base_types) {
			if (base_type == type || ClassDB::is_parent_class(base_type, type)) {
				return DisplayMode::GRID;
			}
		}
	}

	return DisplayMode::LIST;
}

void EditorQuickOpenDialog::_prepare_search(const Vector<StringName> &p_base_types) {
	EditorContentResources::register_uids();
	_base_types = p_base_types;

	const int display_mode_behavior = EDITOR_GET("filesystem/quick_open_dialog/default_display_mode");
	const bool adaptive_display_mode = (display_mode_behavior == 0);
	const bool first_open = _never_opened;

	if (adaptive_display_mode) {
		_set_display_mode(_get_adaptive_display_mode(p_base_types));
	} else if (_never_opened) {
		int last = EditorSettings::get_singleton()->get_project_metadata("quick_open_dialog", "last_mode", (int)DisplayMode::LIST);
		_set_display_mode((DisplayMode)last);
	}

	_ensure_result_vector_capacity();

	const bool do_instant_preview = EDITOR_GET("filesystem/quick_open_dialog/instant_preview");
	const bool fuzzy_matching = EDITOR_GET("filesystem/quick_open_dialog/enable_fuzzy_matching");
	const bool include_addons = EDITOR_GET("filesystem/quick_open_dialog/include_addons");
	_instant_preview_btn->set_pressed_no_signal(do_instant_preview);
	_fuzzy_search_btn->set_pressed_no_signal(fuzzy_matching);
	PopupMenu *filter_popup = _filter_menu_btn->get_popup();
	filter_popup->set_item_checked(filter_popup->get_item_index(ADDONS), include_addons);
	filter_popup->set_item_checked(filter_popup->get_item_index(EDITOR_CONTENT), EDITOR_GET("filesystem/quick_open_dialog/include_editor_content"));
	_never_opened = false;

	const bool enable_highlights = EDITOR_GET("filesystem/quick_open_dialog/show_search_highlight");
	for (QuickOpenResultItemContainer *item : _result_items) {
		item->enable_highlights = enable_highlights;
	}

	bool history_modified = false;

	if (first_open && _history_file->load(_get_cache_file_path()) == OK) {
		// Load history when opening for the first time.
		_file_type_icons.insert(SNAME("__default_icon"), get_editor_theme_icon(SNAME("variant_object")));

		Vector<String> history_keys = _history_file->get_section_keys("selected_history");
		for (const String &type : history_keys) {
			const StringName type_name = type;
			const PackedStringArray history_uids = _history_file->get_value("selected_history", type);

			PackedStringArray cleaned_text_uids;
			cleaned_text_uids.resize(history_uids.size());

			Vector<ResourceUID::ID> cleaned_ids;
			cleaned_ids.resize(history_uids.size());

			{
				String *text_write = cleaned_text_uids.ptrw();
				ResourceUID::ID *id_write = cleaned_ids.ptrw();
				int i = 0;
				for (String uid : history_uids) {
					ResourceUID::ID id = ResourceUID::get_singleton()->text_to_id(uid);
					if (id == ResourceUID::INVALID_ID || !ResourceUID::get_singleton()->has_id(id)) {
						continue;
					}

					_filetypes.insert(id, type_name);
					text_write[i] = uid;
					id_write[i] = id;
					i++;
				}

				cleaned_text_uids.resize(i);
				_selected_history.insert(type, cleaned_ids);

				if (i < history_uids.size()) {
					// Some paths removed, need to update history.
					if (i == 0) {
						_history_file->erase_section_key("selected_history", type);
					} else {
						_history_file->set_value("selected_history", type, cleaned_text_uids);
					}
					history_modified = true;
				}
			}
		}
	} else if (!first_open && _base_types.size() == 1) {
		const StringName &type = _base_types[0];
		Vector<ResourceUID::ID> *history = _selected_history.getptr(type);

		if (history) {
			Vector<ResourceUID::ID> clean_history;

			for (const ResourceUID::ID &uid : *history) {
				if (ResourceUID::get_singleton()->has_id(uid)) {
					clean_history.push_back(uid);
				} else {
					history_modified = true;
				}
			}

			if (clean_history.is_empty()) {
				_selected_history.erase(type);
			} else if (history_modified) {
				*history = clean_history;
			}
		}
	}

	if (history_modified) {
		_history_file->save(_get_cache_file_path());
	}

	_create_initial_results();
}

void EditorQuickOpenDialog::_finish_dialog_setup(const Vector<StringName> &p_base_types) {
	set_process_shortcut_input(_allow_type_switching);
	get_ok_button()->set_disabled(_has_nothing_selected());
	set_title(_get_dialog_title(p_base_types));
	popup_centered_clamped(Size2(780, 650) * EDSCALE, 0.8f);
	_search_box->grab_focus();
}

void EditorQuickOpenDialog::_search_box_text_changed(const String &p_query) {
	_query = p_query;
	_update_results();
	get_ok_button()->set_disabled(_has_nothing_selected());
}

void EditorQuickOpenDialog::_handle_search_box_input(const Ref<InputEvent> &p_ie) {
	if (_num_visible_results < 0) {
		return;
	}

	Ref<InputEventKey> key_event = p_ie;
	if (key_event.is_valid() && key_event->is_pressed()) {
		bool move_selection = false;

		switch (key_event->get_keycode()) {
			case Key::UP:
			case Key::DOWN:
			case Key::PAGEUP:
			case Key::PAGEDOWN: {
				move_selection = true;
			} break;
			case Key::LEFT:
			case Key::RIGHT: {
				if (_content_display_mode == DisplayMode::GRID) {
					// Maybe strip off the shift modifier to allow non-selecting navigation by character?
					if (key_event->get_modifiers_mask().is_empty()) {
						move_selection = true;
					}
				}
			} break;
			default:
				break; // Let the event through so it will reach the search box.
		}

		if (move_selection) {
			_move_selection_index(key_event->get_keycode());
			_search_box->accept_event();
		}
	}
}

void EditorQuickOpenDialog::_item_pressed(bool p_double_click) {
	// A double-click should always be taken as a "confirm" action.
	if (p_double_click) {
		_on_confirmed();
		return;
	}

	// Single-clicks should be taken as a "confirm" action only if Instant Preview
	// isn't currently enabled, or the property object is null for some reason.
	if (!_is_instant_preview_active()) {
		_on_confirmed();
	}
}

void EditorQuickOpenDialog::_selection_changed() {
	// This prevents the property from being changed the first time the Quick Open
	// window is opened.
	if (!_initial_selection_performed) {
		_initial_selection_performed = true;
		return;
	}

	if (_is_instant_preview_active()) {
		_preview_property();
	}
}

bool EditorQuickOpenDialog::_is_instant_preview_active() const {
	return _property_object != nullptr && _instant_preview_btn->is_visible() && _instant_preview_btn->is_pressed();
}

void EditorQuickOpenDialog::_preview_property() {
	ERR_FAIL_COND(_get_selected() == ResourceUID::INVALID_ID);
	String path = _get_selected_path();

	Ref<Resource> loaded_resource = ResourceLoader::load(path);
	ERR_FAIL_COND_MSG(loaded_resource.is_null(), "Cannot load resource from path '" + path + "'.");

	Resource *res = Object::cast_to<Resource>(_property_object);
	if (res) {
		HashSet<Resource *> resources_found;
		resources_found.insert(res);
		if (EditorNode::find_recursive_resources(loaded_resource, resources_found)) {
			EditorToasterContainer::get_singleton()->popup_str(TTR("Recursion detected, Instant Preview failed."), EditorToasterContainer::ERROR);
			loaded_resource = Ref<Resource>();
		}
	}

	// MultiNodeEdit has adding to the undo/redo stack baked into its set function.
	// As such, we have to specifically call a version of its setter that doesn't
	// create undo/redo actions.
	_property_object->set_block_signals(true);
	if (Object::cast_to<MultiNodeEdit>(_property_object)) {
		Object::cast_to<MultiNodeEdit>(_property_object)->_set_impl(_property_path, loaded_resource, "", false);
	} else {
		_property_object->set(_property_path, loaded_resource);
	}
	_property_object->set_block_signals(false);
}

void EditorQuickOpenDialog::_update_property() {
	// Set the property back to the initial value first, so that the undo action
	// has the correct object.
	if (_property_object) {
		if (Object::cast_to<MultiNodeEdit>(_property_object)) {
			Object::cast_to<MultiNodeEdit>(_property_object)->_set_impl(_property_path, _initial_property_value, "", false);
		} else {
			_property_object->set(_property_path, _initial_property_value);
		}
	}

	if (!_item_selected_callback.is_valid()) {
		String err_msg = "The callback provided to the Quick Open dialog was invalid.";
		if (_is_instant_preview_active()) {
			err_msg += " Try disabling \"Instant Preview\" as a workaround.";
		}
		ERR_FAIL_MSG(err_msg);
	}

	_item_selected_callback.call(_get_selected_path());
}

bool EditorQuickOpenDialog::_has_nothing_selected() const {
	return _selection_index < 0;
}

ResourceUID::ID EditorQuickOpenDialog::_get_selected() const {
	ERR_FAIL_COND_V_MSG(_has_nothing_selected(), ResourceUID::INVALID_ID, "Tried to get selected file, but nothing was selected.");
	return _candidates[_selection_index].uid;
}

String EditorQuickOpenDialog::_get_selected_path() const {
	ERR_FAIL_COND_V_MSG(_has_nothing_selected(), "", "Tried to get selected file path, but nothing was selected.");
	String path = ResourceUID::get_singleton()->get_id_path(_candidates[_selection_index].uid);
	ERR_FAIL_COND_V_MSG(path.is_empty(), "", "Failed to get selected file path.");
	return path;
}

void EditorQuickOpenDialog::_save_selected_item() {
	if (_base_types.size() > 1) {
		// Getting the type of the file and checking which base type it belongs to should be possible.
		// However, for now these are not supported, and we don't record this.
		return;
	}

	const StringName &base_type = _base_types[0];
	ResourceUID::ID selected = _get_selected();
	Vector<ResourceUID::ID> *type_history = _selected_history.getptr(base_type);

	if (!type_history) {
		_selected_history.insert(base_type, Vector<ResourceUID::ID>());
		type_history = _selected_history.getptr(base_type);
	} else {
		for (int i = 0; i < type_history->size(); i++) {
			if (selected == type_history->get(i)) {
				type_history->remove_at(i);
				break;
			}
		}
	}

	_history_set.insert(selected);
	type_history->insert(0, selected);
	if (type_history->size() > MAX_HISTORY_SIZE) {
		type_history->resize(MAX_HISTORY_SIZE);
	}

	PackedStringArray history_uids;
	history_uids.resize(type_history->size());
	{
		String *uids_write = history_uids.ptrw();

		int i = 0;
		for (const ResourceUID::ID &uid : *type_history) {
			uids_write[i] = ResourceUID::get_singleton()->id_to_text(uid);
			i++;
		}
	}
	_history_file->set_value("selected_history", base_type, history_uids);
	_history_file->save(_get_cache_file_path());
}

void EditorQuickOpenDialog::_cleanup_results() {
	_num_visible_results = 0;
	_update_result_count();
	_candidates.clear();
	_history_set.clear();
	_select_item(-1);

	for (QuickOpenResultItemContainer *item : _result_items) {
		item->reset();
	}
}

void EditorQuickOpenDialog::_update_results() {
	_candidates.clear();
	_candidates_uids.clear();

	if (_max_total_results != 0) {
		if (_query.is_empty()) {
			_use_default_candidates();
		} else {
			_score_and_sort_candidates();
		}
	}

	_update_result_items(MIN(_candidates.size(), _max_total_results), 0);
}

void EditorQuickOpenDialog::_ensure_result_vector_capacity() {
	const int target_size = EDITOR_GET("filesystem/quick_open_dialog/max_results");
	const int initial_size = _result_items.size();
	for (int i = target_size; i < initial_size; i++) {
		memdelete(_result_items[i]);
	}
	_result_items.resize(target_size);
	for (int i = initial_size; i < target_size; i++) {
		_result_items.write[i] = _create_result_item(i);
	}
}

QuickOpenResultItemContainer *EditorQuickOpenDialog::_create_result_item(int p_index) {
	QuickOpenResultItemContainer *item = memnew(QuickOpenResultItemContainer(_content_display_mode));
	item->connect(SceneStringName(gui_input), callable_mp(this, &EditorQuickOpenDialog::_item_input).bind(p_index));
	item->set_grid_item_scale(_item_size_slider->get_value() / 100.0);
	item->enable_highlights = EDITOR_GET("filesystem/quick_open_dialog/show_search_highlight");
	_get_result_root()->add_child(item);
	return item;
}

void EditorQuickOpenDialog::_sort_uids(int p_max_results) {
	struct FilepathComparator {
		bool operator()(const ResourceUID::ID &p_lhs, const ResourceUID::ID &p_rhs) const {
			String lhs_path = ResourceUID::get_singleton()->get_id_path(p_lhs);
			String rhs_path = ResourceUID::get_singleton()->get_id_path(p_rhs);

			// Sort on (length, alphanumeric) to prioritize shorter filepaths
			return lhs_path.length() == rhs_path.length() ? lhs_path < rhs_path : lhs_path.length() < rhs_path.length();
		}
	};

	SortArray<ResourceUID::ID, FilepathComparator> sorter{};

	if ((int)_uids.size() > p_max_results) {
		sorter.partial_sort(0, _uids.size(), p_max_results, _uids.ptr());
	} else {
		sorter.sort(_uids.ptr(), _uids.size());
	}
}

void EditorQuickOpenDialog::_create_initial_results() {
	_file_type_icons.clear();
	_file_type_icons.insert(SNAME("__default_icon"), get_editor_theme_icon(SNAME("variant_object")));
	_uids.clear();
	_filetypes.clear();
	_history_set.clear();

	Vector<ResourceUID::ID> *history = _get_history();
	if (history) {
		for (const ResourceUID::ID &uid : *history) {
			_history_set.insert(uid);
		}
	}

	const PopupMenu *filter_popup = _filter_menu_btn->get_popup();
	_find_uids_in_folder(EditorFileSystem::get_singleton()->get_filesystem(), filter_popup->is_item_checked(filter_popup->get_item_index(ADDONS)));
	if (filter_popup->is_item_checked(filter_popup->get_item_index(EDITOR_CONTENT))) {
		_find_editor_content_resources();
	}
	_sort_uids(_result_items.size());
	_max_total_results = MIN(_uids.size(), _result_items.size());
	_update_results();
}

void EditorQuickOpenDialog::_find_uids_in_folder(EditorFileSystemDirectory *p_directory, bool p_include_addons) {
	for (int i = 0; i < p_directory->get_subdir_count(); i++) {
		if (p_include_addons || p_directory->get_name() != "addons") {
			_find_uids_in_folder(p_directory->get_subdir(i), p_include_addons);
		}
	}

	for (int i = 0; i < p_directory->get_file_count(); i++) {
		ResourceUID::ID uid = p_directory->get_file_uid(i);
		if (uid == ResourceUID::INVALID_ID) {
			continue;
		}

		const StringName engine_type = p_directory->get_file_type(i);
		const StringName script_type = p_directory->get_file_resource_script_class(i);

		const bool is_engine_type = script_type == StringName();
		const StringName &actual_type = is_engine_type ? engine_type : script_type;

		for (const StringName &parent_type : _base_types) {
			bool is_valid = ClassDB::is_parent_class(engine_type, parent_type) || (!is_engine_type && EditorNode::get_editor_data().script_class_is_parent(script_type, parent_type));

			if (is_valid) {
				_uids.push_back(uid);
				_filetypes.insert(uid, actual_type);
				break; // Stop testing base types as soon as we get a match.
			}
		}
	}
}

void EditorQuickOpenDialog::_find_editor_content_resources() {
	for (const String &path : EditorContentResources::get_paths()) {
		const StringName type = EditorContentResources::get_resource_type(path);
		for (const StringName &base_type : _base_types) {
			if (ClassDB::is_parent_class(type, base_type)) {
				const ResourceUID::ID uid = EditorContentResources::get_resource_uid(path);
				_uids.push_back(uid);
				_filetypes.insert(uid, type);
				break;
			}
		}
	}
}

Vector<ResourceUID::ID> *EditorQuickOpenDialog::_get_history() {
	if (_base_types.size() == 1) {
		return _selected_history.getptr(_base_types[0]);
	}
	return nullptr;
}

void EditorQuickOpenDialog::_add_candidate(QuickOpenResultCandidate &r_candidate) {
	// History can contain content hidden by the current source filters.
	const StringName *actual_type = _filetypes.getptr(r_candidate.uid);
	if (!actual_type || _candidates.size() >= _max_total_results) {
		return;
	}
	ERR_FAIL_COND(!ResourceUID::get_singleton()->has_id(r_candidate.uid));

	String file_path = ResourceUID::get_singleton()->get_id_path(r_candidate.uid);

	// Verify that a PackedScene is actually a "real" Scene if in a Open Scene context.
	if (_base_types.size() == 1 && _base_types[0] == SNAME("PackedScene")) {
		static FixedVector<String, 3> valid_extensions = { "tscn", "scn", "res" };
		bool is_valid_type = false;
		for (const String &ext : valid_extensions) {
			if (file_path.has_extension(ext)) {
				is_valid_type = true;
				break;
			}
		}
		if (!is_valid_type) {
			return;
		}
	}

	EditorResourcePreview::PreviewItem item = EditorResourcePreview::get_singleton()->get_resource_preview_if_available(file_path);
	if (item.preview.is_valid()) {
		r_candidate.thumbnail = item.preview;
	} else if (_file_type_icons.has(*actual_type)) {
		r_candidate.thumbnail = *_file_type_icons.getptr(*actual_type);
	} else if (has_theme_icon(editor_get_type_icon_name(*actual_type), EditorStringName(Editor))) {
		r_candidate.thumbnail = get_editor_theme_icon(editor_get_type_icon_name(*actual_type));
		_file_type_icons.insert(*actual_type, r_candidate.thumbnail);
	} else {
		r_candidate.thumbnail = *_file_type_icons.getptr(SNAME("__default_icon"));
	}

	_candidates.push_back(r_candidate);
	_candidates_uids.insert(r_candidate.uid);
	if (item.preview.is_null() && EditorContentResources::get_resource_uid(file_path) != ResourceUID::INVALID_ID) {
		EditorResourcePreview::get_singleton()->queue_resource_preview(file_path, callable_mp(this, &EditorQuickOpenDialog::_editor_content_preview_ready));
	}
}

void EditorQuickOpenDialog::_editor_content_preview_ready(const String &p_path, const Ref<Texture2D> &p_preview, const Ref<Texture2D> &p_small_preview) {
	if (p_preview.is_null()) {
		return;
	}
	const ResourceUID::ID uid = EditorContentResources::get_resource_uid(p_path);
	for (int i = 0; i < MIN(_num_visible_results, _candidates.size()); i++) {
		if (_candidates[i].uid == uid) {
			_candidates.write[i].thumbnail = p_preview;
			_result_items[i]->set_content(_candidates[i], _history_set.has(uid));
			break;
		}
	}
}

Vector<Ref<FuzzySearchMatch>> EditorQuickOpenDialog::_get_fuzzy_search_results() {
	FuzzySearch fuzzy_search;
	fuzzy_search.set_start_offset(6); // Don't match against "res://" at the start of each filepath.
	fuzzy_search.set_case_sensitive(!_query.is_lowercase());
	fuzzy_search.set_max_results(_max_total_results);
	bool fuzzy_matching = EDITOR_GET("filesystem/quick_open_dialog/enable_fuzzy_matching");
	int max_misses = EDITOR_GET("filesystem/quick_open_dialog/max_fuzzy_misses");
	fuzzy_search.set_use_exact_tokens(!fuzzy_matching);
	fuzzy_search.set_max_misses(fuzzy_matching ? max_misses : 0);

	PackedStringArray paths;
	paths.reserve_exact(_uids.size());

	for (const ResourceUID::ID &uid : _uids) {
		paths.push_back(ResourceUID::get_singleton()->get_id_path(uid));
	}

	return fuzzy_search.search_all(_query, paths);
}

void EditorQuickOpenDialog::_use_default_candidates() {
	Vector<ResourceUID::ID> *history = _get_history();
	if (history) {
		for (const ResourceUID::ID &uid : *history) {
			bool success;
			QuickOpenResultCandidate candidate = QuickOpenResultCandidate::from_uid(uid, success);
			if (!success) {
				continue;
			}
			_add_candidate(candidate);
		}
	}

	for (const ResourceUID::ID &uid : _uids) {
		if (_candidates.size() >= _max_total_results) {
			break;
		}
		if (_candidates_uids.has(uid)) {
			continue;
		}

		bool success;
		QuickOpenResultCandidate candidate = QuickOpenResultCandidate::from_uid(uid, success);
		if (!success) {
			continue;
		}

		_add_candidate(candidate);
	}
}

void EditorQuickOpenDialog::_score_and_sort_candidates() {
	for (const Ref<FuzzySearchMatch> &result : _get_fuzzy_search_results()) {
		bool success;
		QuickOpenResultCandidate candidate = QuickOpenResultCandidate::from_result(result, success);
		if (!success) {
			continue;
		}

		_add_candidate(candidate);
	}
}

void EditorQuickOpenDialog::_update_result_items(int p_new_visible_results_count, int p_new_selection_index) {
	// Only need to update items that were not hidden in previous update.
	int num_items_needing_updates = MAX(_num_visible_results, p_new_visible_results_count);
	_num_visible_results = p_new_visible_results_count;
	_update_result_count();

	for (int i = 0; i < num_items_needing_updates; i++) {
		QuickOpenResultItemContainer *item = _result_items[i];

		if (i < _num_visible_results) {
			item->set_content(_candidates[i], _history_set.has(_candidates[i].uid));
		} else {
			item->reset();
		}
	};

	const bool any_results = _num_visible_results > 0;
	_select_item(any_results ? p_new_selection_index : -1);

	_scroll_container->set_visible(any_results);
	_no_results_pc->set_visible(!any_results);

	if (!any_results) {
		if (_uids.is_empty()) {
			_no_results_lbl->set_text(TTR("No files found for this type"));
		} else {
			_no_results_lbl->set_text(TTR("No results found"));
		}
	}
}

void EditorQuickOpenDialog::_update_result_count() {
	_result_count_lbl->set_text(vformat(TTRN("%d item", "%d items", _num_visible_results), _num_visible_results));
}

void EditorQuickOpenDialog::_move_selection_index(Key p_key) {
	// Don't move selection if there are no results.
	if (_num_visible_results <= 0) {
		return;
	}
	const int max_index = _num_visible_results - 1;

	int idx = _selection_index;
	if (_content_display_mode == DisplayMode::LIST) {
		if (p_key == Key::UP) {
			idx = (idx == 0) ? max_index : (idx - 1);
		} else if (p_key == Key::DOWN) {
			idx = (idx == max_index) ? 0 : (idx + 1);
		} else if (p_key == Key::PAGEUP) {
			idx = (idx == 0) ? idx : MAX(idx - 10, 0);
		} else if (p_key == Key::PAGEDOWN) {
			idx = (idx == max_index) ? idx : MIN(idx + 10, max_index);
		}
	} else {
		int column_count = _grid_hflow->get_line_max_child_count();

		if (p_key == Key::LEFT) {
			idx = (idx == 0) ? max_index : (idx - 1);
		} else if (p_key == Key::RIGHT) {
			idx = (idx == max_index) ? 0 : (idx + 1);
		} else if (p_key == Key::UP) {
			idx = (idx == 0) ? max_index : MAX(idx - column_count, 0);
		} else if (p_key == Key::DOWN) {
			idx = (idx == max_index) ? 0 : MIN(idx + column_count, max_index);
		} else if (p_key == Key::PAGEUP) {
			idx = (idx == 0) ? idx : MAX(idx - (3 * column_count), 0);
		} else if (p_key == Key::PAGEDOWN) {
			idx = (idx == max_index) ? idx : MIN(idx + (3 * column_count), max_index);
		}
	}

	_select_item(idx);
}

void EditorQuickOpenDialog::_select_item(int p_index) {
	if (!_has_nothing_selected()) {
		_result_items[_selection_index]->highlight_item(false);
	}

	_selection_index = p_index;

	if (_has_nothing_selected()) {
		return;
	}

	_result_items[_selection_index]->highlight_item(true);

	_selection_changed();

	const QuickOpenResultItemContainer *item = _result_items[_selection_index];

	// Copied from Tree.
	const int selected_position = item->get_position().y;
	const int selected_size = item->get_size().y;
	const int scroll_window_size = _scroll_container->get_size().y;
	const int scroll_position = _scroll_container->get_v_scroll();

	if (selected_position <= scroll_position) {
		_scroll_container->set_v_scroll(selected_position);
	} else if (selected_position + selected_size > scroll_position + scroll_window_size) {
		_scroll_container->set_v_scroll(selected_position + selected_size - scroll_window_size);
	}
}

void EditorQuickOpenDialog::_item_input(const Ref<InputEvent> &p_ev, int p_index) {
	Ref<InputEventMouseButton> mb = p_ev;

	if (mb.is_valid() && mb->is_pressed()) {
		if (mb->get_button_index() == MouseButton::LEFT) {
			_select_item(p_index);
			_item_pressed(mb->is_double_click());
		} else if (mb->get_button_index() == MouseButton::RIGHT) {
			_select_item(p_index);
			if (EditorContentResources::get_resource_uid(_get_selected_path()) != ResourceUID::INVALID_ID) {
				return;
			}
			_file_context_menu->set_position(_result_items[p_index]->get_screen_position() + mb->get_position());
			_file_context_menu->reset_size();
			_file_context_menu->popup();
		}
	}
}

CanvasItem *EditorQuickOpenDialog::_get_result_root() {
	if (_content_display_mode == DisplayMode::LIST) {
		return _list_vbox;
	} else {
		return _grid_hflow;
	}
}

void EditorQuickOpenDialog::_set_display_mode(DisplayMode p_display_mode) {
	_item_size_slider->set_visible(p_display_mode == DisplayMode::GRID);
	CanvasItem *prev_root = _get_result_root();

	if (prev_root->is_visible() && _content_display_mode == p_display_mode) {
		return;
	}

	const bool mode_changed = _content_display_mode != p_display_mode;
	_content_display_mode = p_display_mode;
	CanvasItem *next_root = _get_result_root();

	EditorSettings::get_singleton()->set_project_metadata("quick_open_dialog", "last_mode", (int)_content_display_mode);

	prev_root->hide();
	next_root->show();

	if (mode_changed) {
		for (int i = 0; i < _result_items.size(); i++) {
			memdelete(_result_items[i]);
			_result_items.write[i] = _create_result_item(i);
		}
	}

	_update_result_items(_num_visible_results, _selection_index);

	if (_content_display_mode == DisplayMode::LIST) {
		_display_mode_btn->set_button_icon(get_editor_theme_icon(SNAME("file_thumbnail")));
		_display_mode_btn->set_tooltip_text(TTR("Grid view"));
	} else {
		_display_mode_btn->set_button_icon(get_editor_theme_icon(SNAME("gui_file_list")));
		_display_mode_btn->set_tooltip_text(TTR("List view"));
	}
}

void EditorQuickOpenDialog::_toggle_display_mode() {
	DisplayMode new_display_mode = (_content_display_mode == DisplayMode::LIST) ? DisplayMode::GRID : DisplayMode::LIST;
	_set_display_mode(new_display_mode);
}

void EditorQuickOpenDialog::_toggle_instant_preview(bool p_pressed) {
	EditorSettings::get_singleton()->set("filesystem/quick_open_dialog/instant_preview", p_pressed);
}

void EditorQuickOpenDialog::_toggle_fuzzy_search(bool p_pressed) {
	EditorSettings::get_singleton()->set("filesystem/quick_open_dialog/enable_fuzzy_matching", p_pressed);
	_update_results();
}

void EditorQuickOpenDialog::_filter_option_selected(int p_option) {
	PopupMenu *filter_popup = _filter_menu_btn->get_popup();
	const int index = filter_popup->get_item_index(p_option);
	ERR_FAIL_COND(index < 0);

	const bool enabled = !filter_popup->is_item_checked(index);
	filter_popup->set_item_checked(index, enabled);
	switch (p_option) {
		case ADDONS: {
			EditorSettings::get_singleton()->set("filesystem/quick_open_dialog/include_addons", enabled);
		} break;
		case EDITOR_CONTENT: {
			EditorSettings::get_singleton()->set("filesystem/quick_open_dialog/include_editor_content", enabled);
		} break;
	}
	_cleanup_results();
	_create_initial_results();
}

void EditorQuickOpenDialog::_item_size_changed(double p_value) {
	for (QuickOpenResultItemContainer *item : _result_items) {
		item->set_grid_item_scale(p_value / 100.0);
	}
	EditorSettings::get_singleton()->set_project_metadata("quick_open_dialog", "item_size", (int)p_value);
}

void EditorQuickOpenDialog::_menu_option(int p_option) {
	ERR_FAIL_COND(_get_selected() == ResourceUID::INVALID_ID);
	String selected_path = _get_selected_path();

	switch (p_option) {
		case SHOW_IN_FILESYSTEM: {
			FileSystemDock::get_singleton()->navigate_to_path(selected_path);
		} break;
		case SHOW_IN_FILE_MANAGER: {
			String dir = ProjectSettings::get_singleton()->globalize_path(selected_path);
			OS::get_singleton()->shell_show_in_file_manager(dir, true);
		} break;
	}
}

String EditorQuickOpenDialog::_get_cache_file_path() const {
	return EditorPaths::get_singleton()->get_project_settings_dir().path_join("quick_open_dialog_cache.cfg");
}
