/**************************************************************************/
/*  undo_redo_container.cpp                                              */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                   */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to   */
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

#include "undo_redo_container.h"

#include "core/error/error_macros.h"
#include "core/input/input_event.h"
#include "core/input/shortcut.h"
#include "core/object/callable_mp.h"
#include "core/string/ustring.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/settings/editor_settings.h"
#include "scene/gui/button.h"
#include "scene/scene_string_names.h"

UndoRedoContainer::UndoRedoContainer() {
	EditorNode *editor_node = EditorNode::get_singleton();

	{
		_undo_btn = memnew(Button);
		_undo_btn->connect(SceneStringName(pressed), callable_mp(editor_node, &EditorNode::undo));
		_undo_btn->set_flat(true);
		_undo_btn->set_focus_mode(FOCUS_NONE);
		_undo_btn->set_shortcut(ED_GET_SHORTCUT("ui_undo"));
		_undo_btn->set_tooltip_text(TTRC("Undo"));
		add_child(_undo_btn);

		_redo_btn = memnew(Button);
		_redo_btn->connect(SceneStringName(pressed), callable_mp(editor_node, &EditorNode::redo));
		_redo_btn->set_flat(true);
		_redo_btn->set_focus_mode(FOCUS_NONE);
		_redo_btn->set_shortcut(ED_GET_SHORTCUT("ui_redo"));
		_redo_btn->set_tooltip_text(TTRC("Redo"));
		add_child(_redo_btn);
	}

	// Keep the shortcuts on the buttons for tooltips and handle key repeats here.
	_undo_btn->set_process_shortcut_input(false);
	_redo_btn->set_process_shortcut_input(false);
	set_process_shortcut_input(true);
}

void UndoRedoContainer::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
			undo_redo->connect(SNAME("history_changed"), callable_mp(this, &UndoRedoContainer::_update_buttons));
			undo_redo->connect(SNAME("version_changed"), callable_mp(this, &UndoRedoContainer::_update_buttons));
			EditorNode::get_singleton()->connect(SNAME("scene_changed"), callable_mp(this, &UndoRedoContainer::_update_buttons));
			_update_buttons();
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			_undo_btn->set_button_icon(get_editor_theme_icon(SNAME("action_undo")));
			_redo_btn->set_button_icon(get_editor_theme_icon(SNAME("action_redo")));
		} break;
	}
}

void UndoRedoContainer::shortcut_input(const Ref<InputEvent> &p_event) {
	ERR_FAIL_COND(p_event.is_null());
	if (!p_event->is_pressed() || !is_visible_in_tree()) {
		return;
	}

	Button *const buttons[] = { _undo_btn, _redo_btn };
	for (Button *button : buttons) {
		const Ref<Shortcut> shortcut = button->get_shortcut();
		if (!button->is_disabled() && shortcut.is_valid() && shortcut->matches_event(p_event)) {
			button->press();
			accept_event();
			return;
		}
	}
}

void UndoRedoContainer::_update_buttons() {
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	_undo_btn->set_disabled(!undo_redo->has_undo());
	_redo_btn->set_disabled(!undo_redo->has_redo());
}
