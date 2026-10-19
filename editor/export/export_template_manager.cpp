/**************************************************************************/
/*  export_template_manager.cpp                                           */
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

#include "export_template_manager.h"

#include "core/error/error_macros.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/zip_io.h"
#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "core/version.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/export/editor_export.h"
#include "editor/export/editor_export_platform.h"
#include "editor/export/editor_export_preset.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/file_system/editor_paths.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/progress_dialog.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tree.h"
#include "scene/resources/texture.h"
#include "scene/scene_string_names.h"

ExportTemplateManager::ExportTemplateManager() {
	set_ok_button_text(TTRC("Close"));
	set_title(TTRC("Export Template Manager"));

	_tpz_selection_dialog = memnew(EditorFileDialog);
	_tpz_selection_dialog->add_filter("*.tpz", TTRC("Godot Export Templates"));
	_tpz_selection_dialog->connect("file_selected", callable_mp(this, &ExportTemplateManager::_tpz_file_selected));
	_tpz_selection_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	_tpz_selection_dialog->set_current_dir(EDITOR_DEF("_export_template_install_directory", ""));
	_tpz_selection_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	_tpz_selection_dialog->set_title(TTRC("Select Template File"));
	add_child(_tpz_selection_dialog);

	VBoxContainer *main_vbox = memnew(VBoxContainer);
	add_child(main_vbox);
	{
		HBoxContainer *install_hbox = memnew(HBoxContainer);
		main_vbox->add_child(install_hbox);
		{
			_tpz_btn = memnew(Button);
			_tpz_btn->connect(SceneStringName(pressed), callable_mp((FileDialog *)_tpz_selection_dialog, &FileDialog::popup_file_dialog));
			_tpz_btn->set_text(TTRC("Install from TPZ File..."));
			_tpz_btn->set_tooltip_text(TTRC("Install export templates from a local TPZ file."));
			install_hbox->add_child(_tpz_btn);
		}

		HSplitContainer *main_split = memnew(HSplitContainer);
		main_split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		main_vbox->add_child(main_split);
		{
			VBoxContainer *versions_vbox = memnew(VBoxContainer);
			main_split->add_child(versions_vbox);
			{
				Label *version_lbl = memnew(Label(TTRC("Godot Version")));
				version_lbl->set_theme_type_variation("HeaderSmall");
				versions_vbox->add_child(version_lbl);

				_version_list = memnew(ItemList);
				_version_list->connect(SceneStringName(item_selected), callable_mp(this, &ExportTemplateManager::_version_selected).unbind(1));
				_version_list->set_theme_type_variation("ItemListSecondary");
				_version_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
				versions_vbox->add_child(_version_list);
			}

			VBoxContainer *installed_vbox = memnew(VBoxContainer);
			installed_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			main_split->add_child(installed_vbox);
			{
				HBoxContainer *installed_hbox = memnew(HBoxContainer);
				installed_vbox->add_child(installed_hbox);
				{
					Label *installed_lbl = memnew(Label(TTRC("Installed Templates")));
					installed_lbl->set_theme_type_variation("HeaderSmall");
					installed_hbox->add_child(installed_lbl);

					_open_folder_btn = memnew(Button);
					_open_folder_btn->connect(SceneStringName(pressed), callable_mp(this, &ExportTemplateManager::_open_template_directory));
					_open_folder_btn->set_h_size_flags(Control::SIZE_EXPAND | Control::SIZE_SHRINK_END);
					_open_folder_btn->set_tooltip_text(TTRC("Open templates directory."));
					installed_hbox->add_child(_open_folder_btn);

					_delete_all_btn = memnew(Button);
					_delete_all_btn->connect(SceneStringName(pressed), callable_mp(this, &ExportTemplateManager::_delete_all));
					_delete_all_btn->set_tooltip_text(TTRC("Remove all installed templates."));
					installed_hbox->add_child(_delete_all_btn);
				}

				_installed_templates_tree = memnew(Tree);
				_installed_templates_tree->connect("button_clicked", callable_mp(this, &ExportTemplateManager::_tree_button_clicked));
				_installed_templates_tree->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
				_installed_templates_tree->set_hide_root(true);
				_installed_templates_tree->set_theme_type_variation("TreeSecondary");
				_installed_templates_tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
				installed_vbox->add_child(_installed_templates_tree);
			}
		}
	}

	_confirm_delete = memnew(Dialog(true));
	_confirm_delete->connect(SceneStringName(confirmed), callable_mp(this, &ExportTemplateManager::_delete_confirmed));
	_confirm_delete->set_flag(Window::FLAG_RESIZE_DISABLED, true);
	add_child(_confirm_delete);
}

