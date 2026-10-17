/**************************************************************************/
/*  project_plugins_dialog.cpp                                            */
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

#include "project_plugins_dialog.h"

#include "core/config/engine.h"
#include "core/object/callable_mp.h"
#include "editor/editor_string_names.h"
#include "editor/gui/dialogs/config_plugin_dialog.h"
#include "editor/managers/editor_managers.h"
#include "editor/managers/project_plugin_manager.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/separator.h"
#include "scene/gui/texture_rect.h"
#include "scene/gui/tree.h"
#include "servers/text/text_server.h"

ProjectPluginsDialog::ProjectPluginsDialog() {
	_manager = EditorManagers::get_singleton()->get_project_plugin_manager();

	set_flag(Window::FLAG_RESIZE_DISABLED, false);
	set_ok_button_text(TTRC("Close"));
	set_title(TTRC("Project Plugins"));

	_config_plugin_dialog = memnew(ConfigPluginDialog);
	_config_plugin_dialog->connect("plugin_saved", callable_mp(this, &ProjectPluginsDialog::update_plugins));
	add_child(_config_plugin_dialog);

	VBoxContainer *content_vbox = memnew(VBoxContainer);
	add_child(content_vbox);
	{
		if (Engine::get_singleton()->is_recovery_mode_hint()) {
			HBoxContainer *recovery_hbox = memnew(HBoxContainer);
			content_vbox->add_child(recovery_hbox);
			{
				_recovery_mode_icon = memnew(TextureRect);
				_recovery_mode_icon->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
				recovery_hbox->add_child(_recovery_mode_icon);

				Label *recovery_lbl = memnew(Label(TTRC("Recovery mode is enabled. Enabled plugins will not run while this mode is active.")));
				recovery_lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
				recovery_lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
				recovery_lbl->set_theme_type_variation("HeaderSmall");
				recovery_hbox->add_child(recovery_lbl);
			}

			HSeparator *separator = memnew(HSeparator);
			content_vbox->add_child(separator);
		}

		HBoxContainer *title_hbox = memnew(HBoxContainer);
		content_vbox->add_child(title_hbox);
		{
			Label *title_lbl = memnew(Label(TTRC("Installed Plugins:")));
			title_lbl->set_theme_type_variation("HeaderSmall");
			title_hbox->add_child(title_lbl);
			title_hbox->add_spacer();

			Button *create_plugin_btn = memnew(Button(TTRC("Create New Plugin")));
			create_plugin_btn->connect(SceneStringName(pressed), callable_mp(this, &ProjectPluginsDialog::_create_clicked));
			title_hbox->add_child(create_plugin_btn);
		}

		_plugin_list = memnew(Tree);
		_plugin_list->connect("item_edited", callable_mp(this, &ProjectPluginsDialog::_plugin_activity_changed), CONNECT_DEFERRED);
		_plugin_list->set_columns(COUNT);
		for (int column = 0; column < COUNT; column++) {
			_plugin_list->set_column_clip_content(column, true);
		}
		_plugin_list->set_column_custom_minimum_width(STATUS, 80 * EDSCALE);
		_plugin_list->set_column_custom_minimum_width(VERSION, 100 * EDSCALE);
		_plugin_list->set_column_custom_minimum_width(AUTHOR, 250 * EDSCALE);
		_plugin_list->set_column_custom_minimum_width(EDIT, 40 * EDSCALE);
		for (int column = 0; column < COUNT; column++) {
			_plugin_list->set_column_expand(column, column == NAME);
		}
		_plugin_list->set_column_title(STATUS, TTRC("Enabled"));
		_plugin_list->set_column_title(NAME, TTRC("Name"));
		_plugin_list->set_column_title(VERSION, TTRC("Version"));
		_plugin_list->set_column_title(AUTHOR, TTRC("Author"));
		_plugin_list->set_column_title(EDIT, TTRC("Edit"));
		for (int column = 0; column < COUNT; column++) {
			_plugin_list->set_column_title_alignment(column, HORIZONTAL_ALIGNMENT_LEFT);
		}
		_plugin_list->set_column_titles_visible(true);
		_plugin_list->set_custom_minimum_size(Size2(0, 280) * EDSCALE);
		_plugin_list->set_hide_folding(true);
		_plugin_list->set_hide_root(true);
		_plugin_list->set_theme_type_variation("TreeTable");
		_plugin_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		content_vbox->add_child(_plugin_list);
	}

	_manager->connect("plugins_changed", callable_mp(this, &ProjectPluginsDialog::update_plugins), CONNECT_DEFERRED);
}

