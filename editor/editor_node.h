/**************************************************************************/
/*  editor_node.h                                                         */
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

#include "core/object/script_language.h"
#include "core/os/mutex.h"
#include "core/os/process_id.h"
#include "core/os/thread.h"
#include "core/string/print_string.h"
#include "core/templates/safe_refcount.h"
#include "editor/editor_data.h"
#include "editor/plugins/editor_plugin.h"
#include "editor/settings/editor_folding.h"
#include "scene/main/node.h"

using EditorNodeInitCallback = void (*)();

#ifndef ANDROID_ENABLED
class AndroidSDKManager;
#endif
class Dialog;
class ColorPicker;
class Control;
class FileDialog;
class HBoxContainer;
class ImageTexture;
class OptionButton;
class PanelContainer;
class RichTextLabel;
class SubViewport;
class TextureProgressBar;
class Timer;
class Translation;
class Tree;
class VSplitContainer;
class Window;

class AudioStreamImportSettingsDialog;
class AudioStreamPreviewGenerator;
class BackgroundProgress;
class DependencyErrorDialog;
class DockSplitContainer;
class DynamicFontImportSettingsDialog;
class EditorBuildProfileManager;
class EditorCommandPalette;
class EditorDockManager;
class EditorExport;
class EditorExportPreset;
class EditorFeatureProfileManager;
class EditorFileDialog;
class EditorIconManager;
class EditorFolding;
class EditorLayoutsDialog;
class EditorLog;
class EditorNativeShaderSourceVisualizer;
class EditorPluginList;
class EditorResourcePreview;
class EditorResourceConversionPlugin;
class EditorRunBar;
class EditorSceneTabs;
class EditorSelectionHistory;
class EditorSettingsDialog;
class EditorHeaderContainer;
class ExportTemplateManager;
class EditorQuickOpenDialog;
class FBXImporterManager;
class FileSystemDock;
class HistoryDock;
class OrphanResourcesDialog;
class ProgressDialog;
class ProjectAutoloadsDialog;
class ProjectExportDialog;
class ProjectPluginsDialog;
class ProjectSettingsEditor;
class SceneEditorDock;
class SceneImportSettingsDialog;

struct EditorProgress {
	EditorProgress(const String &p_task, const String &p_label, int p_amount, bool p_can_cancel = false, bool p_force_background = false);
	~EditorProgress();

	bool step(const String &p_state, int p_step = -1, bool p_force_refresh = true);

	String task;
	bool force_background = false;
};

class EditorNode : public Node {
	GDCLASS(EditorNode, Node);

public:
	enum class SceneNameCasing {
		AUTO,
		PASCAL_CASE,
		SNAKE_CASE,
		KEBAB_CASE,
		CAMEL_CASE,
	};

	enum class ActionOnPlay {
		DO_NOTHING,
		OPEN_OUTPUT,
		OPEN_DEBUGGER,
	};

	enum MenuOptions {
		// File menu.
		SCENE_NEW_SCENE,
		SCENE_NEW_INHERITED_SCENE,
		SCENE_OPEN_SCENE,
		SCENE_OPEN_PREV,
		SCENE_OPEN_RECENT,
		SCENE_SAVE_SCENE,
		SCENE_SAVE_AS_SCENE,
		SCENE_SAVE_ALL_SCENES,
		SCENE_MULTI_SAVE_AS_SCENE,
		SCENE_QUICK_OPEN,
		SCENE_QUICK_OPEN_SCENE,
		SCENE_QUICK_OPEN_SCRIPT,
		SCENE_EXPORT_AS,
		SCENE_UNDO,
		SCENE_REDO,
		SCENE_RELOAD_SAVED_SCENE,
		SCENE_CLOSE,
		SCENE_CLOSE_ALL,
		SCENE_QUIT,

		FILE_EXPORT_MESH_LIBRARY,

		// Project menu.
		PROJECT_OPEN_SETTINGS,
		PROJECT_OPEN_AUTOLOADS,
		PROJECT_OPEN_PLUGINS,
		PROJECT_FIND_IN_FILES,
		PROJECT_REPLACE_IN_FILES,
		PROJECT_VERSION_CONTROL,
		PROJECT_EXPORT,
		PROJECT_PACK_AS_ZIP,
		PROJECT_SETUP_ANDROID_BUILD,
		PROJECT_OPEN_USER_DATA_FOLDER,
		PROJECT_RELOAD_CURRENT_PROJECT,
		PROJECT_QUIT_TO_HUB,