String ExportTemplateManager::get_android_build_directory(const Ref<EditorExportPreset> &p_preset) {
	if (p_preset.is_valid()) {
		String gradle_build_dir = p_preset->get("gradle_build/gradle_build_directory");
		if (!gradle_build_dir.is_empty()) {
			return gradle_build_dir.path_join("build");
		}
	}
	return "res://android/build";
}

String ExportTemplateManager::get_android_source_zip(const Ref<EditorExportPreset> &p_preset) {
	if (p_preset.is_valid()) {
		String android_source_zip = p_preset->get("gradle_build/android_source_template");
		if (!android_source_zip.is_empty()) {
			return android_source_zip;
		}
	}

	const String templates_dir = EditorPaths::get_singleton()->get_export_templates_dir().path_join(GODOT_VERSION_FULL_CONFIG);
	return templates_dir.path_join("android_source.zip");
}

String ExportTemplateManager::get_android_template_identifier(const Ref<EditorExportPreset> &p_preset) {
	// The template identifier is the Godot version for the default template, and the full path plus md5 hash for custom templates.
	if (p_preset.is_valid()) {
		String android_source_zip = p_preset->get("gradle_build/android_source_template");
		if (!android_source_zip.is_empty()) {
			return android_source_zip + String(" [") + FileAccess::get_md5(android_source_zip) + String("]");
		}
	}
	return GODOT_VERSION_FULL_CONFIG;
}

bool ExportTemplateManager::is_android_template_installed(const Ref<EditorExportPreset> &p_preset) {
	return DirAccess::exists(get_android_build_directory(p_preset));
}

bool ExportTemplateManager::is_android_build_version_valid(const Ref<EditorExportPreset> &p_preset) {
	String build_version_path = get_android_build_directory(p_preset).get_base_dir().path_join(".build_version");
	Ref<FileAccess> f = FileAccess::open(build_version_path, FileAccess::READ);
	if (f.is_valid()) {
		String current_version = get_android_template_identifier(p_preset);
		String installed_version = f->get_line().strip_edges();
		if (current_version == installed_version) {
			return true;
		}
	}
	return false;
}

bool ExportTemplateManager::can_install_android_template(const Ref<EditorExportPreset> &p_preset) {
	return FileAccess::exists(get_android_source_zip(p_preset));
}

Error ExportTemplateManager::delete_android_build_directory(const Ref<EditorExportPreset> &p_preset) {
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	ERR_FAIL_COND_V(da.is_null(), ERR_CANT_CREATE);

	String build_dir = get_android_build_directory(p_preset);
	String parent_dir = build_dir.get_base_dir();
	print_verbose("Deleting android build directory " + build_dir);

	ProgressDialog::get_singleton()->add_task("deleting_android_build_dir", TTR("Deleting Android Build Directory"), 1);

	Error err = da->change_dir(parent_dir);
	ERR_FAIL_COND_V(err != OK, err);

	err = da->erase_contents_recursive();
	ERR_FAIL_COND_V(err != OK, err);

	ProgressDialog::get_singleton()->end_task("deleting_android_build_dir");
	EditorFileSystem::get_singleton()->scan_changes();
	return OK;
}

Error ExportTemplateManager::install_android_template(const Ref<EditorExportPreset> &p_preset) {
	const String source_zip = get_android_source_zip(p_preset);
	ERR_FAIL_COND_V(!FileAccess::exists(source_zip), ERR_CANT_OPEN);
	return install_android_template_from_file(source_zip, p_preset);
}

