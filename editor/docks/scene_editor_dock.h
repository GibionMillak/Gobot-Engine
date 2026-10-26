/**************************************************************************/
/*  scene_editor_dock.h                                                 */
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

#include "editor/docks/editor_dock.h"

class Button;
class CanvasItemEditor;
class EditorSceneTabs;
class MarginContainer;
class Node3DEditor;

class SceneEditorDock : public EditorDock {
	GDCLASS(SceneEditorDock, EditorDock);

public:
	SceneEditorDock();

	EditorSceneTabs *get_scene_tabs() const { return _scene_tabs; }

	// The dock takes ownership of both viewport controls.
	void set_2d_editor(CanvasItemEditor *p_editor);
	void set_3d_editor(Node3DEditor *p_editor);
	void set_3d_mode(bool p_enabled);
	void set_3d_enabled(bool p_enabled);

	void save_layout_to_config(Ref<ConfigFile> &r_layout, const String &p_section) const override;
	void load_layout_from_config(const Ref<ConfigFile> &p_layout, const String &p_section) override;

protected:
	void _notification(int p_what);

private:
	void _update_editor_visibility();

	EditorSceneTabs *_scene_tabs = nullptr;
	Button *_viewport_mode_btn = nullptr;
	MarginContainer *_viewport_mc = nullptr;
	CanvasItemEditor *_editor_2d = nullptr;
	Node3DEditor *_editor_3d = nullptr;
	bool _3d_mode = true;
	bool _3d_enabled = true;
};
