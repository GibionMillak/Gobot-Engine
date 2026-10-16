/**************************************************************************/
/*  project_compatibility_dialog.cpp                                      */
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

#include "project_compatibility_dialog.h"

#include "core/config/project_settings.h"
#include "core/version.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_unit.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/label.h"

ProjectCompatibilityDialog::ProjectCompatibilityDialog() {
	set_ok_button_text(TTRC("Open Anyway"));
	set_title(TTRC("Project Compatibility"));

	_content_vbox = memnew(VBoxContainer);
	_content_vbox->set_custom_minimum_size(Size2(28 * EDUNIT * EDSCALE, 0));
	_content_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(_content_vbox);

	{
		_heading_lbl = memnew(Label);
		_heading_lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		_content_vbox->add_child(_heading_lbl);

		_summary_lbl = memnew(Label);
		_summary_lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		_content_vbox->add_child(_summary_lbl);

		_details_lbl = memnew(Label);
		_details_lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		_content_vbox->add_child(_details_lbl);

		Label *warning_lbl = memnew(Label);
		warning_lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		warning_lbl->set_text(TTRC("Opening this project may cause data loss or unexpected behavior."));
		_content_vbox->add_child(warning_lbl);

		_backup_checkbox = memnew(CheckBox);
		_backup_checkbox->set_text(TTRC("Back Up Project"));
		_content_vbox->add_child(_backup_checkbox);

		Label *backup_description_lbl = memnew(Label);
		backup_description_lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		backup_description_lbl->set_text(TTRC("Create a copy of the project folder before opening it."));
		_content_vbox->add_child(backup_description_lbl);
	}
}

void ProjectCompatibilityDialog::show_for_project(Reason p_reason, const String &p_editor_version, int p_config_version, const String &p_feature_warning) {
	const String version = p_editor_version.is_empty() ? TTR("Unknown") : p_editor_version;

	switch (p_reason) {
		case CONFIG_VERSION: {
			_heading_lbl->set_text(TTR("Unsupported Project Configuration"));
			_summary_lbl->set_text(TTR("The selected project uses a different or unspecified configuration format. This editor cannot convert the project."));
			_details_lbl->set_text(vformat(TTR("Project format: %s. Supported format: %d."), p_config_version == 0 ? TTR("Unknown") : itos(p_config_version), ProjectSettings::CONFIG_VERSION));
		} break;
		case EDITOR_VERSION: {
			_heading_lbl->set_text(TTR("Different Editor Version"));
			_summary_lbl->set_text(TTR("The selected project was last saved with a different or unknown editor version."));
			_details_lbl->set_text(vformat(TTR("Project editor version: %s. Current editor version: %s.  Project conversion is not supported."), version, GODOT_VERSION_NUMBER));
		} break;
		case UNSUPPORTED_FEATURES: {
			_heading_lbl->set_text(TTR("Unsupported Project Features"));
			_summary_lbl->set_text(TTR("The selected project requires capabilities unavailable in this editor build."));
			_details_lbl->set_text(p_feature_warning);
		} break;
	}

	const bool newer_format = p_config_version > ProjectSettings::CONFIG_VERSION;
	if (newer_format) {
		_details_lbl->set_text(vformat(TTR("Project format: %d. Supported format: %d. This editor cannot read the newer configuration format."), p_config_version, ProjectSettings::CONFIG_VERSION));
	}
	if (p_reason == CONFIG_VERSION && p_editor_version != GODOT_VERSION_NUMBER) {
		_heading_lbl->set_text(TTR("Project Configuration and Editor Version Mismatch"));
		_summary_lbl->set_text(TTR("The selected project uses a different or unspecified configuration format and was last saved with a different or unknown editor version. This editor cannot convert the project."));
		_details_lbl->set_text(_details_lbl->get_text() + "\n\n" +
				vformat(TTR("Project editor version: %s. Current editor version: %s. Opening a project with a different editor version may cause data loss or unexpected behavior."), version, GODOT_VERSION_NUMBER));
	}
	if (p_reason != UNSUPPORTED_FEATURES && !p_feature_warning.is_empty()) {
		_details_lbl->set_text(_details_lbl->get_text() + "\n\n" + p_feature_warning);
	}
	_backup_checkbox->set_pressed(false);
	_backup_checkbox->set_disabled(newer_format);
	get_ok_button()->set_disabled(newer_format);
	// Measure wrapped text at the content width, then discard cached container measurements.
	const real_t content_width = _content_vbox->get_custom_minimum_size().x;
	for (int i = 0; i < _content_vbox->get_child_count(); i++) {
		Label *child_lbl = Object::cast_to<Label>(_content_vbox->get_child(i));
		if (child_lbl) {
			child_lbl->set_size(Size2(content_width, 0));
			const Size2 minimum_size = child_lbl->get_minimum_size();
			child_lbl->set_size(Size2(content_width, minimum_size.y));
			child_lbl->update_minimum_size();
		}
	}
	_content_vbox->update_minimum_size();
	reset_size();
	popup_centered();
	get_cancel_button()->grab_focus();
}

bool ProjectCompatibilityDialog::is_backup_requested() const {
	return _backup_checkbox->is_pressed();
}

void ProjectCompatibilityDialog::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			_heading_lbl->add_theme_font_override("font", get_theme_font("bold", "EditorFonts"));
		} break;
	}
}
