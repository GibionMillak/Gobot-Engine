/**************************************************************************/
/*  main_menu_container.cpp                                               */
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

#include "main_menu_container.h"

#include "core/string/string_name.h"
#include "core/string/ustring.h"
#include "scene/gui/menu_bar.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/popup_menu.h"

MainMenuContainer::MainMenuContainer(bool p_collapsed) {
	_file_menu = memnew(PopupMenu);
	_file_menu->set_name(TTRC("File"));

	_project_menu = memnew(PopupMenu);
	_project_menu->set_name(TTRC("Project"));

	_editor_menu = memnew(PopupMenu);
	_editor_menu->set_name(TTRC("Editor"));

	_tools_menu = memnew(PopupMenu);
	_tools_menu->set_name(TTRC("Tools"));

	set_collapsed(p_collapsed);
}

void MainMenuContainer::set_collapsed(bool p_collapsed) {
	if (p_collapsed ? _main_menu_btn != nullptr : _main_menu_bar != nullptr) {
		return;
	}

	PopupMenu *const menus[] = { _file_menu, _project_menu, _editor_menu, _tools_menu };
	if (_main_menu_btn) {
		PopupMenu *compact_menu = _main_menu_btn->get_popup();
		compact_menu->hide();
		compact_menu->clear(false);
		for (PopupMenu *menu : menus) {
			compact_menu->remove_child(menu);
		}
		memdelete(_main_menu_btn);
		_main_menu_btn = nullptr;
	} else if (_main_menu_bar) {
		for (PopupMenu *menu : menus) {
			menu->hide();
			_main_menu_bar->remove_child(menu);
		}
		memdelete(_main_menu_bar);
		_main_menu_bar = nullptr;
	}

	_collapsed = p_collapsed;

	if (p_collapsed) {
		_main_menu_btn = memnew(MenuButton);
		if (is_inside_tree()) {
			_main_menu_btn->set_button_icon(get_editor_theme_icon(SNAME("gui_triple_bar")));
		}
		_main_menu_btn->set_switch_on_hover(true);
		_main_menu_btn->set_theme_type_variation("MainMenuButton");
		_main_menu_btn->set_tooltip_text(TTRC("Main Menu"));
		add_child(_main_menu_btn);

		{
			PopupMenu *compact_menu = _main_menu_btn->get_popup();
			for (PopupMenu *menu : menus) {
				compact_menu->add_submenu_node_item(menu->get_name(), menu);
			}
		}
	} else {
		_main_menu_bar = memnew(MenuBar);
		_main_menu_bar->set_mouse_filter(MOUSE_FILTER_STOP);
		_main_menu_bar->set_prefer_global_menu(false);
		_main_menu_bar->set_switch_on_hover(true);
		_main_menu_bar->set_v_size_flags(SIZE_SHRINK_CENTER);
		add_child(_main_menu_bar);

		{
			for (PopupMenu *menu : menus) {
				_main_menu_bar->add_child(menu);
			}
		}
	}
}

void MainMenuContainer::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			if (_main_menu_btn) {
				_main_menu_btn->set_button_icon(get_editor_theme_icon(SNAME("gui_triple_bar")));
			}
		} break;
	}
}
