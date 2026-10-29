/**************************************************************************/
/*  editor_footer_container.cpp                                           */
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

#include "editor_footer_container.h"

#include "editor/gui/buttons/editor_redraw_button.h"
#include "editor/gui/buttons/editor_version_button.h"
#include "editor/gui/buttons/project_renderer_button.h"
#include "editor/gui/containers/editor_toaster_container.h"
#include "editor/run/editor_run_bar.h"
#include "scene/gui/box_container.h"

EditorFooterContainer::EditorFooterContainer() {
	_left_hbox = memnew(HBoxContainer);
	_left_hbox->set_alignment(ALIGNMENT_BEGIN);
	_left_hbox->set_h_size_flags(SIZE_EXPAND_FILL);
	_left_hbox->set_v_size_flags(SIZE_EXPAND_FILL);
	add_child(_left_hbox);

	{
		EditorRedrawButton *editor_redraw_btn = memnew(EditorRedrawButton);
		_left_hbox->add_child(editor_redraw_btn);
	}

	HBoxContainer *center_hbox = memnew(HBoxContainer);
	center_hbox->set_alignment(ALIGNMENT_CENTER);
	center_hbox->set_h_size_flags(SIZE_EXPAND_FILL);
	center_hbox->set_v_size_flags(SIZE_EXPAND_FILL);
	add_child(center_hbox);

	{
		_run_bar = memnew(EditorRunBar);
		_run_bar->set_mouse_filter(MOUSE_FILTER_STOP);
		center_hbox->add_child(_run_bar);
	}

	_right_hbox = memnew(HBoxContainer);
	_right_hbox->set_alignment(ALIGNMENT_END);
	_right_hbox->set_h_size_flags(SIZE_EXPAND_FILL);
	_right_hbox->set_v_size_flags(SIZE_EXPAND_FILL);
	add_child(_right_hbox);

	{
		ProjectRendererButton *project_renderer_btn = memnew(ProjectRendererButton);
		_right_hbox->add_child(project_renderer_btn);

		EditorVersionButton *version_btn = memnew(EditorVersionButton(EditorVersionButton::BASIC));
		_right_hbox->add_child(version_btn);

		EditorToasterContainer *toaster = memnew(EditorToasterContainer);
		_right_hbox->add_child(toaster);
	}
}