Error ExportTemplateManager::install_android_template_from_file(const String &p_file, const Ref<EditorExportPreset> &p_preset) {
	// To support custom Android builds, we install the Java source code and buildsystem
	// from android_source.zip to the project's res://android folder.

	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	ERR_FAIL_COND_V(da.is_null(), ERR_CANT_CREATE);

	String build_dir = get_android_build_directory(p_preset);
	String parent_dir = build_dir.get_base_dir();
	print_verbose("Installing android build directory " + build_dir);

	// Make parent of the build dir (if it does not exist).
	da->make_dir_recursive(parent_dir);
	{
		// Add identifier, to ensure building won't work if the current template doesn't match.
		Ref<FileAccess> f = FileAccess::open(parent_dir.path_join(".build_version"), FileAccess::WRITE);
		ERR_FAIL_COND_V(f.is_null(), ERR_CANT_CREATE);
		f->store_line(get_android_template_identifier(p_preset));
	}

	// Create the android build directory.
	Error err = da->make_dir_recursive(build_dir);
	ERR_FAIL_COND_V(err != OK, err);
	{
		// Add an empty .gdignore file to avoid scan.
		Ref<FileAccess> f = FileAccess::open(build_dir.path_join(".gdignore"), FileAccess::WRITE);
		ERR_FAIL_COND_V(f.is_null(), ERR_CANT_CREATE);
		f->store_line("");
	}

	// Uncompress source template.

	Ref<FileAccess> io_fa;
	zlib_filefunc_def io = zipio_create_io(&io_fa);

	unzFile pkg = unzOpen2(p_file.utf8().get_data(), &io);
	ERR_FAIL_NULL_V_MSG(pkg, ERR_CANT_OPEN, "Android sources not in ZIP format.");

	int ret = unzGoToFirstFile(pkg);
	int total_files = 0;
	// Count files to unzip.
	while (ret == UNZ_OK) {
		total_files++;
		ret = unzGoToNextFile(pkg);
	}
	ret = unzGoToFirstFile(pkg);

	ProgressDialog::get_singleton()->add_task("uncompress_src", TTR("Uncompressing Android Build Sources"), total_files);

	HashSet<String> dirs_tested;
	int idx = 0;
	while (ret == UNZ_OK) {
		// Get file path.
		unz_file_info info;
		char fpath[16384];
		ret = unzGetCurrentFileInfo(pkg, &info, fpath, 16384, nullptr, 0, nullptr, 0);
		if (ret != UNZ_OK) {
			break;
		}

		String path = String::utf8(fpath);
		String base_dir = path.get_base_dir();

		if (!path.ends_with("/")) {
			Vector<uint8_t> uncomp_data;
			uncomp_data.resize(info.uncompressed_size);

			// Read.
			unzOpenCurrentFile(pkg);
			unzReadCurrentFile(pkg, uncomp_data.ptrw(), uncomp_data.size());
			unzCloseCurrentFile(pkg);

			if (!dirs_tested.has(base_dir)) {
				da->make_dir_recursive(build_dir.path_join(base_dir));
				dirs_tested.insert(base_dir);
			}

			String to_write = build_dir.path_join(path);
			Ref<FileAccess> f = FileAccess::open(to_write, FileAccess::WRITE);
			if (f.is_valid()) {
				f->store_buffer(uncomp_data.ptr(), uncomp_data.size());
				f.unref(); // close file.
#ifndef WINDOWS_ENABLED
				FileAccess::set_unix_permissions(to_write, (info.external_fa >> 16) & 0x01FF);
#endif
			} else {
				ERR_PRINT("Can't uncompress file: " + to_write);
			}
		}

		ProgressDialog::get_singleton()->task_step("uncompress_src", path, idx);

		idx++;
		ret = unzGoToNextFile(pkg);
	}

	ProgressDialog::get_singleton()->end_task("uncompress_src");
	unzClose(pkg);
	EditorFileSystem::get_singleton()->scan_changes();
	return OK;
}

void ExportTemplateManager::popup_manager() {
	if (_template_data.is_empty()) {
		_initialize_template_data();
	}
	_update_version_list();
	_update_template_tree();
	popup_centered_clamped(Vector2(640, 500) * EDSCALE);
}