		// Tools menu.
		TOOLS_ORPHAN_RESOURCES,
		TOOLS_BUILD_PROFILE_MANAGER,
		TOOLS_CLEAR_PROJECT_CACHE,
		TOOLS_CUSTOM,

		VCS_METADATA,
		VCS_SETTINGS,

		// Editor menu.
		EDITOR_TAKE_SCREENSHOT,
		EDITOR_OPEN_DATA_FOLDER,
		EDITOR_OPEN_CONFIG_FOLDER,
		EDITOR_MANAGE_FEATURE_PROFILES,
		EDITOR_MANAGE_EXPORT_TEMPLATES,
		EDITOR_CONFIGURE_FBX_IMPORTER,

		LAYOUT_SAVE,
		LAYOUT_DELETE,
		LAYOUT_DEFAULT,

		// Non-menu options.
		SCENE_TAB_CLOSE,
		SCENE_TAB_SAVE_SCENE,
		SCENE_TAB_SAVE_AS_AND_RUN,
		SCENE_TAB_SAVE_AS_MAIN_SCENE,
		SAVE_AND_RUN,
		SAVE_AND_RUN_MAIN_SCENE,
		RESOURCE_SAVE,
		RESOURCE_SAVE_AS,
		SETTINGS_PICK_MAIN_SCENE,
	};

	struct ExecuteThreadArgs {
		String path;
		List<String> args;
		String output;
		Thread execute_output_thread;
		Mutex execute_output_mutex;
		int exit_code = 0;
		SafeFlag done;
	};

	struct AdditiveNodeEntry {
		Node *node = nullptr;
		NodePath parent;
		Node *owner = nullptr;
		int index = 0;
		// Used if the original parent node is lost
		Transform2D transform_2d;
		Transform3D transform_3d;
	};

	struct ConnectionWithNodePath {
		Connection connection;
		NodePath node_path;
	};

	struct ModificationNodeEntry {
		HashMap<StringName, Variant> property_table;
		List<ConnectionWithNodePath> connections_to;
		List<Connection> connections_from;
		List<Node::GroupInfo> groups;
	};

	struct InstanceModificationsEntry {
		Node *original_node;
		String instance_path;
		List<Node *> instance_list;
		HashMap<NodePath, ModificationNodeEntry> modifications;
		List<AdditiveNodeEntry> addition_list;
	};

	struct SceneModificationsEntry {
		List<InstanceModificationsEntry> instance_list;
		HashMap<NodePath, ModificationNodeEntry> other_instances_modifications;
	};

	struct SceneEditorDataEntry {
		bool is_editable = false;
		bool is_display_folded = false;
	};

	EditorNode();
	~EditorNode() override;

	static EditorNode *get_singleton() { return _singleton; }

	static EditorLog *get_log() { return _singleton->_log; }
	static EditorData &get_editor_data() { return _singleton->_editor_data; }
	static EditorFolding &get_editor_folding() { return _singleton->_editor_folding; }

	static SceneEditorDock *get_scene_editor_dock() { return _singleton->_scene_editor_dock; }

	static String adjust_scene_name_casing(const String &p_root_name);
	static String adjust_script_name_casing(const String &p_file_name);

	static bool has_unsaved_changes() { return _singleton->_unsaved_cache; }
	static void disambiguate_filenames(const Vector<String> p_full_paths, Vector<String> &r_filenames);
	static void add_io_error(const String &p_error);
	static void add_io_warning(const String &p_warning);
	static bool find_recursive_resources(const Variant &p_variant, HashSet<Resource *> &r_resources_found);

	static void progress_add_task(const String &p_task, const String &p_label, int p_steps, bool p_can_cancel = false);
	static bool progress_task_step(const String &p_task, const String &p_state, int p_step = -1, bool p_force_refresh = true);
	static void progress_end_task(const String &p_task);

	static void progress_add_task_bg(const String &p_task, const String &p_label, int p_steps);
	static void progress_task_step_bg(const String &p_task, int p_step = -1);
	static void progress_end_task_bg(const String &p_task);

	static void add_editor_plugin(EditorPlugin *p_editor, bool p_config_changed = false);
	static void remove_editor_plugin(EditorPlugin *r_editor, bool p_config_changed = false);

	static void add_init_callback(EditorNodeInitCallback p_callback) { _init_callbacks.push_back(p_callback); }

	static bool immediate_confirmation_dialog(const String &p_text, const String &p_ok_text = TTRC("OK"), const String &p_cancel_text = TTRC("Cancel"), uint32_t p_wrap_width = 0);

	static bool is_cmdline_mode();

	static HashMap<String, Variant> get_initial_settings();

	static void cleanup();

