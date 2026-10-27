/**************************************************************************/
/*  editor_zoom_container.h                                               */
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

class Button;

class EditorZoomContainer : public HBoxContainer {
	GDCLASS(EditorZoomContainer, HBoxContainer);

public:
	EditorZoomContainer();

	void set_zoom(float p_zoom);
	float get_zoom() const;
	void set_zoom_by_increments(int p_increment_count, bool p_integer_only = false);

	float get_min_zoom() const;
	float get_max_zoom() const;
	// Sets inclusive limits satisfying 0 <= min <= max and clamps the current zoom.
	void setup_zoom_limits(float p_min, float p_max);
	// Sets the shortcut context for the zoom buttons. By default their context is this EditorZoomContainer control.
	void set_shortcut_context(const Node *p_node) const;

protected:
	static void _bind_methods();
	void _notification(int p_what);

private:
	void _update_zoom_label();
	void _button_zoom_by(int p_direction);
	void _button_zoom_minus();
	void _button_zoom_reset();
	void _button_zoom_plus();

	Button *_zoom_minus_btn = nullptr;
	Button *_zoom_reset_btn = nullptr;
	Button *_zoom_plus_btn = nullptr;

	float _zoom = 1.0;
	float _min_zoom = 1.0 / 128;
	float _max_zoom = 128.0;
};