void ExportTemplateManager::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_TRANSLATION_CHANGED: {
			if (!_template_data.is_empty()) {
				_platform_map[PlatformID::COMMON].name = TTR("Common");
				_template_data[TemplateID::ICU_DATA].name = TTR("ICU Data");
				_update_template_tree();
			}
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			_open_folder_btn->set_button_icon(get_editor_theme_icon("Folder"));
			_delete_all_btn->set_button_icon(get_editor_theme_icon("Remove"));
			_tpz_btn->set_button_icon(get_editor_theme_icon("FileBrowse"));
			_theme_cache.remove_icon = get_editor_theme_icon("Remove");
			_theme_cache.current_version_color = get_theme_color("accent_color", EditorStringName(Editor));
			_theme_cache.incomplete_template_color = get_theme_color("warning_color", EditorStringName(Editor));
			_theme_cache.missing_file_color = get_theme_color("error_color", EditorStringName(Editor));
			_theme_cache.icon_width = get_theme_constant("class_icon_size", EditorStringName(Editor));
		} break;
	}
}

void ExportTemplateManager::_delete_all() {
	_item_to_delete = _installed_templates_tree->get_root();
	_confirm_delete->set_text(TTRC("Remove all installed template files? (Cannot be undone.)\nDepending on your filesystem configuration, the files will either be moved to the system trash or deleted permanently."));
	_confirm_delete->popup_centered();
}

void ExportTemplateManager::_delete_confirmed() {
	ERR_FAIL_NULL(_item_to_delete);
	_delete_file(_item_to_delete);

	const String selected_version = _version_list->get_item_text(_version_list->get_current());
	if (selected_version != GODOT_VERSION_FULL_CONFIG) {
		// Deleting all installed templates removes the version from list.
		_update_version_list();
	}
	_update_template_tree();
	_item_to_delete = nullptr;
}

void ExportTemplateManager::_delete_file(const TreeItem *p_item) {
	if (_item_is_file(p_item)) {
		const String selected_version = _version_list->get_item_text(_version_list->get_current());
		const String full_path = _get_template_folder_path(selected_version).path_join(p_item->get_text(0));

		if (FileAccess::exists(full_path)) {
			OS::get_singleton()->move_to_trash(full_path);
		}
	} else {
		for (TreeItem *child = p_item->get_first_child(); child; child = child->get_next()) {
			_delete_file(child);
		}
	}
}