	bool call_build();
	void call_run_scene(const String &p_scene, Vector<String> &r_args);

	// This is a very naive estimation, but we need something now. Will be reworked later.
	bool is_editor_ready() const { return is_inside_tree() && !_waiting_for_first_scan; }

	EditorPluginList *get_editor_plugins_force_input_forwarding() { return _editor_plugins_force_input_forwarding; }
	EditorPluginList *get_editor_plugins_force_over() { return _editor_plugins_force_over; }
	EditorPluginList *get_editor_plugins_over() { return _editor_plugins_over; }
	EditorSelection *get_editor_selection() { return _editor_selection; }
	EditorSelectionHistory *get_editor_selection_history() { return &_editor_history; }

	ProjectSettingsEditor *get_project_settings() { return _project_settings_editor; }

	void trigger_menu_option(int p_option, bool p_confirmed);
	bool has_previous_closed_scenes() const;

	void new_inherited_scene() { _menu_option_confirm(SCENE_NEW_INHERITED_SCENE, false); }

	void edit_node(Node *p_node);
	void edit_resource(const Ref<Resource> &p_resource);

	void save_resource_in_path(const Ref<Resource> &r_resource, const String &p_path);
	void save_resource(const Ref<Resource> &p_resource);
	void save_resource_as(const Ref<Resource> &p_resource, const String &p_at_path = String());
	bool is_resource_internal_to_scene(Ref<Resource> p_resource);
	void gather_resources(const Variant &p_variant, List<Ref<Resource>> &r_list, HashSet<Object *> &r_scanned_objects, bool p_subresources = false, bool p_allow_external = false);
	void update_resource_count(Node *p_node, bool p_remove = false);
	void update_node_reference(const Variant &p_value, Node *p_node, bool p_remove = false);
	void clear_node_reference(Ref<Resource> p_res);
	int get_resource_count(Ref<Resource> p_res);
	List<Node *> get_resource_node_list(Ref<Resource> p_res);

	void push_item(Object *p_object, const String &p_property = "", bool p_inspector_only = false);
	void push_item_no_inspector(Object *p_object);
	void edit_previous_item();
	void edit_item(Object *p_object, Object *p_editing_owner, bool p_set_current = true);
	void push_node_item(Node *p_node);
	void hide_unused_editors(const Object *p_editing_owner = nullptr);

	void replace_resources_in_object(
			Object *r_object,
			const Vector<Ref<Resource>> &p_source_resources,
			const Vector<Ref<Resource>> &p_target_resource);
	void replace_resources_in_scenes(
			const Vector<Ref<Resource>> &p_source_resources,
			const Vector<Ref<Resource>> &p_target_resource);
	void edit_foreign_resource(Ref<Resource> p_resource);

	bool is_resource_read_only(Ref<Resource> p_resource, bool p_foreign_resources_are_writable = false);

	String get_multiwindow_support_tooltip_text() const;

	bool is_changing_scene() const;

	SubViewport *get_scene_root() { return _edited_scene_viewport; }
	// Viewport that hosts the root of the scene being edited.

	void set_edited_scene(Node *p_scene);
	void set_edited_scene_root(Node *p_scene, bool p_auto_add);
	Node *get_edited_scene() { return _editor_data.get_edited_scene_root(); }

	String get_preview_locale() const;
	bool is_pseudolocalization_enabled() const;
	void set_preview_locale(const String &p_locale, bool p_pseudolocalization);

	int new_scene();
	Error load_scene(const String &p_scene, bool p_ignore_broken_deps = false, bool p_set_inherited = false, bool p_force_open_imported = false, bool p_update_tabs = true);
	Error open_scene(const String &p_scene, bool p_ignore_broken_deps = false, bool p_set_inherited = false, bool p_force_open_imported = false);
	Error load_resource(const String &p_resource, bool p_ignore_broken_deps = false);
	Error load_scene_or_resource(const String &p_path, bool p_ignore_broken_deps = false, bool p_change_scene_tab_if_already_open = true);

	HashMap<StringName, Variant> get_modified_properties_for_node(Node *p_node, bool p_node_references_only);
	HashMap<StringName, Variant> get_modified_properties_reference_to_nodes(Node *p_node, const List<Node *> &p_nodes_referenced_by);

	void set_unfocused_low_processor_usage_mode_enabled(bool p_enabled);

	void update_node_from_node_modification_entry(Node *r_node, const ModificationNodeEntry &p_node_modification);

	void get_scene_editor_data_for_node(Node *p_root, Node *p_node, HashMap<NodePath, SceneEditorDataEntry> &r_table);

