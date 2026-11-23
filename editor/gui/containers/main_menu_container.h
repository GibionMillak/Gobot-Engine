/**************************************************************************/
/*  main_menu_container.h                                                 */
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

#include "scene/gui/box_container.h"

class MenuBar;
class MenuButton;
class PopupMenu;

class MainMenuContainer : public HBoxContainer {
	GDCLASS(MainMenuContainer, HBoxContainer);

public:
	explicit MainMenuContainer(bool p_collapsed = false);

	PopupMenu *get_file_menu() const { return _file_menu; }
	PopupMenu *get_project_menu() const { return _project_menu; }
	PopupMenu *get_editor_menu() const { return _editor_menu; }
	PopupMenu *get_tools_menu() const { return _tools_menu; }

	void set_collapsed(bool p_collapsed);
	bool is_collapsed() const { return _collapsed; }

protected:
	void _notification(int p_what);

private:
	bool _collapsed = false;
	MenuBar *_main_menu_bar = nullptr;
	MenuButton *_main_menu_btn = nullptr;
	PopupMenu *_file_menu = nullptr;
	PopupMenu *_project_menu = nullptr;
	PopupMenu *_editor_menu = nullptr;
	PopupMenu *_tools_menu = nullptr;
};