void ExportTemplateManager::_tpz_file_selected(const String &p_file) {
	Ref<FileAccess> io_fa;
	zlib_filefunc_def io = zipio_create_io(&io_fa);

	unzFile pkg = unzOpen2(p_file.utf8().get_data(), &io);
	if (!pkg) {
		EditorNode::get_singleton()->show_warning(TTR("Can't open the export templates file."));
		return;
	}
	int ret = unzGoToFirstFile(pkg);

	// Count them and find version.
	int fc = 0;
	String version;
	String contents_dir;

	while (ret == UNZ_OK) {
		unz_file_info info;
		char fname[16384];
		ret = unzGetCurrentFileInfo(pkg, &info, fname, 16384, nullptr, 0, nullptr, 0);
		if (ret != UNZ_OK) {
			break;
		}

		String file = String::utf8(fname);

		// Skip the __MACOSX directory created by macOS's built-in file zipper.
		if (file.begins_with("__MACOSX")) {
			ret = unzGoToNextFile(pkg);
			continue;
		}

		if (file.ends_with("version.txt")) {
			Vector<uint8_t> uncomp_data;
			uncomp_data.resize(info.uncompressed_size);

			// Read.
			unzOpenCurrentFile(pkg);
			ret = unzReadCurrentFile(pkg, uncomp_data.ptrw(), uncomp_data.size());
			ERR_BREAK_MSG(ret < 0, vformat("An error occurred while attempting to read from file: %s. This file will not be used.", file));
			unzCloseCurrentFile(pkg);

			String data_str = String::utf8((const char *)uncomp_data.ptr(), uncomp_data.size());
			data_str = data_str.strip_edges();

			// Version number should be of the form major.minor[.patch].status[.double]
			// so it can in theory have 3 or more slices.
			if (data_str.get_slice_count(".") < 3) {
				EditorNode::get_singleton()->show_warning(vformat(TTR("Invalid version.txt format inside the export templates file: %s."), data_str));
				unzClose(pkg);
				return;
			}

			version = data_str;
			contents_dir = file.get_base_dir().trim_suffix("/").trim_suffix("\\");
		}

		if (file.get_file().size() != 0) {
			fc++;
		}

		ret = unzGoToNextFile(pkg);
	}

	if (version.is_empty()) {
		EditorNode::get_singleton()->show_warning(TTR("No version.txt found inside the export templates file."));
		unzClose(pkg);
		return;
	}

	Ref<DirAccess> d = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	String template_path = EditorPaths::get_singleton()->get_export_templates_dir().path_join(version);
	Error err = d->make_dir_recursive(template_path);
	if (err != OK) {
		EditorNode::get_singleton()->show_warning(TTR("Error creating path for extracting templates:") + "\n" + template_path);
		unzClose(pkg);
		return;
	}

	EditorProgress ep("export_tpz", TTR("Extracting Export Templates"), fc);

	fc = 0;
	ret = unzGoToFirstFile(pkg);
	while (ret == UNZ_OK) {
		// Get filename.
		unz_file_info info;
		char fname[16384];
		ret = unzGetCurrentFileInfo(pkg, &info, fname, 16384, nullptr, 0, nullptr, 0);
		if (ret != UNZ_OK) {
			break;
		}

		if (String::utf8(fname).ends_with("/")) {
			// File is a directory, ignore it.
			// Directories will be created when extracting each file.
			ret = unzGoToNextFile(pkg);
			continue;
		}

		String file_path(String::utf8(fname).simplify_path());

		String file = file_path.get_file();

		// Skip the __MACOSX directory created by macOS's built-in file zipper.
		if (file.is_empty() || file.begins_with("__MACOSX")) {
			ret = unzGoToNextFile(pkg);
			continue;
		}

		Vector<uint8_t> uncomp_data;
		uncomp_data.resize(info.uncompressed_size);

		// Read
		unzOpenCurrentFile(pkg);
		ret = unzReadCurrentFile(pkg, uncomp_data.ptrw(), uncomp_data.size());
		ERR_BREAK_MSG(ret < 0, vformat("An error occurred while attempting to read from file: %s. This file will not be used.", file));
		unzCloseCurrentFile(pkg);

		String base_dir = file_path.get_base_dir().trim_suffix("/");

		if (base_dir != contents_dir && base_dir.begins_with(contents_dir)) {
			base_dir = base_dir.substr(contents_dir.length(), file_path.length()).trim_prefix("/");
			file = base_dir.path_join(file);

			Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
			ERR_CONTINUE(da.is_null());

			String output_dir = template_path.path_join(base_dir);

			if (!DirAccess::exists(output_dir)) {
				Error mkdir_err = da->make_dir_recursive(output_dir);
				ERR_CONTINUE(mkdir_err != OK);
			}
		}
		ep.step(TTR("Importing:") + " " + file, fc);

		String to_write = template_path.path_join(file);
		Ref<FileAccess> f = FileAccess::open(to_write, FileAccess::WRITE);

		if (f.is_null()) {
			ret = unzGoToNextFile(pkg);
			fc++;
			ERR_CONTINUE_MSG(true, "Can't open file from path '" + String(to_write) + "'.");
		}

		f->store_buffer(uncomp_data.ptr(), uncomp_data.size());
		f.unref(); // close file.
#ifndef WINDOWS_ENABLED
		FileAccess::set_unix_permissions(to_write, (info.external_fa >> 16) & 0x01FF);
#endif

		ret = unzGoToNextFile(pkg);
		fc++;
	}

	unzClose(pkg);

	EditorSettings::get_singleton()->set("_export_template_install_directory", p_file.get_base_dir());

	_update_version_list();
	for (int i = 0; i < _version_list->get_item_count(); i++) {
		if (_version_list->get_item_text(i) == version) {
			_version_list->select(i);
			break;
		}
	}
	_update_template_tree();
}

