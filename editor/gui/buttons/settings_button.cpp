/**************************************************************************/
/*  settings_button.cpp                                                   */
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

#include "settings_button.h"

#include "core/error/error_macros.h"
#include "core/object/callable_mp.h"
#include "editor/gui/dialogs/editor_settings_dialog.h"
#include "editor/gui/dialogs/project_settings_editor.h"
#include "editor/settings/editor_command_palette.h"
#include "scene/gui/popup_menu.h"

void SettingsButton::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_POSTINITIALIZE: {
			_initialize();
		} break;
		case NOTIFICATION_READY:
		case NOTIFICATION_THEME_CHANGED: {
			if (is_inside_tree()) {
				_update_icons();
			}
		} break;
	}
}

void SettingsButton::_initialize() {
	set_focus_mode(FOCUS_NONE);
	set_tooltip_auto_translate_mode(AUTO_TRANSLATE_MODE_ALWAYS);
	set_tooltip_text(TTRC("Settings"));

	PopupMenu *settings_popup = get_popup();
	settings_popup->set_auto_translate_mode(AUTO_TRANSLATE_MODE_ALWAYS);
	settings_popup->add_shortcut(ED_SHORTCUT_AND_COMMAND("editor/editor_settings", TTRC("Editor Settings"), KeyModifierMask::CMD_OR_CTRL + Key::COMMA), EDITOR_SETTINGS);
	settings_popup->set_item_text(settings_popup->get_item_index(EDITOR_SETTINGS), TTRC("Editor Settings"));
	settings_popup->add_shortcut(ED_SHORTCUT_AND_COMMAND("editor/project_settings", TTRC("Project Settings"), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::SHIFT + Key::COMMA, TTRC("Project Settings")), PROJECT_SETTINGS);
	settings_popup->set_item_text(settings_popup->get_item_index(PROJECT_SETTINGS), TTRC("Project Settings"));
	settings_popup->connect(SceneStringName(id_pressed), callable_mp(this, &SettingsButton::_item_selected));
}

void SettingsButton::_item_selected(int p_id) {
	switch (p_id) {
		case EDITOR_SETTINGS: {
			EditorSettingsDialog *settings_dialog = EditorSettingsDialog::get_singleton();
			ERR_FAIL_NULL(settings_dialog);
			settings_dialog->popup_edit_settings();
		} break;
		case PROJECT_SETTINGS: {
			ProjectSettingsEditor *project_settings = ProjectSettingsEditor::get_singleton();
			ERR_FAIL_NULL(project_settings);
			project_settings->popup_project_settings();
		} break;
	}
}

void SettingsButton::_update_icons() {
	set_button_icon(get_editor_theme_icon(SNAME("gui_settings")));

	PopupMenu *settings_popup = get_popup();
	settings_popup->set_item_icon(settings_popup->get_item_index(EDITOR_SETTINGS), get_editor_theme_icon(SNAME("viewport_tools")));
	settings_popup->set_item_icon(settings_popup->get_item_index(PROJECT_SETTINGS), get_editor_theme_icon(SNAME("gui_class_list")));
}