	void get_preload_scene_modification_table(
			Node *p_edited_scene,
			Node *p_reimported_root,
			Node *p_node,
			InstanceModificationsEntry &r_instance_modifications);

	void get_preload_modifications_reference_to_nodes(
			Node *p_root,
			Node *p_node,
			const HashSet<Node *> &p_excluded_nodes,
			const List<Node *> &p_instance_list_with_children,
			HashMap<NodePath, ModificationNodeEntry> &r_modification_table);
	void get_children_nodes(Node *p_node, List<Node *> &r_nodes);
	bool is_additional_node_in_scene(Node *p_edited_scene, Node *p_reimported_root, Node *p_node);

	void replace_history_reimported_nodes(Node *p_original_root_node, Node *p_new_root_node, Node *p_node);

	bool is_scene_open(const String &p_path);
	bool is_multi_window_enabled() const;

	static void setup_built_in_resource(const Ref<Resource> &r_resource, const String &p_owner_path);
	void setup_color_picker(ColorPicker *r_picker);
	void register_hdr_viewport(Viewport *r_viewport);

	void request_instantiate_scenes(const Vector<String> &p_files);
	void handle_dropped_files(const Vector<String> &p_files);

	void notify_all_debug_sessions_exited();

	ProcessID has_child_process(ProcessID p_pid) const;
	void stop_child_process(ProcessID p_pid);

	Ref<Theme> get_editor_theme() const { return _theme; }
	void update_preview_themes();

	Ref<Script> get_object_custom_type_base(const Object *p_object) const;
	StringName get_object_custom_type_name(const Object *p_object) const;
	// p_fallback selects a shared Editor icon identifier when the type has no icon.
	Ref<Texture2D> get_object_icon(const Object *p_object, const String &p_fallback = "");
	Ref<Texture2D> get_class_icon(const String &p_class, const String &p_fallback = "");

	bool is_object_of_custom_type(const Object *p_object, const StringName &p_class);

	void show_save_accept(const String &p_text, const String &p_ok_text = TTRC("OK"));
	void show_warning(const String &p_text, const String &p_title = TTRC("Warning!"));

	Error export_preset(const String &p_preset, const String &p_path, bool p_debug, bool p_pack_only, bool p_android_build_template, bool p_patch, const Vector<String> &p_patches);

	Control *get_gui_base() { return _editor_overlay; }

	void save_scene_to_path(String p_file, bool p_with_preview = true) {
		if (p_with_preview) {
			_save_scene_with_preview(p_file);
		} else {
			_save_scene(p_file);
		}
	}

	bool close_scene();

	void save_editor_layout_delayed();
	void save_default_environment();

	void open_export_template_manager();

	void reload_scene(const String &p_path);

	void find_all_instances_inheriting_path_in_node(Node *p_root, Node *p_node, const String &p_instance_path, HashSet<Node *> &r_instance_list);
	void preload_reimporting_with_path_in_edited_scenes(const List<String> &p_scenes);
	void reload_instances_with_path_in_edited_scenes();

	bool is_exiting() const { return _exiting; }

	Dictionary drag_resource(const Ref<Resource> &p_res, Control *r_from);
	Dictionary drag_files_and_dirs(const Vector<String> &p_paths, Control *r_from);

	EditorQuickOpenDialog *get_quick_open_dialog() { return _quick_open_dialog; }

	void add_tool_menu_item(const String &p_name, const Callable &p_callback);
	void add_tool_submenu_item(const String &p_name, PopupMenu *p_submenu);
	void remove_tool_menu_item(const String &p_name);

	PopupMenu *get_export_as_menu();

	void save_all_scenes();
	void save_scene_if_open(const String &p_scene_path);
	void save_scene_list(const HashSet<String> &p_scene_paths);
	void save_before_run();
	void try_autosave();
	void restart_editor(bool p_goto_hub = false);

	void open_setting_override(const String &p_property);
	void notify_settings_overrides_changed();

	void dim_editor(bool p_dimming);

	void edit_current() { _edit_current(); }

	bool has_scenes_in_session();

	void undo();
	void redo();

	int execute_and_show_output(const String &p_title, const String &p_path, const List<String> &p_arguments, bool p_close_on_ok = true, bool p_close_on_errors = false, String *r_output = nullptr);

	Error setup_android_build_template(const Ref<EditorExportPreset> &p_preset, bool p_confirmed);

