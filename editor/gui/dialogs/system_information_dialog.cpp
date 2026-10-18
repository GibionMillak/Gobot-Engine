/**************************************************************************/
/*  system_information_dialog.cpp                                        */
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

#include "system_information_dialog.h"

#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "core/version.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/table.h"
#include "scene/main/viewport.h"
#include "servers/audio/audio_server.h"
#include "servers/audio/audio_server_enums.h"
#include "servers/display/display_server.h"
#include "servers/rendering/rendering_server.h"

SystemInformationDialog::SystemInformationDialog() {
	set_ok_button_text(TTRC("Close"));
	set_flag(Window::FLAG_RESIZE_DISABLED, false);
	set_title(TTRC("System Information"));
	_copy_btn = add_button(TTRC("Copy To Clipboard"));
	_copy_btn->connect("pressed", callable_mp(this, &SystemInformationDialog::_copy_information));

	_scroll = memnew(ScrollContainer);
	_scroll->set_custom_minimum_size(Size2(640, 400) * EDSCALE);
	_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	add_child(_scroll);

	{
		_content_vbox = memnew(VBoxContainer);
		_content_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		_scroll->add_child(_content_vbox);
	}
}

void SystemInformationDialog::show_information() {
	while (_content_vbox->get_child_count() > 0) {
		Node *child = _content_vbox->get_child(0);
		_content_vbox->remove_child(child);
		memdelete(child);
	}
	_table = nullptr;
	_report = String();

	OS *os = OS::get_singleton();
	DisplayServer *display = DisplayServer::get_singleton();
	RenderingServer *rendering = RenderingServer::get_singleton();
	AudioServer *audio = AudioServer::get_singleton();

	_add_section(TTR("Application"));
	_add_row(TTR("Version"), GODOT_VERSION_FULL_CONFIG);
	_add_row(TTR("Build"), GODOT_VERSION_BUILD);
	_add_row(TTR("Commit"), GODOT_VERSION_HASH);

	_add_section(TTR("Operating System"));
	_add_row(TTR("Platform"), os->get_name());
	_add_row(TTR("Distribution"), os->get_distribution_name());
	_add_row(TTR("Version"), os->get_version_alias());
	_add_row(TTR("Locale"), os->get_locale());

	_add_section(TTR("Display"));
	_add_row(TTR("Display Driver"), display->get_name());
	_add_row(TTR("Window Mode"), get_parent()->get_viewport()->is_embedding_subwindows() ? TTR("Single-window") : TTR("Multi-window"));
	const Size2i window_size = display->window_get_size();
	_add_row(TTR("Window Size"), vformat(TTR("%d x %d pixels"), window_size.x, window_size.y));
	for (int i = 0; i < display->get_screen_count(); i++) {
		const Size2i screen_size = display->screen_get_size(i);
		String screen_info = vformat(TTR("%d x %d pixels"), screen_size.x, screen_size.y);
		const float refresh_rate = display->screen_get_refresh_rate(i);
		if (refresh_rate > 0) {
			screen_info += vformat(" @ %.2f Hz", refresh_rate);
		}
		_add_row(vformat(TTR("Screen %d"), i + 1), screen_info);
	}

	_add_section(TTR("Graphics"));
	_add_row(TTR("Rendering Driver"), os->get_current_rendering_driver_name());
	_add_row(TTR("Rendering Method"), os->get_current_rendering_method());
	_add_row(TTR("GPU"), rendering->get_video_adapter_name());
	_add_row(TTR("Vendor"), rendering->get_video_adapter_vendor());
	_add_row(TTR("API Version"), rendering->get_video_adapter_api_version());
	const Vector<String> driver_info = os->get_video_adapter_driver_info();
	if (!driver_info.is_empty()) {
		_add_row(TTR("GPU Driver"), String(" ").join(driver_info));
	}

	_add_section(TTR("Hardware"));
	_add_row(TTR("CPU"), os->get_processor_name());
	_add_row(TTR("CPU Threads"), itos(os->get_processor_count()));
	const Dictionary memory = os->get_memory_info();
	const int64_t physical_memory = memory.get("physical", -1);
	if (physical_memory > 0) {
		_add_row(TTR("Physical Memory"), String::humanize_size(physical_memory));
	}
	const int64_t available_memory = memory.get("available", -1);
	if (available_memory > 0) {
		_add_row(TTR("Available Memory"), String::humanize_size(available_memory));
	}

	_add_section(TTR("Audio"));
	_add_row(TTR("Audio Driver"), audio->get_driver_name());
	_add_row(TTR("Mix Rate"), vformat("%d Hz", int(audio->get_mix_rate())));
	String speaker_mode;
	switch (audio->get_speaker_mode()) {
		case AuSE::SpeakerMode::SPEAKER_MODE_STEREO:
			speaker_mode = TTR("Stereo/mono");
			break;
		case AuSE::SpeakerMode::SPEAKER_SURROUND_31:
			speaker_mode = TTR("Surround 3.1");
			break;
		case AuSE::SpeakerMode::SPEAKER_SURROUND_51:
			speaker_mode = TTR("Surround 5.1");
			break;
		case AuSE::SpeakerMode::SPEAKER_SURROUND_71:
			speaker_mode = TTR("Surround 7.1");
			break;
	}
	_add_row(TTR("Speaker Mode"), speaker_mode);

	_scroll->set_v_scroll(0);
	popup_centered_clamped(Size2i(760, 600) * EDSCALE);
	get_ok_button()->grab_focus();
}