void ExportTemplateManager::_initialize_template_data() {
	// Base templates.
	{
		TemplateInfo info;
		info.name = "Windows x86_32";
		info.description = TTRC("32-bit build for Microsoft Windows, including console wrapper.");
		info.file_list = { "windows_debug_x86_32.exe", "windows_debug_x86_32_console.exe", "windows_release_x86_32.exe", "windows_release_x86_32_console.exe" };
		_template_data[TemplateID::WINDOWS_X86_32] = info;
	}
	{
		TemplateInfo info;
		info.name = "Windows x86_64";
		info.description = TTRC("64-bit build for Microsoft Windows, including console wrapper.");
		info.file_list = { "windows_debug_x86_64.exe", "windows_debug_x86_64_console.exe", "windows_release_x86_64.exe", "windows_release_x86_64_console.exe" };
		_template_data[TemplateID::WINDOWS_X86_64] = info;
	}
	{
		TemplateInfo info;
		info.name = "Windows arm64";
		info.description = TTRC("64-bit build for Microsoft Windows on ARM architecture, including console wrapper.");
		info.file_list = { "windows_debug_arm64.exe", "windows_debug_arm64_console.exe", "windows_release_arm64.exe", "windows_release_arm64_console.exe" };
		_template_data[TemplateID::WINDOWS_ARM64] = info;
	}

	{
		TemplateInfo info;
		info.name = "Linux x86_32";
		info.description = TTRC("32-bit build for Linux systems.");
		info.file_list = { "linux_debug.x86_32", "linux_release.x86_32" };
		_template_data[TemplateID::LINUX_X86_32] = info;
	}
	{
		TemplateInfo info;
		info.name = "Linux x86_64";
		info.description = TTRC("64-bit build for Linux systems.");
		info.file_list = { "linux_debug.x86_64", "linux_release.x86_64" };
		_template_data[TemplateID::LINUX_X86_64] = info;
	}
	{
		TemplateInfo info;
		info.name = "Linux arm32";
		info.description = TTRC("32-bit build for Linux systems on ARM architecture.");
		info.file_list = { "linux_debug.arm32", "linux_release.arm32" };
		_template_data[TemplateID::LINUX_ARM32] = info;
	}
	{
		TemplateInfo info;
		info.name = "Linux arm64";
		info.description = TTRC("64-bit build for Linux systems on ARM architecture.");
		info.file_list = { "linux_debug.arm64", "linux_release.arm64" };
		_template_data[TemplateID::LINUX_ARM64] = info;
	}

	{
		TemplateInfo info;
		info.name = "macOS";
		info.description = TTRC("Universal build for macOS.");
		info.file_list = { "macos.zip" };
		_template_data[TemplateID::MACOS] = info;
	}

	{
		TemplateInfo info;
		info.name = "Android";
		info.description = TTRC("Android APK template and source for Gradle builds.");
		info.file_list = { "android_debug.apk", "android_release.apk", "android_source.zip" };
		_template_data[TemplateID::ANDROID] = info;
	}

	{
		TemplateInfo info;
		info.name = "iOS";
		info.description = TTRC("Build for Apple's iOS.");
		info.file_list = { "ios.zip" };
		_template_data[TemplateID::IOS] = info;
	}

	{
		TemplateInfo info;
		info.name = TTR("ICU Data");
		info.description = TTRC("Line breaking dictionaries for TextServer, used by certain languages.");
		info.file_list = { "icudt_godot.dat" };
		_template_data[TemplateID::ICU_DATA] = info;
	}

	// Platforms.
	{
		PlatformInfo info;
		info.name = "Windows";
		info.icon = _get_platform_icon("Windows Desktop");
		info.templates = { TemplateID::WINDOWS_X86_32, TemplateID::WINDOWS_X86_64, TemplateID::WINDOWS_ARM64 };
		_platform_map[PlatformID::WINDOWS] = info;
	}
	{
		PlatformInfo info;
		info.name = "Linux";
		info.icon = _get_platform_icon("Linux");
		info.templates = { TemplateID::LINUX_X86_32, TemplateID::LINUX_X86_64, TemplateID::LINUX_ARM32, TemplateID::LINUX_ARM64 };
		_platform_map[PlatformID::LINUX] = info;
	}
	{
		PlatformInfo info;
		info.name = "macOS";
		info.icon = _get_platform_icon("macOS");
		info.templates = { TemplateID::MACOS };
		_platform_map[PlatformID::MACOS] = info;
	}
	{
		PlatformInfo info;
		info.name = "Android";
		info.icon = _get_platform_icon("Android");
		info.templates = { TemplateID::ANDROID };
		_platform_map[PlatformID::ANDROID] = info;
	}
	{
		PlatformInfo info;
		info.name = "iOS";
		info.icon = _get_platform_icon("iOS");
		info.templates = { TemplateID::IOS };
		_platform_map[PlatformID::IOS] = info;
	}
	{
		PlatformInfo info;
		info.name = TTR("Common");
		info.templates = { TemplateID::ICU_DATA };
		_platform_map[PlatformID::COMMON] = info;
	}

	// Template directory status.
	DirAccess::make_dir_recursive_absolute(_get_template_folder_path(GODOT_VERSION_FULL_CONFIG));
}

