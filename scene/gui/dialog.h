/**************************************************************************/
/*  dialog.h                                                              */
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
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/panel.h"
#include "scene/gui/popup.h"
#include "scene/main/window.h"

class LineEdit;

class Dialog : public Window {
	GDCLASS(Dialog, Window);

public:
	explicit Dialog(bool p_show_cancel = false);
	~Dialog() override = default;

	Label *get_label() { return _message_lbl; }
	void register_text_enter(LineEdit *p_line_edit);
	Button *get_ok_button() { return _ok_btn; }
	Button *get_cancel_button() { return _cancel_btn; }
	Button *add_button(const String &p_text, bool p_right = false, const String &p_action = "");
	Button *add_cancel_button(const String &p_cancel = "");
	void remove_button(Button *p_button);
	void set_cancel_button_visible(bool p_visible);
	bool is_cancel_button_visible() const;
	void set_cancel_button_text(String p_cancel_button_text);
	String get_cancel_button_text() const;
	void set_hide_on_ok(bool p_hide);
	bool get_hide_on_ok() const;
	void set_close_on_escape(bool p_enable);
	bool get_close_on_escape() const;
	void set_text(String p_text);
	String get_text() const;
	void set_autowrap(bool p_autowrap);
	bool has_autowrap();
	void set_ok_button_text(String p_ok_button_text);
	String get_ok_button_text() const;

protected:
	static void _bind_methods();
	void _notification(int p_what);
	Size2 _get_contents_minimum_size() const override;
	void _input_from_window(const Ref<InputEvent> &p_event) override;
	void _post_popup() override;
	void _validate_property(PropertyInfo &r_property) const;
	virtual void _on_confirmed() {}
	virtual void _on_canceled() {}
	virtual void _on_custom_action(const String &p_action) {}
	void _set_default_ok_text(const String &p_text);
	void _text_submitted(const String &p_text);
	void _ok_pressed();
	void _cancel_pressed();

private:
	struct ThemeCache {
		Ref<StyleBox> panel_style;
		int buttons_separation = 0;
		int buttons_min_width = 0;
		int buttons_min_height = 0;
	};

	void _custom_action(const String &p_action);
	void _button_visibility_changed(Button *p_button);
	void _update_child_rects();
	void _update_ok_text();
	void _parent_focused();
	void _update_confirmation_style();

	Window *_parent_visible = nullptr;
	Panel *_bg_panel = nullptr;
	Label *_message_lbl = nullptr;
	HBoxContainer *_buttons_hbox = nullptr;
	Button *_ok_btn = nullptr;
	Button *_cancel_btn = nullptr;
	bool _popped_up = false;
	String _ok_text;
	String _default_ok_text;
	bool _hide_on_ok = true;
	bool _close_on_escape = true;
	// Required by the BIND_THEME_ITEM macros.
	ThemeCache theme_cache;
};