void SystemInformationDialog::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			_copy_btn->set_button_icon(get_editor_theme_icon(SNAME("Copy")));
			for (int i = 0; i < _content_vbox->get_child_count(); i++) {
				Table *table = Object::cast_to<Table>(_content_vbox->get_child(i));
				if (table) {
					for (int j = 0; j < table->get_footer()->get_child_count(); j++) {
						Button *copy_btn = Object::cast_to<Button>(table->get_footer()->get_child(j));
						if (copy_btn) {
							copy_btn->set_button_icon(get_editor_theme_icon(SNAME("Copy")));
						}
					}
				}
			}
		} break;
	}
}

void SystemInformationDialog::_add_section(const String &p_title) {
	_table = memnew(Table);
	_table->set_column_count(2);
	_table->set_column_width(0, 200 * EDSCALE);
	_table->set_column_width(1, 240 * EDSCALE);
	_table->set_column_width_mode(0, Table::FIXED);
	_table->set_column_width_mode(1, Table::EXPAND);
	_table->set_fit_content_height(true);
	_table->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	_table->set_row_count(1);
	_table->set_show_row_count(false);
	_content_vbox->add_child(_table);

	{
		_table->set_cell(0, 0, p_title);
		_table->set_cell_role(0, 0, Table::HEADER);
		ERR_FAIL_COND(!_table->set_cell_span(0, 0, 2, 1));

		Button *copy_btn = memnew(Button);
		copy_btn->connect("pressed", callable_mp(this, &SystemInformationDialog::_copy_section).bind(_table));
		copy_btn->set_button_icon(get_editor_theme_icon(SNAME("Copy")));
		copy_btn->set_text(TTRC("Copy"));
		_table->get_footer()->add_child(copy_btn);
	}

	if (!_report.is_empty()) {
		_report += "\n";
	}
	_report += p_title + "\n";
}

void SystemInformationDialog::_add_row(const String &p_name, const String &p_value) {
	const String value = p_value.is_empty() ? TTR("Unknown") : p_value;
	const int row = _table->add_row();
	_table->set_cell_text(row, 0, p_name);
	_table->set_cell_text(row, 1, value);
	_report += p_name + ": " + value + "\n";
}

void SystemInformationDialog::_copy_information() {
	DisplayServer::get_singleton()->clipboard_set(_report);
}

void SystemInformationDialog::_copy_section(Table *p_table) {
	String report = p_table->get_cell_text(0, 0) + "\n";
	for (int row = 1; row < p_table->get_row_count(); row++) {
		report += p_table->get_cell_text(row, 0) + ": " + p_table->get_cell_text(row, 1) + "\n";
	}
	DisplayServer::get_singleton()->clipboard_set(report);
}