	void add_resource_conversion_plugin(const Ref<EditorResourceConversionPlugin> &p_plugin);
	void remove_resource_conversion_plugin(const Ref<EditorResourceConversionPlugin> &p_plugin);
	Vector<Ref<EditorResourceConversionPlugin>> find_resource_conversion_plugin_for_resource(const Ref<Resource> &p_for_resource);
	Vector<Ref<EditorResourceConversionPlugin>> find_resource_conversion_plugin_for_type_name(const String &p_type);

	bool ensure_main_scene(bool p_from_native);
	bool validate_custom_directory();
	void run_editor_script(const Ref<Script> &p_script);
	bool is_scene_unsaved(int p_idx);

	bool is_portrait() const { return _portrait_mode; }

protected:
	static void _bind_methods();
	void _notification(int p_what);

	void input(const Ref<InputEvent> &p_event) override;
	void shortcut_input(const Ref<InputEvent> &p_event) override;

private:
	friend class FileSystemDock;

	struct DeferredExport {
		String preset;
		String path;
		bool debug = false;
		bool pack_only = false;
		bool android_build_template = false;
		bool patch = false;
		Vector<String> patches;
	};

	static void _dependency_error_report(const String &p_path, const String &p_dep, const String &p_type) {
		DEV_ASSERT(Thread::get_caller_id() == Thread::get_main_id());
		if (!_singleton->_dependency_errors.has(p_path)) {
			_singleton->_dependency_errors[p_path] = HashSet<String>();
		}
		_singleton->_dependency_errors[p_path].insert(p_dep + "::" + p_type);
	}

	static Ref<Texture2D> _file_dialog_get_icon(const String &p_path);
	static Ref<Texture2D> _file_dialog_get_thumbnail(const String &p_path);
	static void _file_dialog_thumbnail_callback(const String &p_path, const Ref<Texture2D> &p_preview, const Ref<Texture2D> &p_small_preview, Ref<ImageTexture> r_texture);

	static void _file_dialog_register(FileDialog *p_dialog);
	static void _file_dialog_unregister(FileDialog *p_dialog);

	static void _file_access_close_error_notify(const String &p_str);
	static void _file_access_close_error_notify_impl(const String &p_str);

	static void _print_handler(void *p_this, const String &p_string, bool p_error, bool p_rich);
	static void _print_handler_impl(const String &p_string, bool p_error, bool p_rich);
	static void _resource_saved(Ref<Resource> p_resource, const String &p_path);
	static void _resource_loaded(Ref<Resource> p_resource, const String &p_path);

	void _update_main_menu_icons();
	void _update_theme(bool p_skip_creation = false);
	void _update_main_layout_style(bool p_theme_changed = false);
	void _build_icon_type_cache();

	void _dialog_action(String p_file);

	void _add_to_history(const Object *p_object, const String &p_property, bool p_inspector_only);
	void _edit_current(bool p_skip_foreign = false, bool p_skip_inspector_update = false);
	void _dialog_display_save_error(String p_file, Error p_error);
	void _dialog_display_load_error(String p_file, Error p_error);

	void _menu_option(int p_option);
	void _menu_confirm_current();
	void _menu_option_confirm(int p_option, bool p_confirmed);

	void _android_build_source_selected(const String &p_file);
	void _android_export_preset_selected(int p_index);
	void _android_install_build_template();
	void _android_explore_build_templates();
	void _android_remove_build_templates(bool p_prompt_for_removal);
	void _setup_android_build(bool p_confirmed);

	void _request_screenshot();
	void _screenshot(bool p_use_utc = false);
	void _save_screenshot(const String &p_path);
	void _save_screenshot_with_embedded_process(int64_t p_w, int64_t p_h, const String &p_emb_path, const Rect2i &p_rect, const String &p_path);

	void _check_system_theme_changed();

	void _tool_menu_option(int p_idx);
	void _export_as_menu_option(int p_idx);
	void _update_file_menu_opened();
	void _palette_quick_open_dialog();
	void _clear_cache_confirmed();

	void _plugin_over_edit(EditorPlugin *r_plugin, Object *p_object, bool p_set_current = true);
	void _register_plugin_as_its_own_owner(EditorPlugin *p_plugin);

	void _filesystem_changed();
	void _resources_reimporting(const Vector<String> &p_resources);
	void _resources_reimported(const Vector<String> &p_resources);
	void _sources_changed(bool p_exist);
	void _remove_lock_file();

	void _node_renamed();
	void _save_editor_states(const String &p_file, int p_idx = -1);

	void _update_title();
	void _update_unsaved_cache();
	void _version_control_menu_option(int p_idx);
	void _titlebar_resized();
	void _viewport_resized();

	int _save_external_resources(bool p_also_save_external_data = false);
	void _save_scene_silently();

