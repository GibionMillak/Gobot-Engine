/**************************************************************************/
/*  editor_title_bar.h                                                    */
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

class EditorTitleBar : public HBoxContainer {
	GDCLASS(EditorTitleBar, HBoxContainer);

public:
	void set_center_control(Control *p_center_control);
	Control *get_center_control() const;

	void set_can_move_window(bool p_enabled);
	bool get_can_move_window() const;

protected:
	static void _bind_methods() {}

	void _notification(int p_what);

	virtual void gui_input(const Ref<InputEvent> &p_event) override;

private:
	void _on_minimize_pressed();
	void _on_maximize_pressed();
	void _on_close_pressed();
	void _update_window_buttons();
	void _create_window_buttons();
	void _destroy_window_buttons();

	Point2i _click_pos;
	bool _moving = false;
	bool _can_move = false;
	Control *_center_control = nullptr;

	HBoxContainer *_window_buttons_hb = nullptr;
	Button *_btn_minimize = nullptr;
	Button *_btn_maximize = nullptr;
	Button *_btn_close = nullptr;
};