void ExportTemplateManager::_update_version_list() {
	String current_selected;
	if (_version_list->get_current() > -1) {
		current_selected = _version_list->get_item_text(_version_list->get_current());
	}
	_version_list->clear();

	Ref<DirAccess> templates_dir = DirAccess::open(EditorPaths::get_singleton()->get_export_templates_dir());
	ERR_FAIL_COND(templates_dir.is_null());

	int current_version_id = -1;
	for (const String &dir : templates_dir->get_directories()) {
		if (dir == GODOT_VERSION_FULL_CONFIG) {
			_version_list->add_item(dir);
			_version_list->set_item_custom_fg_color(-1, _theme_cache.current_version_color);
			current_version_id = _version_list->get_item_count() - 1;
		} else {
			bool recognized = false;
			for (const String &file : DirAccess::get_files_at(templates_dir->get_current_dir().path_join(dir))) {
				for (const KeyValue<TemplateID, TemplateInfo> &KV : _template_data) {
					if (KV.value.file_list.has(file)) {
						recognized = true;
						break;
					}
				}
				if (recognized) {
					break;
				}
			}
			if (!recognized) {
				continue;
			}
			_version_list->add_item(dir);
			if (dir == current_selected) {
				_version_list->select(_version_list->get_item_count() - 1);
			}
		}
	}
	if (_version_list->get_current() == -1) {
		_version_list->select(current_version_id);
	}
}

void ExportTemplateManager::_update_template_tree() {
	if (_installed_templates_tree->get_root()) {
		_update_folding_cache(_installed_templates_tree->get_root());
		_installed_templates_tree->clear();
	}
	_item_to_delete = nullptr;
	TreeItem *root = _installed_templates_tree->create_item();
	_setup_item_text(root, String());
	const int selected = _version_list->get_current();
	if (selected < 0) {
		_delete_all_btn->set_disabled(true);
		_open_folder_btn->set_disabled(true);
		return;
	}
	_open_folder_btn->set_disabled(false);
	const String selected_version = _version_list->get_item_text(selected);
	const String template_path = _get_template_folder_path(selected_version);
	for (const KeyValue<PlatformID, PlatformInfo> &KV : _platform_map) {
		const PlatformInfo &platform = KV.value;
		TreeItem *platform_item = nullptr;
		for (TemplateID id : platform.templates) {
			const TemplateInfo &info = _template_data[id];
			bool any_installed = false;
			for (const String &file : info.file_list) {
				if (FileAccess::exists(template_path.path_join(file))) {
					any_installed = true;
					break;
				}
			}
			if (!any_installed) {
				continue;
			}
			if (!platform_item) {
				platform_item = root->create_child();
				_setup_item_text(platform_item, platform.name);
				platform_item->set_icon(0, platform.icon);
				platform_item->set_icon_max_width(0, _theme_cache.icon_width);
			}
			TreeItem *template_item = platform_item;
			if (platform.templates.size() != 1 || info.name != platform.name) {
				template_item = platform_item->create_child();
				_setup_item_text(template_item, info.name);
			}
			template_item->set_tooltip_text(0, TTR(info.description));
			bool any_missing = false;
			for (const String &file : info.file_list) {
				TreeItem *file_item = template_item->create_child();
				file_item->set_meta(_FILE_META, true);
				_setup_item_text(file_item, file);
				if (FileAccess::exists(template_path.path_join(file))) {
					file_item->add_button(0, _theme_cache.remove_icon);
					file_item->set_button_tooltip_text(0, -1, TTR("Remove this file."));
				} else {
					file_item->set_custom_color(0, _theme_cache.missing_file_color);
					any_missing = true;
				}
			}
			if (any_missing) {
				template_item->set_custom_color(0, _theme_cache.incomplete_template_color);
			}
			template_item->add_button(0, _theme_cache.remove_icon);
			template_item->set_button_tooltip_text(0, -1, TTR("Remove this template."));
			_apply_item_folding(template_item, true);
		}
		if (platform_item) {
			_apply_item_folding(platform_item);
		}
	}
	_delete_all_btn->set_disabled(root->get_child_count() == 0);
	if (root->get_child_count() == 0) {
		TreeItem *empty = root->create_child();
		empty->set_text(0, TTR("No templates installed."));
		empty->set_custom_color(0, get_theme_color(SNAME("font_disabled_color"), EditorStringName(Editor)));
	}
}

