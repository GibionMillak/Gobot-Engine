/**************************************************************************/
/*  quick_settings_dialog.cpp                                             */
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

#include "quick_settings_dialog.h"

#include "core/object/callable_mp.h"
#include "editor/doc/editor_help.h"
#include "editor/editor_string_names.h"
#include "editor/gui/dialogs/editor_settings_dialog.h"
#include "editor/inspector/editor_properties.h"
#include "editor/inspector/editor_resource_picker.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme_manager.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel_container.h"
#include "servers/display/display_server.h"

void QuickSettingsDialog::_fetch_setting_values() {
	_editor_languages.clear();
	_editor_scales.clear();
	_editor_network_modes.clear();
	_editor_check_for_updates.clear();
	_editor_directory_naming_conventions.clear();

	{
		List<PropertyInfo> editor_settings_properties;
		EditorSettings::get_singleton()->get_property_list(&editor_settings_properties);

		for (const PropertyInfo &pi : editor_settings_properties) {
			if (pi.name == "interface/editor/localization/editor_language") {
				_editor_languages = pi.hint_string.split(";", false);
			} else if (pi.name == "interface/editor/appearance/display_scale") {
				_editor_scales = pi.hint_string.split(",");
			} else if (pi.name == "network/connection/network_mode") {
				_editor_network_modes = pi.hint_string.split(",");
			} else if (pi.name == "network/connection/check_for_updates") {
				_editor_check_for_updates = pi.hint_string.split(",");
			} else if (pi.name == "hub/directory_naming_convention") {
				_editor_directory_naming_conventions = pi.hint_string.split(",");
			}
		}
	}
}

void QuickSettingsDialog::_update_current_values() {
	// Language options.
	{
		const String current_lang = EDITOR_GET("interface/editor/localization/editor_language");

		for (int i = 0; i < _editor_languages.size(); i++) {
			const String &lang_value = _editor_languages[i].get_slicec('/', 0);
			if (current_lang == lang_value) {
				_language_option_btn->set_text(_editor_languages[i].get_slicec('/', 1));
				_language_option_btn->select(i);
				break;
			}
		}
	}

	// Theme options.
	{
		_theme_picker->set_edited_resource(EDITOR_GET("interface/theme/theme"));
	}

	// Scale options.
	{
		const int current_scale = EDITOR_GET("interface/editor/appearance/display_scale");

		for (int i = 0; i < _editor_scales.size(); i++) {
			const String &scale_value = _editor_scales[i];
			if (current_scale == i) {
				_scale_option_button->set_text(scale_value);
				_scale_option_button->select(i);
			}
		}
	}

	// Network mode options.
	{
		const int current_network_mode = EDITOR_GET("network/connection/network_mode");

		for (int i = 0; i < _editor_network_modes.size(); i++) {
			const String &network_mode_value = _editor_network_modes[i];
			if (current_network_mode == i) {
				_network_mode_option_button->set_text(network_mode_value);
				_network_mode_option_button->select(i);
			}
		}
	}

	// Check for updates options.
	{
		const int current_update_mode = EDITOR_GET("network/connection/check_for_updates");

		for (int i = 0; i < _editor_check_for_updates.size(); i++) {
			const String &check_for_update_value = _editor_check_for_updates[i];
			if (current_update_mode == i) {
				_check_for_update_button->set_text(check_for_update_value);
				_check_for_update_button->select(i);

				// Disables Check for Updates selection if Network mode is set to Offline.
				_check_for_update_button->set_disabled(!EDITOR_GET("network/connection/network_mode"));
			}
		}
	}

	// Project directory naming options.
	{
		const int current_directory_naming = EDITOR_GET("hub/directory_naming_convention");

		for (int i = 0; i < _editor_directory_naming_conventions.size(); i++) {
			const String &directory_naming_value = _editor_directory_naming_conventions[i];
			if (current_directory_naming == i) {
				_directory_naming_convention_button->set_text(directory_naming_value);
				_directory_naming_convention_button->select(i);
			}
		}
	}
}

void QuickSettingsDialog::_add_setting_control(const String &p_text, const String &p_setting, Control *p_control) {
	HBoxContainer *container = memnew(HBoxContainer);
	_settings_list->add_child(container);

	Label *label = memnew(SettingLabel(p_text, p_setting));
	label->set_text(p_text);
	label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	label->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	container->add_child(label);

	p_control->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	container->add_child(p_control);
}

