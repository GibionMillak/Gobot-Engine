/**************************************************************************/
/*  project_autoloads_dialog.h                                            */
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

#include "scene/gui/dialogs.h"

class Button;
class Control;
class EditorValidationPanel;
class FileDialog;
class LineEdit;
class ProjectAutoloadManager;
class SceneCreateDialog;
class Script;
class ScriptCreateDialog;
class Tree;

class ProjectAutoloadsDialog : public AcceptDialog {
	GDCLASS(ProjectAutoloadsDialog, AcceptDialog);

public:
	ProjectAutoloadsDialog();

	void popup_autoloads();

protected:
	void _notification(int p_what);

private:
	enum ButtonId {
		MOVE_UP,
		MOVE_DOWN,
		DELETE,
	};

	static constexpr int PREVIEW_LIST_MAX_SIZE = 10;

	void _validate_autoload_name();

	void _refresh_tree();
	void _autoload_selected();
	void _autoload_edited();
	void _autoload_button_pressed(Object *p_item, int p_column, int p_button, MouseButton p_mouse_button);
	void _autoload_activated();
	void _autoload_open(const String &p_path);

	void _create_script_autoload();
	void _create_scene_autoload();
	void _autoload_file_selected(const String &p_path);
	void _scene_file_selected(const String &p_path);

	void _script_created(Ref<Script> p_script);
	void _scene_created();
	void _add_autoload(const String &p_name, const String &p_path);
	void _try_add_autoload(const String &p_name, const String &p_path);
	void _confirm_autoload_name();

	Variant _get_drag_data_fw(const Point2 &p_point, Control *p_control);
	bool _can_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_control) const;
	void _drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_control);

	// Owned by EditorManagers for the editor session.
	ProjectAutoloadManager *_manager = nullptr;
	String _pending_autoload_path;
	String _selected_autoload;
	bool _updating_tree = false;

	Tree *_tree = nullptr;
	Button *_browse_btn = nullptr;
	Button *_create_script_autoload_btn = nullptr;
	Button *_create_scene_autoload_btn = nullptr;
	FileDialog *_autoload_file_dialog = nullptr;
	FileDialog *_scene_file_dialog = nullptr;
	ScriptCreateDialog *_script_create_dialog = nullptr;
	SceneCreateDialog *_scene_create_dialog = nullptr;
	ConfirmationDialog *_name_dialog = nullptr;
	LineEdit *_name_edit = nullptr;
	EditorValidationPanel *_name_validator = nullptr;
};
