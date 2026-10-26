/**************************************************************************/
/*  document_list.h                                                       */
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

#include "core/templates/local_vector.h"
#include "scene/gui/box_container.h"
#include "scene/resources/texture.h"

class DocumentEditorContainer;
class ItemList;
class LineEdit;
class PopupMenu;

class DocumentList : public VBoxContainer {
	GDCLASS(DocumentList, VBoxContainer);

public:
	explicit DocumentList(DocumentEditorContainer *p_document_editor_container);
	void update_list();
	void ensure_current_is_visible();
	void goto_next_document(bool p_previous = false);
	void update_editor_settings();

protected:
	void _notification(int p_what);

private:
	struct ItemData {
		String display_name;
		String tooltip;
		Ref<Texture2D> icon;
		int index = 0;
		bool hidden = false;
		bool tool = false;
		bool used_in_scene = false;
		bool has_error = false;
	};
	void _document_selected(int p_index);
	void _document_clicked(int p_index, Vector2 p_local_mouse_pos, MouseButton p_mouse_button_index);
	void _show_context_menu();
	void _update_item_list();
	Variant _get_drag_data_fw(const Point2 &p_point, Control *p_from);
	bool _can_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from) const;
	void _drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from);
	LocalVector<ItemData> _items;
	ItemList *_item_list = nullptr;
	LineEdit *_filter = nullptr;
	PopupMenu *_context_menu = nullptr;
	DocumentEditorContainer *_document_editor_container = nullptr;
	bool _script_temperature_enabled = false;
	int _temperature_history_size = 0;
};