void QuickSettingsDialog::_language_selected(int p_id) {
	const String selected_language = _language_option_btn->get_item_metadata(p_id);
	_set_setting_value("interface/editor/localization/editor_language", selected_language);
}

void QuickSettingsDialog::_theme_changed(const Ref<Resource> &p_resource) {
	EditorSettings::get_singleton()->set_manually("interface/theme/follow_system_theme", false);
	_set_setting_value("interface/theme/theme", p_resource);
}

void QuickSettingsDialog::_scale_selected(int p_id) {
	_set_setting_value("interface/editor/appearance/display_scale", p_id, true);
}

void QuickSettingsDialog::_network_mode_selected(int p_id) {
	_set_setting_value("network/connection/network_mode", p_id);

	// Disables Check for Updates selection if Network mode is set to Offline.
	_check_for_update_button->set_disabled(!p_id);
}

void QuickSettingsDialog::_check_for_update_selected(int p_id) {
	_set_setting_value("network/connection/check_for_updates", p_id);
}

void QuickSettingsDialog::_directory_naming_convention_selected(int p_id) {
	_set_setting_value("hub/directory_naming_convention", p_id);
}

void QuickSettingsDialog::_set_setting_value(const String &p_setting, const Variant &p_value, bool p_restart_required) {
	EditorSettings::get_singleton()->set(p_setting, p_value);
	EditorSettings::get_singleton()->notify_changes();
	EditorSettings::get_singleton()->save();

	if (p_restart_required) {
		_restart_required_label->show();

		if (!_restart_required_button) {
			_restart_required_button = add_button(TTRC("Restart Now"), true);
			_restart_required_button->connect(SceneStringName(pressed), callable_mp(this, &QuickSettingsDialog::_request_restart));
		}
	}
}

void QuickSettingsDialog::_show_full_settings() {
	if (!_editor_settings_dialog) {
		Ref<EditorInspectorDefaultPlugin> eidp;
		eidp.instantiate();
		EditorInspector::add_inspector_plugin(eidp);

		_editor_settings_dialog = memnew(EditorSettingsDialog);
		get_parent()->add_child(_editor_settings_dialog);
		_editor_settings_dialog->connect("restart_requested", callable_mp(this, &QuickSettingsDialog::_request_restart));
	}
	hide();
	_editor_settings_dialog->popup_edit_settings();
}

void QuickSettingsDialog::_request_restart() {
	emit_signal("restart_required");
}

void QuickSettingsDialog::update_size_limits(const Size2 &p_max_popup_size) {
	_language_option_btn->get_popup()->set_max_size(p_max_popup_size);
}

void QuickSettingsDialog::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			_settings_list_panel->add_theme_style_override(SceneStringName(panel), get_theme_stylebox(SNAME("quick_settings_panel"), SNAME("Hub")));

			_restart_required_label->add_theme_color_override(SceneStringName(font_color), get_theme_color(SNAME("warning_color"), EditorStringName(Editor)));
		} break;

		case NOTIFICATION_VISIBILITY_CHANGED: {
			if (is_visible()) {
				_update_current_values();
			}
		} break;
	}
}

void QuickSettingsDialog::_bind_methods() {
	ADD_SIGNAL(MethodInfo("restart_required"));
}