	void _set_current_scene(int p_idx);
	void _apply_edited_scene(int p_idx, bool p_ignore_state = false);

	bool _validate_scene_recursive(const String &p_filename, Node *p_node);
	void _save_scene(String p_file, int p_idx = -1);
	void _save_all_scenes();
	int _next_unsaved_scene(bool p_valid_filename, int p_start = 0);
	void _discard_changes(const String &p_str = String());
	void _scene_tab_closed(int p_tab);
	void _cancel_close_scene_tab();
	void _cancel_confirmation();

	void _prepare_save_confirmation_popup();

	void _inherit_request(String p_file);
	void _instantiate_request(const Vector<String> &p_files);

	void _quick_opened(const String &p_file_path);
	void _open_command_palette();

	void _project_run_started();

	void _update_prev_closed_scenes(const String &p_scene_path, bool p_add_scene);

	void _add_to_recent_scenes(const String &p_scene);
	void _update_recent_scenes();
	void _open_recent_scene(int p_idx);

	void _add_dropped_files_recursive(const Vector<String> &p_files, String p_to_path);

	void _update_vsync_mode();
	void _update_from_settings();
	void _update_translations();
	void _translation_resources_changed();
	void _queue_translation_notification();
	void _propagate_translation_notification();
	void _queue_resource_counter_changed();
	void _emit_resource_counter_changed();

	void _exit_editor(int p_exit_code);

	void _remove_edited_scene(bool p_change_tab = true);
	void _remove_scene(int p_idx, bool p_change_tab = true);
	bool _find_and_save_resource(Ref<Resource> r_res, HashMap<Ref<Resource>, bool> &r_processed, int32_t p_flags);
	bool _find_and_save_edited_subresources(Object *p_object, HashMap<Ref<Resource>, bool> &r_processed, int32_t p_flags);
	void _save_edited_subresources(Node *p_scene, HashMap<Ref<Resource>, bool> &r_processed, int32_t p_flags);
	void _mark_unsaved_scenes();

	void _find_node_types(Node *p_node, int &r_count_2d, int &r_count_3d);
	void _save_scene_with_preview(String p_file, int p_idx = -1);
	void _close_save_scene_progress();

	void _proceed_closing_scene_tabs();
	void _save_next_scene_tab_as();
	bool _is_closing_editor() const;
	void _restart_editor(bool p_goto_hub = false);

	Dictionary _get_main_scene_state();
	void _set_main_scene_state(const Dictionary &p_state);
	Ref<ConfigFile> _load_scene_config(const String &p_scene_path);

	void _save_editor_layout();
	void _load_editor_layout();

	void _save_central_editor_layout_to_config(Ref<ConfigFile> r_config_file);
	void _load_central_editor_layout_from_config(Ref<ConfigFile> p_config_file);

	void _save_window_settings_to_config(Ref<ConfigFile> r_layout, const String &p_section);

	void _save_open_scenes_to_config(Ref<ConfigFile> r_layout);
	void _load_open_scenes_from_config(Ref<ConfigFile> p_layout);

	void _update_layouts_menu();
	void _layout_menu_option(int p_id);

	void _inherit_imported(const String &p_action);
	void _open_imported();

	void _resources_changed(const Vector<String> &p_resources);
	void _scan_external_changes();
	void _reload_modified_scenes();
	void _reload_project_settings();
	void _resave_externally_modified_scenes(String p_str);

	void _feature_profile_changed();
	bool _is_class_editor_disabled_by_feature_profile(const StringName &p_class);

	Ref<Texture2D> _get_class_or_script_icon(const String &p_class, const String &p_script_path, const String &p_fallback = "", bool p_fallback_script_to_theme = false, bool p_skip_fallback_virtual = false);

	void _pick_main_scene_custom_action(const String &p_custom_action_name);

	void _immediate_dialog_confirmed();

	void _begin_first_scan();

	void _notify_nodes_scene_reimported(Node *p_node, Array p_reimported_nodes);

	void _remove_all_not_owned_children(Node *p_node, Node *p_owner);

	void _progress_dialog_visibility_changed();
	void _load_error_dialog_visibility_changed();

	bool _is_project_data_missing();

	void _build_file_menu();
	void _build_project_menu();
	void _build_settings_menu();

	void _copy_warning(const String &p_str);
#ifdef ANDROID_ENABLED
	void _screen_orientation_changed(int p_new_orientation);
#endif

	DeferredExport _deferred_export;

	static EditorNode *_singleton;

	EditorData _editor_data;
	EditorFolding _editor_folding;
	EditorSelectionHistory _editor_history;

