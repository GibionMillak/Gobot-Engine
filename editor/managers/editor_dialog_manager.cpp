/**************************************************************************/
/*  editor_dialog_manager.cpp                                             */
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

#include "editor_dialog_manager.h"

#include "core/error/error_macros.h"
#include "editor/gui/dialogs/editor_about_dialog.h"
#include "editor/gui/dialogs/system_information_dialog.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_unit.h"

EditorDialogManager::EditorDialogManager() {
	ERR_FAIL_COND_MSG(_singleton != nullptr, "EditorDialogManager already exists.");
	_singleton = this;
}

EditorDialogManager::~EditorDialogManager() {
	if (_singleton == this) {
		_singleton = nullptr;
	}
}

EditorDialogManager *EditorDialogManager::get_singleton() {
	return _singleton;
}

void EditorDialogManager::show_about() {
	if (!_about_dialog) {
		_about_dialog = memnew(EditorAboutDialog);
		add_child(_about_dialog);
	}
	_about_dialog->popup_centered_clamped(Size2(24, 16) * EDUNIT * EDSCALE);
}

void EditorDialogManager::show_system_information() {
	if (!_system_information_dialog) {
		_system_information_dialog = memnew(SystemInformationDialog);
		add_child(_system_information_dialog);
	}
	_system_information_dialog->popup_centered_clamped(Size2(24, 19) * EDUNIT * EDSCALE);
}