QuickSettingsDialog::QuickSettingsDialog() {
	set_title(TTRC("Quick Settings"));
	set_ok_button_text(TTRC("Close"));

	VBoxContainer *main_vbox = memnew(VBoxContainer);
	add_child(main_vbox);
	main_vbox->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);

	// Settings grid.
	{
		_fetch_setting_values();

		_settings_list_panel = memnew(PanelContainer);
		main_vbox->add_child(_settings_list_panel);

		_settings_list = memnew(VBoxContainer);
		_settings_list_panel->add_child(_settings_list);

		// Language options.
		{
			_language_option_btn = memnew(OptionButton);

			for (int i = 0; i < _editor_languages.size(); i++) {
				const String &lang_code = _editor_languages[i].get_slicec('/', 0);
				const String &lang_name = _editor_languages[i].get_slicec('/', 1);
				_language_option_btn->add_item(lang_name, i);
				_language_option_btn->set_item_metadata(i, lang_code);
			}

			_language_option_btn->connect(SceneStringName(item_selected), callable_mp(this, &QuickSettingsDialog::_language_selected));
			_language_option_btn->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
			_language_option_btn->set_fit_to_longest_item(false);
			_language_option_btn->set_search_bar_enabled(true);

			_add_setting_control(TTRC("Language"), "interface/editor/localization/editor_language", _language_option_btn);
		}

		// Theme options.
		{
			_theme_picker = memnew(EditorResourcePicker);
			_theme_picker->connect("resource_changed", callable_mp(this, &QuickSettingsDialog::_theme_changed));
			_theme_picker->set_base_type("Theme");
			_theme_picker->set_builtin_resources(EditorThemeManager::get_builtin_themes());
			_theme_picker->set_property_path("interface/theme/theme");
			_theme_picker->set_resource_inspection_enabled(false);
			_theme_picker->set_resource_owner(EditorSettings::get_singleton());
			_add_setting_control(TTRC("Theme"), "interface/theme/theme", _theme_picker);
		}

		// Scale options.
		{
			_scale_option_button = memnew(OptionButton);
			_scale_option_button->set_fit_to_longest_item(false);
			_scale_option_button->connect(SceneStringName(item_selected), callable_mp(this, &QuickSettingsDialog::_scale_selected));

			for (int i = 0; i < _editor_scales.size(); i++) {
				const String &scale_value = _editor_scales[i];
				_scale_option_button->add_item(scale_value, i);
			}

			_add_setting_control(TTRC("Display Scale"), "interface/editor/appearance/display_scale", _scale_option_button);
		}

		// Network mode options.
		{
			_network_mode_option_button = memnew(OptionButton);
			_network_mode_option_button->set_fit_to_longest_item(false);
			_network_mode_option_button->connect(SceneStringName(item_selected), callable_mp(this, &QuickSettingsDialog::_network_mode_selected));

			for (int i = 0; i < _editor_network_modes.size(); i++) {
				const String &network_mode_value = _editor_network_modes[i];
				_network_mode_option_button->add_item(network_mode_value, i);
			}

			_add_setting_control(TTRC("Network Mode"), "network/connection/network_mode", _network_mode_option_button);
		}

		// Check for updates options.
		{
			_check_for_update_button = memnew(OptionButton);
			_check_for_update_button->set_fit_to_longest_item(false);
			_check_for_update_button->connect(SceneStringName(item_selected), callable_mp(this, &QuickSettingsDialog::_check_for_update_selected));

			for (int i = 0; i < _editor_check_for_updates.size(); i++) {
				const String &check_for_update_value = _editor_check_for_updates[i];
				_check_for_update_button->add_item(check_for_update_value, i);
			}

			_add_setting_control(TTRC("Check for Updates"), "network/connection/check_for_updates", _check_for_update_button);
		}

		// Project directory naming options.
		{
			_directory_naming_convention_button = memnew(OptionButton);
			_directory_naming_convention_button->set_fit_to_longest_item(false);
			_directory_naming_convention_button->connect(SceneStringName(item_selected), callable_mp(this, &QuickSettingsDialog::_directory_naming_convention_selected));

			for (int i = 0; i < _editor_directory_naming_conventions.size(); i++) {
				const String &directory_naming_convention = _editor_directory_naming_conventions[i];
				_directory_naming_convention_button->add_item(directory_naming_convention, i);
			}

			_add_setting_control(TTRC("Directory Naming Convention"), "hub/directory_naming_convention", _directory_naming_convention_button);
		}

		_update_current_values();
	}

	// Full settings button.
	{
		Button *open_full_settings = memnew(Button);
		open_full_settings->set_text(TTRC("Edit All Settings"));
		open_full_settings->set_h_size_flags(Control::SIZE_SHRINK_END);
		_settings_list->add_child(open_full_settings);
		open_full_settings->connect(SceneStringName(pressed), callable_mp(this, &QuickSettingsDialog::_show_full_settings));
	}

	// Restart required panel.
	{
		_restart_required_label = memnew(Label(TTRC("Settings changed! The hub must be restarted for changes to take effect.")));
		_restart_required_label->set_custom_minimum_size(Size2(560, 0) * EDSCALE);
		_restart_required_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD);
		_restart_required_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		_restart_required_label->hide();
		main_vbox->add_child(_restart_required_label);
	}
}

Control *SettingLabel::make_custom_tooltip(const String &p_text) const {
	return EditorHelpBitTooltip::make_tooltip(const_cast<SettingLabel *>(this), vformat("property|EditorSettings|%s", _setting_name), String());
}

SettingLabel::SettingLabel(const String &p_text, const String &p_setting) :
		Label(p_text) {
	_setting_name = p_setting;
}
