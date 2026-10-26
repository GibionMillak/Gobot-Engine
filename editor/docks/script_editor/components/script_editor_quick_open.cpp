/**************************************************************************/
/*  script_editor_quick_open.cpp                                          */
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

#include "script_editor_quick_open.h"

#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "editor/gui/filter_line_edit.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/tree.h"

ScriptEditorQuickOpen::ScriptEditorQuickOpen() :
		Dialog(true) {
	set_ok_button_text(TTRC("Open"));
	get_ok_button()->set_disabled(true);
	set_hide_on_ok(false);

	VBoxContainer *search_vbox = memnew(VBoxContainer);
	add_child(search_vbox);

	{
		_search_box = memnew(FilterLineEdit);
		search_vbox->add_margin_child(TTRC("Search:"), _search_box);
		_search_box->connect(SceneStringName(text_changed), callable_mp(this, &ScriptEditorQuickOpen::_update_search).unbind(1));
		register_text_enter(_search_box);

		_search_options = memnew(Tree);
		_search_options->add_theme_constant_override("draw_guides", 1);
		_search_options->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
		_search_options->set_hide_folding(true);
		_search_options->set_hide_root(true);
		search_vbox->add_margin_child(TTRC("Matches:"), _search_options, true);
		_search_options->connect("item_activated", callable_mp(this, &ScriptEditorQuickOpen::_confirmed));
		_search_box->set_forward_control(_search_options);
	}
}

void ScriptEditorQuickOpen::popup_dialog(const Vector<String> &p_functions, bool p_dontclear) {
	popup_centered_ratio(0.6);
	if (p_dontclear) {
		_search_box->select_all();
	} else {
		_search_box->clear();
	}
	_search_box->grab_focus();
	_functions = p_functions;
	_update_search();
}

void ScriptEditorQuickOpen::_bind_methods() {
	ADD_SIGNAL(MethodInfo("goto_line", PropertyInfo(Variant::INT, "line")));
}

void ScriptEditorQuickOpen::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			connect(SceneStringName(confirmed), callable_mp(this, &ScriptEditorQuickOpen::_confirmed));
		} break;

		case NOTIFICATION_EXIT_TREE: {
			disconnect(SceneStringName(confirmed), callable_mp(this, &ScriptEditorQuickOpen::_confirmed));
		} break;
	}
}

void ScriptEditorQuickOpen::_update_search() {
	_search_options->clear();
	TreeItem *root = _search_options->create_item();

	for (int i = 0; i < _functions.size(); i++) {
		String file = _functions[i];
		if ((_search_box->get_text().is_empty() || file.containsn(_search_box->get_text()))) {
			TreeItem *ti = _search_options->create_item(root);
			ti->set_text(0, file);
			if (root->get_first_child() == ti) {
				ti->select(0);
			}
		}
	}

	get_ok_button()->set_disabled(root->get_first_child() == nullptr);
}

void ScriptEditorQuickOpen::_confirmed() {
	TreeItem *ti = _search_options->get_selected();
	if (!ti) {
		return;
	}
	int line = ti->get_text(0).get_slicec(':', 1).to_int();

	emit_signal(SNAME("goto_line"), line - 1);
	hide();
}
