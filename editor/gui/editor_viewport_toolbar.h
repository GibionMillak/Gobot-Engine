/**************************************************************************/
/*  editor_viewport_toolbar.h                                             */
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

#include "core/object/object_id.h"
#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "core/variant/callable.h"
#include "scene/gui/control.h"

class EditorToolbarFlowContainer;
class Popup;
class ScrollContainer;

class EditorViewportToolbar : public Control {
	GDCLASS(EditorViewportToolbar, Control);

public:
	EditorViewportToolbar();

	EditorToolbarFlowContainer *create_group();
	// Space from the left and right viewport edges to the overlay controls, in pixels.
	Vector2 get_overlay_insets() const;
	// Takes ownership of the contextual controls and displays them on the opposite edge.
	void set_context_control(EditorToolbarFlowContainer *p_control);
	void set_context_visible(bool p_visible);
	// Opens a toolbar popup toward the inside of the viewport.
	void popup_tool(Popup *p_popup, Control *p_anchor);

protected:
	static void _bind_methods();
	void _notification(int p_what);

private:
	void _queue_layout();
	void _controls_changed();
	void _update_layout();
	Rect2 _get_toolbar_rect(const Size2 &p_content_size, bool p_right_side, real_t p_width_limit) const;
	void _configure_flow(Control *p_control, int p_height, bool p_right_side);
	void _refresh_controls();
	void _configure_buttons(Control *p_control, HashSet<ObjectID> &r_controls, HashMap<ObjectID, Callable> &r_popup_connections);
	void _position_popup(Popup *p_popup, Control *p_anchor);

	ScrollContainer *_toolbar_scroll = nullptr;
	EditorToolbarFlowContainer *_content_vflow = nullptr;
	ScrollContainer *_context_scroll = nullptr;
	EditorToolbarFlowContainer *_context_control = nullptr;
	HashSet<ObjectID> _observed_controls;
	HashMap<ObjectID, Callable> _popup_connections;
	bool _layout_pending = false;
	bool _controls_dirty = true;
	bool _context_visible = false;
	Vector2 _overlay_insets;
};