	EditorCommandPalette *_command_palette = nullptr;
	EditorQuickOpenDialog *_quick_open_dialog = nullptr;
	EditorExport *_editor_export = nullptr;
	EditorLog *_log = nullptr;
	EditorNativeShaderSourceVisualizer *_native_shader_source_visualizer = nullptr;
	EditorPluginList *_editor_plugins_force_input_forwarding = nullptr;
	EditorPluginList *_editor_plugins_force_over = nullptr;
	EditorPluginList *_editor_plugins_over = nullptr;
	EditorQuickOpenDialog *_quick_open_color_palette = nullptr;
	EditorResourcePreview *_resource_preview = nullptr;
	EditorSelection *_editor_selection = nullptr;
	EditorSettingsDialog *_editor_settings_dialog = nullptr;
	HistoryDock *_history_dock = nullptr;

	ProjectExportDialog *_project_export_dialog = nullptr;
	ProjectAutoloadsDialog *_project_autoloads_dialog = nullptr;
	ProjectPluginsDialog *_project_plugins_dialog = nullptr;
	ProjectSettingsEditor *_project_settings_editor = nullptr;

	FBXImporterManager *_fbx_importer_manager = nullptr;

	Vector<EditorPlugin *> _editor_plugins;
	HashMap<ObjectID, HashSet<EditorPlugin *>> _active_plugins;

	Control *_editor_overlay = nullptr;
	PanelContainer *_main_layout_pc = nullptr;
	PanelContainer *_dock_layout_pc = nullptr;

	bool _portrait_mode = false;

	// Main tabs.
	EditorSceneTabs *_scene_tabs = nullptr;

	int _tab_closing_idx = 0;
	List<String> _tabs_to_close;
	List<int> _scenes_to_save_as;
	int _tab_closing_menu_option = -1;

	bool _exiting = false;
	bool _dimmed = false;

	Control *_vp_base = nullptr;

	Label *_project_title_lbl = nullptr;
	Control *_left_menu_spacer = nullptr;
	EditorHeaderContainer *_editor_header = nullptr;
	EditorRunBar *_project_run_bar = nullptr;

	PopupMenu *_file_menu = nullptr;
	PopupMenu *_project_menu = nullptr;
	PopupMenu *_settings_menu = nullptr;
	PopupMenu *_tool_menu = nullptr;
	PopupMenu *_export_as_menu = nullptr;
	Button *_export_btn = nullptr;

	Timer *_screenshot_timer = nullptr;

	RichTextLabel *_load_error_log = nullptr;
	Dialog *_load_error_dialog = nullptr;
	bool _load_errors_queued_to_display = false;

	RichTextLabel *_process_output_log = nullptr;
	Dialog *_execute_output_dialog = nullptr;

	Ref<Theme> _theme;
	EditorIconManager *_icon_manager = nullptr;

	bool _follow_system_theme = false;
	bool _use_system_accent_color = false;
	bool _last_dark_mode_state = false;
	Color _last_system_base_color = Color(0, 0, 0, 0);
	Color _last_system_accent_color = Color(0, 0, 0, 0);

	PopupMenu *_recent_scenes_menu = nullptr;
	String _recent_scene;
	LocalVector<String> _prev_closed_scenes;
	String _defer_load_scene;
	Node *_last_instantiated_scene = nullptr;

	Dialog *_confirmation_dialog = nullptr;
	bool _stop_project_confirmation = false;
	Button *_discard_changes_btn = nullptr;
	Dialog *_save_confirmation_dialog = nullptr;
	Dialog *_import_confirmation_dialog = nullptr;
	Dialog *_pick_main_scene_dialog = nullptr;
	Dialog *_open_project_settings_dialog = nullptr;
	Button *_select_current_scene_btn = nullptr;
	Dialog *_save_message_dialog = nullptr;
	Dialog *_warning_dialog = nullptr;
	Callable _editor_save_callback;

	bool _overridden_default_layout = false;
	Ref<ConfigFile> _default_layout;
	PopupMenu *_editor_layouts_menu = nullptr;
	EditorLayoutsDialog *_editor_layout_dialog = nullptr;

	Dialog *_missing_android_build_template_dialog = nullptr;
	Dialog *_install_android_build_template_dialog = nullptr;
	Dialog *_remove_android_build_template_dialog = nullptr;
	Label *_install_android_build_template_lbl = nullptr;
	OptionButton *_android_export_profile_btn = nullptr;
	Ref<EditorExportPreset> _android_export_preset;

