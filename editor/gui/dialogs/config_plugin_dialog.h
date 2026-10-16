/**************************************************************************/
/*  config_plugin_dialog.h                                                */
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

#include "scene/gui/dialogs.h"

class EditorValidationPanel;
class Label;
class LineEdit;
class OptionButton;
class ScriptLanguage;
class TextEdit;

class ConfigPluginDialog : public ConfirmationDialog {
	GDCLASS(ConfigPluginDialog, ConfirmationDialog);

public:
	ConfigPluginDialog();

	void popup_create();
	void popup_edit(const String &p_config_path);

protected:
	static void _bind_methods();
	void _notification(int p_what);

private:
	enum MessageId {
		PLUGIN_NAME,
		SUBFOLDER,
		SCRIPT,
		ENABLE_WARNINGS,
	};

	void _clear_fields();
	void _update_mode();
	void _on_confirmed();
	void _validate();
	String _get_subfolder() const;
	Error _save_plugin();
	Error _create_plugin_script(const String &p_script_path, ScriptLanguage *p_language);

	LineEdit *_name_edit = nullptr;
	Label *_subfolder_lbl = nullptr;
	LineEdit *_subfolder_edit = nullptr;
	TextEdit *_description_edit = nullptr;
	LineEdit *_author_edit = nullptr;
	LineEdit *_version_edit = nullptr;
	OptionButton *_language_option_btn = nullptr;
	LineEdit *_script_edit = nullptr;
	EditorValidationPanel *_validation_panel = nullptr;
	String _config_path;
	int _default_language = -1;
};