void ProjectPluginsDialog::popup_plugins() {
	update_plugins();
	popup_centered_clamped(Size2(1000, 600) * EDSCALE, 0.8);
}

void ProjectPluginsDialog::update_plugins() {
	_updating = true;
	_plugin_list->clear();
	TreeItem *root = _plugin_list->create_item();
	const Color disabled_color = get_theme_color(SNAME("font_disabled_color"), EditorStringName(Editor));
	for (const ProjectPluginManager::PluginEntry &entry : _manager->get_plugins()) {
		const PackedInt32Array boundaries = TS->string_get_word_breaks(entry.description, "", 80);
		String wrapped_description;
		for (int i = 0; i < boundaries.size(); i += 2) {
			const int start = boundaries[i];
			const int end = boundaries[i + 1];
			wrapped_description += "\n" + entry.description.substr(start, end - start).rstrip("\n");
		}

		TreeItem *item = _plugin_list->create_item(root);
		item->set_text(NAME, entry.name);
		item->set_auto_translate_mode(NAME, AUTO_TRANSLATE_MODE_DISABLED);
		if (!entry.enabled) {
			item->set_custom_color(NAME, disabled_color);
		}
		item->set_tooltip_text(NAME, vformat(TTR("Name: %s\nPath: %s\nMain Script: %s\n\n%s"), entry.name, entry.config_path, entry.script, wrapped_description));
		item->set_metadata(NAME, entry.config_path);
		item->set_text(VERSION, entry.version);
		item->set_auto_translate_mode(VERSION, AUTO_TRANSLATE_MODE_DISABLED);
		item->set_custom_font(VERSION, get_theme_font("source", EditorStringName(EditorFonts)));
		item->set_metadata(VERSION, entry.script);
		item->set_text(AUTHOR, entry.author);
		item->set_auto_translate_mode(AUTHOR, AUTO_TRANSLATE_MODE_DISABLED);
		item->set_metadata(AUTHOR, entry.description);
		item->set_cell_mode(STATUS, TreeItem::CELL_MODE_CHECK);
		item->set_text(STATUS, TTRC("On"));
		item->set_checked(STATUS, entry.enabled);
		item->set_editable(STATUS, true);
		item->add_button(EDIT, get_editor_theme_icon(SNAME("Edit")), EDIT_PLUGIN, false, TTRC("Edit Plugin"));
	}
	_updating = false;
}

void ProjectPluginsDialog::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_WM_WINDOW_FOCUS_IN: {
			update_plugins();
		} break;

		case NOTIFICATION_READY: {
			_plugin_list->connect("button_clicked", callable_mp(this, &ProjectPluginsDialog::_cell_button_pressed));
		} break;

		case NOTIFICATION_TRANSLATION_CHANGED: {
			if (_plugin_list->get_root()) {
				update_plugins();
			}
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			if (Engine::get_singleton()->is_recovery_mode_hint()) {
				_recovery_mode_icon->set_texture(get_editor_theme_icon(SNAME("NodeWarning")));
			}
		} break;
	}
}

void ProjectPluginsDialog::_plugin_activity_changed() {
	if (_updating) {
		return;
	}

	TreeItem *ti = _plugin_list->get_edited();
	ERR_FAIL_NULL(ti);
	bool checked = ti->is_checked(STATUS);
	String name = ti->get_metadata(NAME);

	_manager->set_plugin_enabled(name, checked, true);

	bool is_enabled = _manager->is_plugin_enabled(name);

	if (is_enabled != checked) {
		_updating = true;
		ti->set_checked(STATUS, is_enabled);
		_updating = false;
	}
	if (is_enabled) {
		ti->clear_custom_color(NAME);
	} else {
		ti->set_custom_color(NAME, get_theme_color(SNAME("font_disabled_color"), EditorStringName(Editor)));
	}
}

void ProjectPluginsDialog::_create_clicked() {
	_config_plugin_dialog->popup_create();
}

void ProjectPluginsDialog::_cell_button_pressed(Object *p_item, int p_column, int p_id, MouseButton p_button) {
	if (p_button != MouseButton::LEFT) {
		return;
	}
	TreeItem *item = Object::cast_to<TreeItem>(p_item);
	if (!item) {
		return;
	}
	if (p_id == EDIT_PLUGIN) {
		if (p_column == EDIT) {
			String dir = item->get_metadata(NAME);
			_config_plugin_dialog->popup_edit(dir);
		}
	}
}