	PopupMenu *_vcs_actions_menu = nullptr;
	EditorFileDialog *_scene_resource_file_dialog = nullptr;
	ExportTemplateManager *_export_template_manager = nullptr;
	EditorFeatureProfileManager *_feature_profile_manager = nullptr;
	EditorBuildProfileManager *_build_profile_manager = nullptr;
	EditorFileDialog *_export_template_import_dialog = nullptr;
	EditorFileDialog *_mesh_library_export_dialog = nullptr;
	EditorFileDialog *_android_build_source_file_dialog = nullptr;
	EditorFileDialog *_project_zip_export_dialog = nullptr;

	SceneEditorDock *_scene_editor_dock = nullptr;

	AudioStreamPreviewGenerator *_audio_preview_generator = nullptr;
	ProgressDialog *_progress_dialog = nullptr;
	BackgroundProgress *_background_progress = nullptr;

	DependencyErrorDialog *_dependency_error_dialog = nullptr;
	HashMap<String, HashSet<String>> _dependency_errors;
	OrphanResourcesDialog *_orphan_resources_dialog = nullptr;
	Dialog *_open_imported_dialog = nullptr;
	Button *_new_inherited_scene_btn = nullptr;
	String _open_import_request;

	EditorDockManager *_editor_dock_manager = nullptr;
	Timer *_editor_layout_save_delay_timer = nullptr;
	Timer *_scan_changes_timer = nullptr;
	Callable _palette_file_selected_callback;

	Tree *_external_changes_tree = nullptr;
	HashSet<String> _externally_changed_scene_paths;
	bool _project_settings_changed_on_disk = false;
	Dialog *_external_changes_dialog = nullptr;
	Dialog *_missing_project_data_dialog = nullptr;

	bool _changing_scene = false;
	bool _cmdline_mode = false;
	bool _immediate_dialog_was_confirmed = false;
	bool _restoring_scenes = false;
	bool _settings_overrides_changed = false;
	bool _unsaved_cache = false;

	bool _requested_first_scan = false;
	bool _waiting_for_first_scan = true;
	bool _load_editor_layout_done = false;

	HashSet<Ref<Translation>> _tracked_translations;
	bool _pending_translation_notification = false;
	bool _resource_counter_queued = false;

	int _current_menu_option = 0;

	SubViewport *_edited_scene_viewport = nullptr;
	// Viewport that hosts the root of the scene being edited.

	Ref<Resource> _saving_resource;
	HashSet<Ref<Resource>> _saving_resources_in_path;
	HashMap<Ref<Resource>, List<Node *>> _resource_node_references;
	// Keeps track of linked Resources from a Scene.

	String _saving_scene_path;
	EditorProgress *_save_scene_progress = nullptr;

	DynamicFontImportSettingsDialog *_dynamic_font_import_settings_dialog = nullptr;
	SceneImportSettingsDialog *_scene_import_settings_dialog = nullptr;
	AudioStreamImportSettingsDialog *_audio_stream_import_settings_dialog = nullptr;

	HashSet<String> _force_textfile_extensions;
	HashSet<String> _textfile_extensions;
	HashSet<String> _other_file_extensions;
	HashSet<FileDialog *> _file_dialogs;
	LocalVector<ObjectID> _hdr_viewports;

	Vector<Ref<EditorResourceConversionPlugin>> _resource_conversion_plugins;
	PrintHandlerList _print_handler_registration;

	HashMap<String, Ref<Texture2D>> _icon_type_cache;
	HashMap<Pair<String, String>, Ref<Texture2D>> _class_icon_cache;

	Dialog *_clear_cache_dialog = nullptr;

	LocalVector<String> _files_to_delete_on_exit;

	bool _was_window_windowed_last = false;
	bool _uses_padded_layout_style = false;

	bool _unfocused_low_processor_usage_mode_enabled = true;

#ifndef ANDROID_ENABLED
	AndroidSDKManager *_android_sdk_manager = nullptr;
#endif

	static Vector<EditorNodeInitCallback> _init_callbacks;
	Ref<ImageTexture> _default_thumbnail;

	HashMap<int, SceneModificationsEntry> _scenes_modification_table;
	List<String> _scenes_reimported;
	List<String> _resources_pending_reimport;
};

struct EditorProgressBG {
	EditorProgressBG(const String &p_task, const String &p_label, int p_amount) :
			task(p_task) {
		EditorNode::progress_add_task_bg(task, p_label, p_amount);
	}
	~EditorProgressBG() { EditorNode::progress_end_task_bg(task); }

	void step(int p_step = -1) { EditorNode::progress_task_step_bg(task, p_step); }

	String task;
};
