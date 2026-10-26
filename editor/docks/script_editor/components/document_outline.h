/**************************************************************************/
/*  document_outline.h                                                    */
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

#include "core/math/color.h"
#include "core/templates/hash_map.h"
#include "scene/gui/box_container.h"

class Button;
class DocumentEditorContainer;
class FilterLineEdit;
class HBoxContainer;
class Tree;
class TreeItem;

class DocumentOutline : public VBoxContainer {
	GDCLASS(DocumentOutline, VBoxContainer);

public:
	explicit DocumentOutline(DocumentEditorContainer *p_document_editor_container);
	void update_editor_settings();
	void update_outline();
	void update_visibility();

protected:
	void _notification(int p_what);

private:
	void _add_function(const String &p_function_name, const String &p_selected, const Color &p_disabled_color, HashMap<String, TreeItem *> &r_parents);
	void _toggle_sort(bool p_alphabetic_sort);
	void _tree_selected();
	DocumentEditorContainer *_document_editor_container = nullptr;
	Tree *_tree = nullptr;
	HBoxContainer *_buttons_hbox = nullptr;
	FilterLineEdit *_filter = nullptr;
	Button *_sort_btn = nullptr;
	bool _members_overview_enabled = false;
	bool _help_overview_enabled = false;
	Control *_current_editor = nullptr;
	bool _updating_outline = false;
};
