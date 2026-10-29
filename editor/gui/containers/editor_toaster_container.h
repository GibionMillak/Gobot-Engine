/**************************************************************************/
/*  editor_toaster_container.h                                            */
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

#include "core/error/error_macros.h"
#include "core/templates/hash_map.h"
#include "scene/gui/box_container.h"

class Button;
class Label;
class PanelContainer;
class StyleBox;

class EditorToasterContainer : public HBoxContainer {
	GDCLASS(EditorToasterContainer, HBoxContainer);

public:
	enum Severity {
		INFO = 0,
		WARNING,
		ERROR,
	};

	EditorToasterContainer();
	~EditorToasterContainer() override;

	static EditorToasterContainer *get_singleton();

	// Takes engine ownership of the supplied control by parenting it into the toast.
	Control *popup(Control *p_control, Severity p_severity = INFO, double p_time = 0.0, const String &p_tooltip = String());
	void popup_str(const String &p_message, Severity p_severity = INFO, const String &p_tooltip = String());
	void close(Control *p_control);
	void instant_close(Control *p_control);
	void copy(Control *p_control);

protected:
	static void _bind_methods();
	void _notification(int p_what);

private:
	static constexpr int STYLEBOX_RADIUS = 3;
	static constexpr int MAX_TEMPORARY_COUNT = 5;
	static constexpr double DEFAULT_MESSAGE_DURATION = 5.0;

	struct Toast {
		Severity severity = INFO;
		real_t duration = -1.0;
		real_t remaining_time = 0.0;
		bool popped = false;
		Button *copy_btn = nullptr;
		Button *close_btn = nullptr;
		String message;
		String tooltip;
		int count = 0;
		Label *message_lbl = nullptr;
		Label *message_count_lbl = nullptr;
	};

	static void _error_handler(void *p_self, const char *p_func, const char *p_file, int p_line, const char *p_error, const char *p_errorexp, bool p_editor_notify, ErrorHandlerType p_type);
	static void _error_handler_impl(const String &p_file, int p_line, const String &p_error, const String &p_errorexp, bool p_editor_notify, int p_type);
	void _update_vbox_position();
	void _update_disable_notifications_button();
	void _auto_hide_or_free_toasts();
	void _draw_button();
	void _draw_progress(Control *p_panel);
	void _set_notifications_enabled(bool p_enabled);
	void _repop_old();
	void _popup_str(const String &p_message, Severity p_severity, const String &p_tooltip);
	void _toast_theme_changed(Control *p_control);

	inline static EditorToasterContainer *_singleton = nullptr;
	ErrorHandlerList _error_handler_list;
	Ref<StyleBox> _info_panel_style_background;
	Ref<StyleBox> _warning_panel_style_background;
	Ref<StyleBox> _error_panel_style_background;
	Ref<StyleBox> _info_panel_style_progress;
	Ref<StyleBox> _warning_panel_style_progress;
	Ref<StyleBox> _error_panel_style_progress;
	Button *_main_btn = nullptr;
	PanelContainer *_disable_notifications_pc = nullptr;
	Button *_disable_notifications_btn = nullptr;
	VBoxContainer *_toasts_vbox = nullptr;
	HashMap<Control *, Toast> _toasts;
	bool _is_processing_error = false;
};

VARIANT_ENUM_CAST(EditorToasterContainer::Severity);