void ExportTemplateManager::_update_folding_cache(TreeItem *p_item) {
	_folding_cache[_get_item_path(p_item)] = p_item->is_collapsed();
	for (TreeItem *child = p_item->get_first_child(); child; child = child->get_next()) {
		_update_folding_cache(child);
	}
}

String ExportTemplateManager::_get_template_folder_path(const String &p_version) const {
	return EditorPaths::get_singleton()->get_export_templates_dir().path_join(p_version);
}

Ref<Texture2D> ExportTemplateManager::_get_platform_icon(const String &p_platform_name) {
	for (int i = 0; i < EditorExport::get_singleton()->get_export_platform_count(); i++) {
		Ref<EditorExportPlatform> platform = EditorExport::get_singleton()->get_export_platform(i);
		if (platform->get_name() == p_platform_name) {
			return platform->get_logo();
		}
	}
	return Ref<Texture2D>();
}

void ExportTemplateManager::_version_selected() {
	_update_template_tree();
}

void ExportTemplateManager::_tree_button_clicked(TreeItem *p_item, int p_column, int p_id, MouseButton p_button) {
	_item_to_delete = p_item;
	_confirm_delete->set_text(TTRC("Remove the selected template files? (Cannot be undone.)\nDepending on your filesystem configuration, the files will either be moved to the system trash or deleted permanently."));
	_confirm_delete->popup_centered();
}

void ExportTemplateManager::_open_template_directory() {
	const String selected_version = _version_list->get_item_text(_version_list->get_current());
	OS::get_singleton()->shell_show_in_file_manager(_get_template_folder_path(selected_version), true);
}

void ExportTemplateManager::_apply_item_folding(TreeItem *p_item, bool p_default) {
	if (_folding_cache.is_empty()) {
		if (p_default) {
			p_item->set_collapsed(true);
		}
	} else {
		const bool *cached = _folding_cache.getptr(_get_item_path(p_item));
		if (cached) {
			p_item->set_collapsed(*cached);
		} else if (p_default) {
			p_item->set_collapsed(true);
		}
	}
}

void ExportTemplateManager::_setup_item_text(TreeItem *p_item, const String &p_text) {
	if (p_item == _installed_templates_tree->get_root()) {
		p_item->set_meta(_PATH_META, "installed");
	} else {
		p_item->set_text(0, p_text);
		const String path = p_item->get_parent()->get_meta(_PATH_META).operator String() + "/" + p_text;
		p_item->set_meta(_PATH_META, path);
	}
}

String ExportTemplateManager::_get_item_path(TreeItem *p_item) const {
	return p_item->get_meta(_PATH_META, String());
}

bool ExportTemplateManager::_item_is_file(const TreeItem *p_item) const {
	return p_item->get_meta(_FILE_META, false).operator bool();
}
