/**************************************************************************/
/*  export_template_manager.h                                             */
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

#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "scene/gui/dialog.h"

class Button;
class EditorExportPreset;
class EditorFileDialog;
class ItemList;
class Texture2D;
class Tree;
class TreeItem;

class ExportTemplateManager : public Dialog {
	GDCLASS(ExportTemplateManager, Dialog);

public:
	ExportTemplateManager();

	static String get_android_build_directory(const Ref<EditorExportPreset> &p_preset);
	static String get_android_source_zip(const Ref<EditorExportPreset> &p_preset);
	static String get_android_template_identifier(const Ref<EditorExportPreset> &p_preset);

	static bool is_android_template_installed(const Ref<EditorExportPreset> &p_preset);
	static bool is_android_build_version_valid(const Ref<EditorExportPreset> &p_preset);
	static bool can_install_android_template(const Ref<EditorExportPreset> &p_preset);
	static Error delete_android_build_directory(const Ref<EditorExportPreset> &p_preset);
	static Error install_android_template(const Ref<EditorExportPreset> &p_preset);
	static Error install_android_template_from_file(const String &p_file, const Ref<EditorExportPreset> &p_preset);

	void popup_manager();

protected:
	void _notification(int p_what);

private:
	enum class TemplateID {
		WINDOWS_X86_32,
		WINDOWS_X86_64,
		WINDOWS_ARM64,

		LINUX_X86_32,
		LINUX_X86_64,
		LINUX_ARM32,
		LINUX_ARM64,

		MACOS,

		ANDROID,

		IOS,

		ICU_DATA,
	};

	enum class PlatformID {
		WINDOWS,
		LINUX,
		MACOS,
		ANDROID,
		IOS,
		COMMON,
	};

	struct PlatformInfo {
		String name;
		Ref<Texture2D> icon;
		HashSet<TemplateID> templates;
	};

	struct TemplateInfo {
		String name;
		String description;
		PackedStringArray file_list;
	};

	struct ThemeCache {
		Ref<Texture2D> remove_icon;
		Color current_version_color;
		Color incomplete_template_color;
		Color missing_file_color;
		int icon_width = 0;
	};

	const StringName _PATH_META = "path";
	const StringName _FILE_META = "file";

	void _delete_all();
	void _delete_confirmed();
	void _delete_file(const TreeItem *p_item);
	void _tpz_file_selected(const String &p_file);
	void _initialize_template_data();
	void _update_version_list();
	void _update_template_tree();
	void _update_folding_cache(TreeItem *p_item);
	String _get_template_folder_path(const String &p_version) const;
	Ref<Texture2D> _get_platform_icon(const String &p_platform_name);
	void _version_selected();
	void _tree_button_clicked(TreeItem *p_item, int p_column, int p_id, MouseButton p_button);
	void _open_template_directory();
	void _apply_item_folding(TreeItem *p_item, bool p_default = false);
	void _setup_item_text(TreeItem *p_item, const String &p_text);
	String _get_item_path(TreeItem *p_item) const;
	bool _item_is_file(const TreeItem *p_item) const;

	HashMap<PlatformID, PlatformInfo> _platform_map;
	HashMap<TemplateID, TemplateInfo> _template_data;
	HashMap<String, bool> _folding_cache;
	TreeItem *_item_to_delete = nullptr;
	ItemList *_version_list = nullptr;
	Tree *_installed_templates_tree = nullptr;
	Button *_open_folder_btn = nullptr;
	Button *_delete_all_btn = nullptr;
	Button *_tpz_btn = nullptr;
	Dialog *_confirm_delete = nullptr;
	EditorFileDialog *_tpz_selection_dialog = nullptr;
	ThemeCache _theme_cache;
};
