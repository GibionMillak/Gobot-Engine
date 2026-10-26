/**************************************************************************/
/*  editor_help_button.cpp                                                */
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

#include "editor_help_button.h"

#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "editor/editor_node.h"
#include "editor/managers/editor_dialog_manager.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "scene/gui/popup_menu.h"

void EditorHelpButton::_notification(int p_what) {
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

void EditorHelpButton::_initialize() {
	set_focus_mode(FOCUS_NONE);
	set_switch_on_hover(true);
	set_tooltip_auto_translate_mode(AUTO_TRANSLATE_MODE_ALWAYS);
	set_tooltip_text(TTRC("Help"));

	PopupMenu *help_popup = get_popup();
	help_popup->set_auto_translate_mode(AUTO_TRANSLATE_MODE_ALWAYS);
	help_popup->add_shortcut(ED_SHORTCUT_AND_COMMAND("editor/editor_help", TTRC("Documentation"), Key::F1), DOCUMENTATION);
	ED_SHORTCUT_OVERRIDE("editor/editor_help", "macos", KeyModifierMask::ALT | Key::SPACE);
	help_popup->add_shortcut(ED_SHORTCUT("editor/command_palette", TTRC("Command Palette"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::P), COMMAND_PALETTE);
	help_popup->add_shortcut(ED_SHORTCUT_AND_COMMAND("editor/system_information", TTRC("System Information")), SYSTEM_INFORMATION);
	help_popup->add_separator();
	help_popup->add_shortcut(ED_SHORTCUT_AND_COMMAND("editor/report_a_bug", TTRC("Report a Bug")), REPORT_A_BUG);
	help_popup->add_shortcut(ED_SHORTCUT_AND_COMMAND("editor/about", TTRC("About Gobot")), ABOUT);
	help_popup->connect(SceneStringName(id_pressed), callable_mp(this, &EditorHelpButton::_item_selected));
}

void EditorHelpButton::_item_selected(int p_id) {
	switch (p_id) {
		case DOCUMENTATION: {
			EditorNode::get_singleton()->emit_signal(SNAME("request_help_search"), "");
		} break;
		case COMMAND_PALETTE: {
			EditorCommandPalette::get_singleton()->open_popup();
		} break;
		case SYSTEM_INFORMATION: {
			EditorDialogManager::get_singleton()->show_system_information();
		} break;
		case REPORT_A_BUG: {
			OS::get_singleton()->shell_open("https://github.com/GibionMillak/Gobot-Engine/issues");
		} break;
		case ABOUT: {
			EditorDialogManager::get_singleton()->show_about();
		} break;
	}
}

void EditorHelpButton::_update_icons() {
	set_button_icon(get_editor_theme_icon(SNAME("gui_help")));

	PopupMenu *help_popup = get_popup();
	help_popup->set_item_icon(help_popup->get_item_index(DOCUMENTATION), get_editor_theme_icon(SNAME("action_help_search")));
	help_popup->set_item_icon(help_popup->get_item_index(COMMAND_PALETTE), get_editor_theme_icon(SNAME("gui_command")));
	help_popup->set_item_icon(help_popup->get_item_index(SYSTEM_INFORMATION), get_editor_theme_icon(SNAME("status_info")));
	help_popup->set_item_icon(help_popup->get_item_index(ABOUT), get_editor_theme_icon(SNAME("branding_logo_solid")));
}
