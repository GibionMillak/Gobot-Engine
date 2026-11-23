/**************************************************************************/
/*  editor_node.cpp                                                       */
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

#include "editor_node.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/io/config_file.h"
#include "core/io/file_access.h"
#include "core/io/file_access_content.h"
#include "core/io/image.h"
#include "core/io/missing_resource.h"
#include "core/io/resource_importer.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "core/os/time.h"
#include "core/string/print_string.h"
#include "core/string/translation_server.h"
#include "core/templates/local_vector.h"
#include "core/version.h"
#include "editor/animation/animation_player_editor_plugin.h"
#include "editor/audio/audio_stream_editor_plugin.h"
#include "editor/audio/audio_stream_preview.h"
#include "editor/audio/editor_audio_buses.h"
#include "editor/debugger/debugger_editor_plugin.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/debugger/script_editor_debugger.h"
#include "editor/doc/editor_help.h"
#include "editor/docks/dock_tab_container.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/docks/groups_dock.h"
#include "editor/docks/hierarchy_dock.h"
#include "editor/docks/history_dock.h"
#include "editor/docks/import_dock.h"
#include "editor/docks/inspector_dock.h"
#include "editor/docks/scene_editor/scene_editor_dock.h"
#include "editor/docks/script_editor/script_editor_dock.h"
#include "editor/docks/signals_dock.h"
#include "editor/editor_data.h"
#include "editor/editor_interface.h"
#include "editor/editor_log.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/export/dedicated_server_export_plugin.h"
#include "editor/export/editor_export.h"
#include "editor/export/export_template_manager.h"
#include "editor/export/project_export.h"
#include "editor/export/project_zip_packer.h"
#include "editor/export/register_exporters.h"
#include "editor/file_system/dependency_editor.h"
#include "editor/file_system/editor_paths.h"
#include "editor/gui/containers/editor_footer_container.h"
#include "editor/gui/containers/editor_header_container.h"
#include "editor/gui/containers/editor_toaster_container.h"
#include "editor/gui/dialogs/editor_quick_open_dialog.h"
#include "editor/gui/dialogs/editor_settings_dialog.h"
#include "editor/gui/dialogs/project_autoloads_dialog.h"
#include "editor/gui/dialogs/project_plugins_dialog.h"
#include "editor/gui/dialogs/project_settings_editor.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_icon_manager.h"
#include "editor/gui/progress_dialog.h"
#include "editor/gui/window_wrapper.h"
#include "editor/import/3d/editor_import_collada.h"
#include "editor/import/3d/resource_importer_obj.h"
#include "editor/import/3d/resource_importer_scene.h"
#include "editor/import/3d/scene_import_settings.h"
#include "editor/import/audio_stream_import_settings.h"
#include "editor/import/dynamic_font_import_settings.h"
#include "editor/import/fbx_importer_manager.h"
#include "editor/import/resource_importer_bitmask.h"
#include "editor/import/resource_importer_bmfont.h"
#include "editor/import/resource_importer_csv_translation.h"
#include "editor/import/resource_importer_dynamic_font.h"
#include "editor/import/resource_importer_image.h"
#include "editor/import/resource_importer_imagefont.h"
#include "editor/import/resource_importer_layered_texture.h"
#include "editor/import/resource_importer_shader_file.h"
#include "editor/import/resource_importer_streamed_texture.h"
#include "editor/import/resource_importer_svg.h"
#include "editor/import/resource_importer_texture.h"
#include "editor/import/resource_importer_texture_atlas.h"
#include "editor/import/resource_importer_wav.h"
#include "editor/inspector/editor_context_menu_plugin.h"
#include "editor/inspector/editor_inspector.h"
#include "editor/inspector/editor_preview_plugins.h"
#include "editor/inspector/editor_properties.h"
#include "editor/inspector/editor_property_name_processor.h"
#include "editor/inspector/editor_resource_picker.h"
#include "editor/inspector/editor_resource_preview.h"
#include "editor/inspector/multi_node_edit.h"
#include "editor/managers/editor_dialog_manager.h"
#include "editor/managers/editor_managers.h"
#include "editor/managers/project_autoload_manager.h"
#include "editor/managers/project_plugin_manager.h"
#include "editor/plugins/editor_plugin.h"
#include "editor/plugins/editor_plugin_list.h"
#include "editor/plugins/editor_resource_conversion_plugin.h"
#include "editor/run/editor_run.h"
#include "editor/run/editor_run_bar.h"
#include "editor/run/game_view_plugin.h"
#include "editor/scene/3d/material_3d_conversion_plugins.h"
#include "editor/scene/3d/mesh_library_editor_plugin.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "editor/scene/3d/node_3d_editor_viewport.h"
#include "editor/scene/3d/root_motion_editor_plugin.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/scene/editor_scene_tabs.h"
#include "editor/scene/material_editor_plugin.h"
#include "editor/scene/particle_process_material_editor_plugin.h"
#include "editor/script/editor_script.h"
#include "editor/script/find_in_files.h"
#include "editor/script/script_text_editor.h"
#include "editor/script/text_editor.h"
#include "editor/settings/editor_build_profile.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_feature_profile.h"
#include "editor/settings/editor_layouts_dialog.h"
#include "editor/settings/editor_settings.h"
#include "editor/shader/editor_native_shader_source_visualizer.h"
#include "editor/shader/shader_text_editor.h"
#include "editor/themes/editor_icons.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme_manager.h"
#include "editor/translations/editor_translation_parser.h"
#include "editor/translations/packed_scene_translation_parser_plugin.h"
#include "editor/version_control/version_control_editor_plugin.h"
#include "main/main.h"
#include "scene/2d/node_2d.h"
#include "scene/3d/bone_attachment_3d.h"
#include "scene/animation/animation_tree.h"
#include "scene/gui/box_container.h"
#include "scene/gui/color_picker.h"
#include "scene/gui/control.h"
#include "scene/gui/dialog.h"
#include "scene/gui/file_dialog.h"
#include "scene/gui/menu_bar.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/popup.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tab_container.h"
#include "scene/main/scene_tree.h"
#include "scene/main/timer.h"
#include "scene/main/window.h"
#include "scene/property_utils.h"
#include "scene/resources/3d/mesh_library.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/portable_compressed_texture.h"
#include "scene/resources/style_box.h"
#include "scene/theme/theme_db.h"
#include "servers/audio/audio_server.h"
#include "servers/display/display_server.h"
#include "servers/display/display_server_enums.h"
#include "servers/navigation_2d/navigation_server_2d.h"
#include "servers/navigation_3d/navigation_server_3d.h"
#include "servers/physics_3d/physics_server_3d_manager.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_server.h"

#ifdef RD_ENABLED
#include "editor/export/shader_baker/shader_baker_export_plugin.h"

#ifdef VULKAN_ENABLED
#include "editor/shader/shader_baker/shader_baker_export_plugin_platform_vulkan.h"
#endif

#ifdef D3D12_ENABLED
#include "editor/shader/shader_baker/shader_baker_export_plugin_platform_d3d12.h"
#endif

#ifdef METAL_ENABLED
#include "editor/shader/shader_baker/shader_baker_export_plugin_platform_metal.h"
#endif
#endif // RD_ENABLED

#ifndef PHYSICS_2D_DISABLED
#include "servers/physics_2d/physics_server_2d.h"
#endif // PHYSICS_2D_DISABLED

#ifndef PHYSICS_3D_DISABLED
#include "servers/physics_3d/physics_server_3d.h"
#endif // PHYSICS_3D_DISABLED

#ifndef ANDROID_ENABLED
#include "editor/export/android_sdk_manager.h"
#endif

#include "modules/modules_enabled.gen.h" // For gdscript.

#include <cstdlib>

EditorNode *EditorNode::_singleton = nullptr;

static const String EDITOR_NODE_CONFIG_SECTION = "EditorNode";

static const String ANDROID_BUILD_TEMPLATE_ALREADY_INSTALLED_MESSAGE = TTRC("The Android build template is already installed.");
static const String REMOVE_ANDROID_BUILD_TEMPLATE_MESSAGE = TTRC("Remove the \"%s\" directory manually, or enable automatic deletion in the Editor Settings (Export > Android > Build > Automatically Delete Build Directory) before attempting this operation again.");
static const String INSTALL_ANDROID_BUILD_TEMPLATE_MESSAGE = TTRC("This will set up your project for gradle Android builds by installing the source template to \"%s\".\nNote that in order to make gradle builds instead of using pre-built APKs, the \"Use Gradle Build\" option should be enabled in the Android export preset.");

constexpr int LARGE_RESOURCE_WARNING_SIZE_THRESHOLD = 512'000; // 500 KB

EditorProgress::EditorProgress(const String &p_task, const String &p_label, int p_amount, bool p_can_cancel, bool p_force_background) {
	if (!p_force_background && Thread::is_main_thread()) {
		EditorNode::progress_add_task(p_task, p_label, p_amount, p_can_cancel);
	} else {
		EditorNode::progress_add_task_bg(p_task, p_label, p_amount);
	}
	task = p_task;
	force_background = p_force_background;
}

EditorProgress::~EditorProgress() {
	if (!force_background && Thread::is_main_thread()) {
		EditorNode::progress_end_task(task);
	} else {
		EditorNode::progress_end_task_bg(task);
	}
}

bool EditorProgress::step(const String &p_state, int p_step, bool p_force_refresh) {
	if (!force_background && Thread::is_main_thread()) {
		return EditorNode::progress_task_step(task, p_state, p_step, p_force_refresh);
	} else {
		EditorNode::progress_task_step_bg(task, p_step);
		return false;
	}
}

static void _reset_animation_mixers(Node *p_node, List<Pair<AnimationMixer *, Ref<AnimatedValuesBackup>>> *r_anim_backups) {
	for (int i = 0; i < p_node->get_child_count(); i++) {
		AnimationMixer *mixer = Object::cast_to<AnimationMixer>(p_node->get_child(i));
		if (mixer && mixer->is_active() && mixer->is_reset_on_save_enabled() && mixer->can_apply_reset()) {
			AnimationTree *tree = Object::cast_to<AnimationTree>(p_node->get_child(i));
			if (tree) {
				AnimationPlayer *player = Object::cast_to<AnimationPlayer>(tree->get_node_or_null(tree->get_animation_player()));
				if (player && player->is_active() && player->is_reset_on_save_enabled() && player->can_apply_reset()) {
					continue; // Avoid to process reset/restore many times.
				}
			}
			Ref<AnimatedValuesBackup> backup = mixer->apply_reset();
			if (backup.is_valid()) {
				Pair<AnimationMixer *, Ref<AnimatedValuesBackup>> pair;
				pair.first = mixer;
				pair.second = backup;
				r_anim_backups->push_back(pair);
			}
		}
		_reset_animation_mixers(p_node->get_child(i), r_anim_backups);
	}
}

static String _get_unsaved_scene_dialog_text(String p_scene_filename, uint64_t p_opened_timestamp) {
	const uint64_t scene_modified_time = FileAccess::get_modified_time(p_scene_filename);
	String unsaved_message;

	// Consider scene opening to be a point of saving, so that when you
	// close and reopen the editor, you don't get an excessively long
	// "modified X hours ago".
	const uint64_t last_modified_seconds = Time::get_singleton()->get_unix_time_from_system() - MAX(p_opened_timestamp, scene_modified_time);

	String last_modified_string;
	if (last_modified_seconds < 120) {
		last_modified_string = vformat(TTRN("%d second ago", "%d seconds ago", last_modified_seconds), last_modified_seconds);
	} else if (last_modified_seconds < 7200) {
		last_modified_string = vformat(TTRN("%d minute ago", "%d minutes ago", last_modified_seconds / 60), last_modified_seconds / 60);
	} else {
		last_modified_string = vformat(TTRN("%d hour ago", "%d hours ago", last_modified_seconds / 3600), last_modified_seconds / 3600);
	}

	String last_action_and_time;
	if (p_opened_timestamp > scene_modified_time) {
		last_action_and_time = vformat(TTR("Scene opened: %s."), last_modified_string);
	} else {
		last_action_and_time = vformat(TTR("Last saved: %s."), last_modified_string);
	}

	unsaved_message = vformat(TTR("Scene \"%s\" has unsaved changes.\n%s"), p_scene_filename, last_action_and_time);

	return unsaved_message;
}

// Used to track the progress of tasks in the CLI output (since we don't have any other frame of reference).
static HashMap<String, int> progress_total_steps;

static String last_progress_task;
static String last_progress_state;
static int last_progress_step = 0;
static double last_progress_time = 0;

Vector<EditorNodeInitCallback> EditorNode::_init_callbacks;

static Node *_resource_get_edited_scene() {
	return EditorNode::get_singleton()->get_edited_scene();
}

static void _execute_thread(void *r_userdata) {
	EditorNode::ExecuteThreadArgs *execution = (EditorNode::ExecuteThreadArgs *)r_userdata;
	Error err = OS::get_singleton()->execute(execution->path, execution->args, &execution->output, &execution->exit_code, true, &execution->execute_output_mutex);
	print_verbose("Thread exit status: " + itos(execution->exit_code));
	if (err != OK) {
		execution->exit_code = err;
	}

	execution->done.set();
}

#ifdef MACOS_ENABLED
extern "C" GameViewPluginBase *get_game_view_plugin();
#else
GameViewPluginBase *get_game_view_plugin() {
	return memnew(GameViewPlugin);
}
#endif

EditorNode::EditorNode() {
	DEV_ASSERT(!_singleton);
	_singleton = this;

	// Detecting headless mode, that means the editor is running in command line.
	_cmdline_mode = (DisplayServer::get_singleton()->get_name() == "headless");

	Resource::_get_local_scene_func = _resource_get_edited_scene;

	{
		PortableCompressedTexture2D::set_keep_all_compressed_buffers(true);
		RenderingServer::get_singleton()->set_debug_generate_wireframes(true);

		AudioServer::get_singleton()->set_enable_tagging_used_audio_streams(true);

		// No navigation by default if in editor.
		if (NavigationServer3D::get_singleton()->get_debug_enabled()) {
			NavigationServer3D::get_singleton()->set_active(true);
		} else {
			NavigationServer3D::get_singleton()->set_active(false);
		}

		// No physics by default if in editor.
#ifndef PHYSICS_3D_DISABLED
		PhysicsServer3D::get_singleton()->set_active(false);
#endif // PHYSICS_3D_DISABLED
#ifndef PHYSICS_2D_DISABLED
		PhysicsServer2D::get_singleton()->set_active(false);
#endif // PHYSICS_2D_DISABLED

		// No scripting by default if in editor (except for tool).
		ScriptServer::set_scripting_enabled(false);

		if (!DisplayServer::get_singleton()->is_touchscreen_available()) {
			// Only if no touchscreen ui hint, disable emulation just in case.
			Input::get_singleton()->set_emulate_touch_from_mouse(false);
		}
		if (DisplayServer::get_singleton()->has_feature(DisplayServerEnums::FEATURE_CUSTOM_CURSOR_SHAPE)) {
			for (int i = 0; i < DisplayServerEnums::CURSOR_MAX; i++) {
				DisplayServer::get_singleton()->cursor_set_custom_image(Ref<Resource>(), static_cast<DisplayServerEnums::CursorShape>(i));
			}
		}
	}

	SceneState::set_disable_placeholders(true);
	ResourceLoader::clear_translation_remaps(); // Using no remaps if in editor.
	ResourceLoader::set_create_missing_resources_if_class_unavailable(true);

	EditorPropertyNameProcessor *epnp = memnew(EditorPropertyNameProcessor);
	add_child(epnp);

	EditorUndoRedoManager::get_singleton()->connect("version_changed", callable_mp(this, &EditorNode::_update_unsaved_cache));
	EditorUndoRedoManager::get_singleton()->connect("history_changed", callable_mp(this, &EditorNode::_update_unsaved_cache));
	ProjectSettings::get_singleton()->connect("settings_changed", callable_mp(this, &EditorNode::_update_from_settings));

	Ref<TranslationDomain> domain = TranslationServer::get_singleton()->get_main_domain();
	domain->set_enabled(false);
	domain->set_locale_override(TranslationServer::get_singleton()->get_default_context());

	// Load settings.
	if (!EditorSettings::get_singleton()) {
		EditorSettings::create();
	}

	ED_SHORTCUT("editor/lock_selected_nodes", TTRC("Lock Selected Node(s)"), KeyModifierMask::CMD_OR_CTRL | Key::L);
	ED_SHORTCUT("editor/unlock_selected_nodes", TTRC("Unlock Selected Node(s)"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::L);
	ED_SHORTCUT("editor/group_selected_nodes", TTRC("Group Selected Node(s)"), KeyModifierMask::CMD_OR_CTRL | Key::G);
	ED_SHORTCUT("editor/ungroup_selected_nodes", TTRC("Ungroup Selected Node(s)"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::G);
	ED_SHORTCUT("editor/toggle_selected_nodes_visibility", TTRC("Toggle Selected Node(s) Visibility"), Key::H);

	FileAccess::set_backup_save(EDITOR_GET("filesystem/on_save/safe_save_on_backup_then_rename"));

	_update_vsync_mode();

	{
		bool agile_input_event_flushing = EDITOR_GET("input/buffering/agile_event_flushing");
		bool use_accumulated_input = EDITOR_GET("input/buffering/use_accumulated_input");

		Input::get_singleton()->set_agile_input_event_flushing(agile_input_event_flushing);
		Input::get_singleton()->set_use_accumulated_input(use_accumulated_input);
	}

	{
		int display_scale = EDITOR_GET("interface/editor/appearance/display_scale");

		switch (display_scale) {
			case 0:
				// Try applying a suitable display scale automatically.
				EditorScale::set_scale(EditorSettings::get_auto_display_scale());
				break;
			case 1:
				EditorScale::set_scale(0.75);
				break;
			case 2:
				EditorScale::set_scale(1.0);
				break;
			case 3:
				EditorScale::set_scale(1.25);
				break;
			case 4:
				EditorScale::set_scale(1.5);
				break;
			case 5:
				EditorScale::set_scale(1.75);
				break;
			case 6:
				EditorScale::set_scale(2.0);
				break;
			default:
				EditorScale::set_scale(EDITOR_GET("interface/editor/appearance/custom_display_scale"));
				break;
		}
	}

	// Define a minimum window size to prevent UI elements from overlapping or being cut off.
	Window *w = Object::cast_to<Window>(SceneTree::get_singleton()->get_root());
	if (w) {
		const Size2 display_size = DisplayServer::get_singleton()->screen_get_usable_rect(DisplayServerEnums::SCREEN_OF_MAIN_WINDOW).size;
		const real_t smallest_display_dimension = display_size.width < display_size.height ? display_size.width : display_size.height;
		const Size2 editor_minimum_size = Size2(1024, 600) * EDSCALE;
		// Ensure the minimum size is not larger than the display size to avoid issues on smaller screens.
		const Size2 computed_minimum_size = editor_minimum_size.minf(smallest_display_dimension);
		w->set_min_size(computed_minimum_size); // Calling it this early doesn't sync the property with DS.
		DisplayServer::get_singleton()->window_set_min_size(computed_minimum_size);
	}

	FileDialog::set_default_show_hidden_files(EDITOR_GET("filesystem/file_dialog/show_hidden_files"));
	FileDialog::set_default_display_mode(EDITOR_GET("filesystem/file_dialog/display_mode"));

	ResourceLoader::set_abort_on_missing_resources(false);
	ResourceLoader::set_error_notify_func(&EditorNode::add_io_error);
	ResourceLoader::set_dependency_error_notify_func(&EditorNode::_dependency_error_report);

	SceneState::set_instantiation_warning_notify_func([](const String &p_warning) {
		add_io_warning(p_warning);
		callable_mp(EditorInterface::get_singleton(), &EditorInterface::mark_scene_as_unsaved).call_deferred();
	});

	{
		// Register importers at the beginning, so dialogs are created with the right extensions.
		Ref<ResourceImporterTexture> import_texture;
		import_texture.instantiate(true);
		ResourceFormatImporter::get_singleton()->add_importer(import_texture);

		Ref<ResourceImporterStreamedTexture> import_streamed_texture;
		import_streamed_texture.instantiate(true);
		ResourceFormatImporter::get_singleton()->add_importer(import_streamed_texture);

		Ref<ResourceImporterLayeredTexture> import_cubemap;
		import_cubemap.instantiate();
		import_cubemap->set_mode(ResourceImporterLayeredTexture::MODE_CUBEMAP);
		ResourceFormatImporter::get_singleton()->add_importer(import_cubemap);

		Ref<ResourceImporterLayeredTexture> import_array;
		import_array.instantiate();
		import_array->set_mode(ResourceImporterLayeredTexture::MODE_2D_ARRAY);
		ResourceFormatImporter::get_singleton()->add_importer(import_array);

		Ref<ResourceImporterLayeredTexture> import_cubemap_array;
		import_cubemap_array.instantiate();
		import_cubemap_array->set_mode(ResourceImporterLayeredTexture::MODE_CUBEMAP_ARRAY);
		ResourceFormatImporter::get_singleton()->add_importer(import_cubemap_array);

		Ref<ResourceImporterLayeredTexture> import_3d = memnew(ResourceImporterLayeredTexture(true));
		import_3d->set_mode(ResourceImporterLayeredTexture::MODE_3D);
		ResourceFormatImporter::get_singleton()->add_importer(import_3d);

		Ref<ResourceImporterImage> import_image;
		import_image.instantiate();
		ResourceFormatImporter::get_singleton()->add_importer(import_image);

		Ref<ResourceImporterSVG> import_svg;
		import_svg.instantiate();
		ResourceFormatImporter::get_singleton()->add_importer(import_svg);

		Ref<ResourceImporterTextureAtlas> import_texture_atlas;
		import_texture_atlas.instantiate();
		ResourceFormatImporter::get_singleton()->add_importer(import_texture_atlas);

		Ref<ResourceImporterDynamicFont> import_font_data_dynamic;
		import_font_data_dynamic.instantiate();
		ResourceFormatImporter::get_singleton()->add_importer(import_font_data_dynamic);

		Ref<ResourceImporterBMFont> import_font_data_bmfont;
		import_font_data_bmfont.instantiate();
		ResourceFormatImporter::get_singleton()->add_importer(import_font_data_bmfont);

		Ref<ResourceImporterImageFont> import_font_data_image;
		import_font_data_image.instantiate();
		ResourceFormatImporter::get_singleton()->add_importer(import_font_data_image);

		Ref<ResourceImporterCSVTranslation> import_csv_translation;
		import_csv_translation.instantiate();
		ResourceFormatImporter::get_singleton()->add_importer(import_csv_translation);

		Ref<ResourceImporterWAV> import_wav;
		import_wav.instantiate();
		ResourceFormatImporter::get_singleton()->add_importer(import_wav);

		Ref<ResourceImporterShaderFile> import_shader_file;
		import_shader_file.instantiate();
		ResourceFormatImporter::get_singleton()->add_importer(import_shader_file);

		Ref<ResourceImporterOBJ> import_obj;
		import_obj.instantiate();
		ResourceFormatImporter::get_singleton()->add_importer(import_obj);

		Ref<ResourceImporterScene> import_model_as_scene;
		import_model_as_scene.instantiate("PackedScene");
		ResourceFormatImporter::get_singleton()->add_importer(import_model_as_scene);

		Ref<ResourceImporterScene> import_model_as_animation;
		import_model_as_animation.instantiate("AnimationLibrary");
		ResourceFormatImporter::get_singleton()->add_importer(import_model_as_animation);

		Ref<ResourceImporterScene> import_scene_as_mesh_library = memnew(ResourceImporterScene("MeshLibrary"));
		ResourceFormatImporter::get_singleton()->add_importer(import_scene_as_mesh_library);

		Ref<ResourceImporterScene> import_scene_as_single_mesh = memnew(ResourceImporterScene("ArrayMesh"));
		ResourceFormatImporter::get_singleton()->add_importer(import_scene_as_single_mesh);

		{
			Ref<EditorSceneFormatImporterCollada> import_collada;
			import_collada.instantiate();
			ResourceImporterScene::add_scene_importer(import_collada);

			Ref<EditorOBJImporter> import_obj2;
			import_obj2.instantiate();
			ResourceImporterScene::add_scene_importer(import_obj2);

			Ref<EditorSceneFormatImporterESCN> import_escn;
			import_escn.instantiate();
			ResourceImporterScene::add_scene_importer(import_escn);
		}

		Ref<ResourceImporterBitMap> import_bitmap;
		import_bitmap.instantiate();
		ResourceFormatImporter::get_singleton()->add_importer(import_bitmap);
	}

	{
		Ref<EditorInspectorDefaultPlugin> eidp;
		eidp.instantiate();
		EditorInspector::add_inspector_plugin(eidp);

		Ref<EditorInspectorRootMotionPlugin> rmp;
		rmp.instantiate();
		EditorInspector::add_inspector_plugin(rmp);

		Ref<EditorInspectorParticleProcessMaterialPlugin> ppm;
		ppm.instantiate();
		EditorInspector::add_inspector_plugin(ppm);

		Ref<EditorInspectorPluginAudioStreamWAV> plugin;
		plugin.instantiate();
		EditorInspector::add_inspector_plugin(plugin);
	}

	_editor_selection = memnew(EditorSelection);

	EditorFileSystem *efs = memnew(EditorFileSystem);
	add_child(efs);

	EditorContextMenuPluginManager::create();

	// Used for previews.
	FileDialog::set_get_icon_callback(callable_mp_static(_file_dialog_get_icon));
	FileDialog::set_get_thumbnail_callback(callable_mp_static(_file_dialog_get_thumbnail));
	FileDialog::register_func = _file_dialog_register;
	FileDialog::unregister_func = _file_dialog_unregister;

	_editor_export = memnew(EditorExport);
	add_child(_editor_export);

	// Exporters might need the theme.
	EditorThemeManager::initialize();
	_theme = EditorThemeManager::generate_theme();
	DisplayServer::set_early_window_clear_color_override(true, _theme->get_color(SNAME("background"), EditorStringName(Editor)));

	EDITOR_DEF("_export_preset_advanced_mode", false); // Could be accessed in EditorExportPreset.

	register_exporters();

	ED_SHORTCUT("canvas_item_editor/pan_view", TTRC("Pan View"), Key::SPACE);

	const Vector<String> textfile_ext = ((String)(EDITOR_GET("docks/filesystem/textfile_extensions"))).split(",", false);
	for (const String &extension : textfile_ext) {
		_textfile_extensions.insert(extension);
	}
	const Vector<String> other_file_ext = ((String)(EDITOR_GET("docks/filesystem/other_file_extensions"))).split(",", false);
	for (const String &extension : other_file_ext) {
		_other_file_extensions.insert(extension);
	}

	_force_textfile_extensions.insert("csv"); // CSV translation source, has `Translation` resource type, but not loadable as resource.

	_resource_preview = memnew(EditorResourcePreview);
	add_child(_resource_preview);

	_progress_dialog = memnew(ProgressDialog);
	_progress_dialog->connect(SceneStringName(visibility_changed), callable_mp(this, &EditorNode::_progress_dialog_visibility_changed));
	add_child(_progress_dialog);

	_editor_overlay = memnew(Control);
	_editor_overlay->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	_editor_overlay->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	add_child(_editor_overlay);

	{
		_icon_manager = memnew(EditorIconManager);
		_editor_overlay->add_child(_icon_manager);
	}

	_editor_dock_manager = memnew(EditorDockManager);

	DockSplitContainer *dock_columns_split = nullptr;

	_main_layout_pc = memnew(PanelContainer);
	_main_layout_pc->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	add_child(_main_layout_pc);
	// Keep the main layout underneath floating docks and plugin controls.
	move_child(_main_layout_pc, _editor_overlay->get_index());

	{
		VBoxContainer *main_vbox = memnew(VBoxContainer);
		_main_layout_pc->add_child(main_vbox);

		{
			_editor_header = memnew(EditorHeaderContainer);
			main_vbox->add_child(_editor_header);

			_dock_layout_pc = memnew(PanelContainer);
			_dock_layout_pc->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			_dock_layout_pc->set_v_size_flags(Control::SIZE_EXPAND_FILL);
			main_vbox->add_child(_dock_layout_pc);

			{
				dock_columns_split = memnew(DockSplitContainer);
				dock_columns_split->set_name("DockColumns");
				dock_columns_split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
				_dock_layout_pc->add_child(dock_columns_split);

				{
					const EditorDock::DockSlot column_slots[5][2] = {
						{ EditorDock::DOCK_SLOT_LEFT_UL, EditorDock::DOCK_SLOT_LEFT_BL },
						{ EditorDock::DOCK_SLOT_LEFT_UR, EditorDock::DOCK_SLOT_LEFT_BR },
						{ EditorDock::CENTER_TOP, EditorDock::CENTER_BOTTOM },
						{ EditorDock::DOCK_SLOT_RIGHT_UL, EditorDock::DOCK_SLOT_RIGHT_BL },
						{ EditorDock::DOCK_SLOT_RIGHT_UR, EditorDock::DOCK_SLOT_RIGHT_BR },
					};
					for (int column = 0; column < 5; column++) {
						DockSplitContainer *dock_column_split = memnew(DockSplitContainer);
						dock_column_split->set_h_size_flags(column == 2 ? Control::SIZE_EXPAND_FILL : Control::SIZE_FILL);
						dock_column_split->set_name("DockColumn" + itos(column + 1));
						dock_column_split->set_vertical(true);
						dock_columns_split->add_child(dock_column_split);

						{
							for (int row = 0; row < 2; row++) {
								DockTabContainer *dock_tabs = memnew(SideDockTabContainer(column_slots[column][row], Rect2i(column * 2, row * 4, 2, 4)));
								dock_tabs->add_margin_valid_drop(row == 0 ? SIDE_BOTTOM : SIDE_TOP, column_slots[column][1 - row]);
								if (column > 0) {
									dock_tabs->add_margin_valid_drop(SIDE_LEFT, column_slots[column - 1][row]);
								}
								if (column < 4) {
									dock_tabs->add_margin_valid_drop(SIDE_RIGHT, column_slots[column + 1][row]);
								}
								if (column == 2) {
									dock_tabs->layout = EditorDock::DOCK_LAYOUT_HORIZONTAL;
									dock_tabs->set_custom_minimum_size(Size2(170, 80) * EDSCALE);
								}
								dock_tabs->set_name("DockSlot" + itos(column_slots[column][row]));
								dock_tabs->set_use_hidden_tabs_for_min_size(column != 2);
								dock_column_split->add_child(dock_tabs);

								_editor_dock_manager->register_dock_slot(dock_tabs);
							}
						}
						_editor_dock_manager->add_vsplit(dock_column_split);
						if (column == 2) {
							dock_column_split->set_split_offset(-270 * EDSCALE);
						}
					}
				}
			}

			_editor_dock_manager->set_main_hsplit(dock_columns_split);

			EditorFooterContainer *footer_container = memnew(EditorFooterContainer);
			main_vbox->add_child(footer_container);

			{
				_project_run_bar = footer_container->get_run_bar();
				_project_run_bar->connect("play_pressed", callable_mp(this, &EditorNode::_project_run_started));
			}
		}
	}

	_editor_layout_save_delay_timer = memnew(Timer);
	_editor_layout_save_delay_timer->connect("timeout", callable_mp(this, &EditorNode::_save_editor_layout));
	_editor_layout_save_delay_timer->set_one_shot(true);
	_editor_layout_save_delay_timer->set_wait_time(0.5);
	add_child(_editor_layout_save_delay_timer);

	_scan_changes_timer = memnew(Timer);
	_scan_changes_timer->connect("timeout", callable_mp(EditorFileSystem::get_singleton(), &EditorFileSystem::scan_changes));
	_scan_changes_timer->set_autostart(EDITOR_GET("interface/editor/behavior/import_resources_when_unfocused"));
	_scan_changes_timer->set_wait_time(0.5);
	add_child(_scan_changes_timer);

	_scene_editor_dock = memnew(SceneEditorDock);
	_editor_dock_manager->add_dock(_scene_editor_dock);

	{
		_scene_tabs = _scene_editor_dock->get_scene_tabs();
		_scene_tabs->connect("tab_changed", callable_mp(this, &EditorNode::_set_current_scene));
		_scene_tabs->connect("tab_closed", callable_mp(this, &EditorNode::_scene_tab_closed));
	}

	_edited_scene_viewport = memnew(SubViewport);
	_edited_scene_viewport->set_as_audio_listener_2d(true);
	_edited_scene_viewport->set_auto_translate_mode(AUTO_TRANSLATE_MODE_ALWAYS);
	_edited_scene_viewport->set_disable_3d(true);
	_edited_scene_viewport->set_disable_input(true);
	_edited_scene_viewport->set_embedding_subwindows(true);
	_edited_scene_viewport->set_translation_domain(StringName());

	_save_message_dialog = memnew(Dialog);
	_save_message_dialog->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::_menu_option).bind((int)MenuOptions::SCENE_SAVE_AS_SCENE));
	_save_message_dialog->set_unparent_when_invisible(true);

	_project_export_dialog = memnew(ProjectExportDialog);
	_editor_overlay->add_child(_project_export_dialog);

	_dependency_error_dialog = memnew(DependencyErrorDialog);
	_editor_overlay->add_child(_dependency_error_dialog);

	_editor_settings_dialog = memnew(EditorSettingsDialog);
	_editor_settings_dialog->connect("restart_requested", callable_mp(this, &EditorNode::restart_editor).bind(false));
	_editor_overlay->add_child(_editor_settings_dialog);

	EditorManagers *editor_managers = memnew(EditorManagers);
	add_child(editor_managers);

	EditorManagers::get_singleton()->get_project_plugin_manager()->connect("plugin_load_failed", callable_mp(this, &EditorNode::show_warning).bind(TTRC("Warning!")));

	_project_settings_editor = memnew(ProjectSettingsEditor(&_editor_data));
	_editor_overlay->add_child(_project_settings_editor);

	_scene_import_settings_dialog = memnew(SceneImportSettingsDialog);
	_editor_overlay->add_child(_scene_import_settings_dialog);

	_audio_stream_import_settings_dialog = memnew(AudioStreamImportSettingsDialog);
	_editor_overlay->add_child(_audio_stream_import_settings_dialog);

	_dynamic_font_import_settings_dialog = memnew(DynamicFontImportSettingsDialog);
	_editor_overlay->add_child(_dynamic_font_import_settings_dialog);

	_export_template_manager = memnew(ExportTemplateManager);
	_editor_overlay->add_child(_export_template_manager);

	_feature_profile_manager = memnew(EditorFeatureProfileManager);
	_feature_profile_manager->connect("current_feature_profile_changed", callable_mp(this, &EditorNode::_feature_profile_changed));
	_editor_overlay->add_child(_feature_profile_manager);

	_build_profile_manager = memnew(EditorBuildProfileManager);
	_editor_overlay->add_child(_build_profile_manager);

#if !defined(ANDROID_ENABLED)
	_fbx_importer_manager = memnew(FBXImporterManager);
	_editor_overlay->add_child(_fbx_importer_manager);
#endif

#ifndef ANDROID_ENABLED
	_android_sdk_manager = memnew(AndroidSDKManager);
	_editor_overlay->add_child(_android_sdk_manager);
#endif

	_warning_dialog = memnew(Dialog);
	_warning_dialog->add_button(TTRC("Copy Text"), true, "copy");
	_warning_dialog->connect("custom_action", callable_mp(this, &EditorNode::_copy_warning));
	_warning_dialog->set_unparent_when_invisible(true);

	_clear_cache_dialog = memnew(Dialog(true));
	_clear_cache_dialog->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::_clear_cache_confirmed));
	_clear_cache_dialog->set_autowrap(true);
	_clear_cache_dialog->set_ok_button_text(TTRC("Clear & Restart"));
	_clear_cache_dialog->set_text(TTRC("Clear all caches for the current project, including the cache for imported assets, exported assets, shaders, UIDs, scripts, and scene groups. New caches will be built upon restarting or exporting the project.\n\nNote that clearing the cache is almost always a workaround for a bug. If you can consistently reproduce the invalid cache or unexpected behavior that is solved by clearing the cache, please report this bug (\"Help > Report a Bug\") with reproduction steps."));
	_editor_overlay->add_child(_clear_cache_dialog);

	{
		_clear_cache_dialog->get_label()->set_custom_minimum_size(Vector2(600 * EDSCALE, 0));
	}

	// Command palette and editor shortcuts.
	_command_palette = EditorCommandPalette::get_singleton();
	_command_palette->set_title(TTR("Command Palette"));
	_editor_overlay->add_child(_command_palette);

	ED_SHORTCUT("editor/next_tab", TTRC("Next Scene Tab"), KeyModifierMask::CTRL + Key::TAB);
	ED_SHORTCUT("editor/prev_tab", TTRC("Previous Scene Tab"), KeyModifierMask::CTRL + KeyModifierMask::SHIFT + Key::TAB);
	ED_SHORTCUT("editor/filter_files", TTRC("Focus FileSystem Filter"), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::ALT + Key::P);

	ED_SHORTCUT_AND_COMMAND("editor/new_scene", TTRC("New Scene"), KeyModifierMask::CMD_OR_CTRL + Key::N);
	ED_SHORTCUT_AND_COMMAND("editor/new_inherited_scene", TTRC("New Inherited Scene..."), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::SHIFT + Key::N);
	ED_SHORTCUT_AND_COMMAND("editor/open_scene", TTRC("Open Scene..."), KeyModifierMask::CMD_OR_CTRL + Key::O);
	ED_SHORTCUT_AND_COMMAND("editor/reopen_closed_scene", TTRC("Reopen Closed Scene"), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::SHIFT + Key::T);

	ED_SHORTCUT_AND_COMMAND("editor/save_scene", TTRC("Save Scene"), KeyModifierMask::CMD_OR_CTRL + Key::S);
	ED_SHORTCUT_AND_COMMAND("editor/save_scene_as", TTRC("Save Scene As..."), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::SHIFT + Key::S);
	ED_SHORTCUT_AND_COMMAND("editor/save_all_scenes", TTRC("Save All Scenes"), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::SHIFT + KeyModifierMask::ALT + Key::S);

	ED_SHORTCUT_ARRAY_AND_COMMAND("editor/quick_open", TTRC("Quick Open..."), { int32_t(KeyModifierMask::SHIFT + KeyModifierMask::ALT + Key::O), int32_t(KeyModifierMask::CMD_OR_CTRL + Key::P) });
	ED_SHORTCUT_OVERRIDE_ARRAY("editor/quick_open", "macos", { int32_t(KeyModifierMask::META + KeyModifierMask::CTRL + Key::O), int32_t(KeyModifierMask::CMD_OR_CTRL + Key::P) });
	ED_SHORTCUT_AND_COMMAND("editor/quick_open_scene", TTRC("Quick Open Scene..."), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::SHIFT + Key::O);
	ED_SHORTCUT_AND_COMMAND("editor/quick_open_script", TTRC("Quick Open Script..."), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::ALT + Key::O);

	ED_SHORTCUT("editor/export_as_mesh_library", TTRC("MeshLibrary..."));

	ED_SHORTCUT_AND_COMMAND("editor/reload_saved_scene", TTRC("Reload Saved Scene"));
	ED_SHORTCUT_AND_COMMAND("editor/close_scene", TTRC("Close Scene"), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::SHIFT + Key::W);
	ED_SHORTCUT_AND_COMMAND("editor/close_all_scenes", TTRC("Close All Scenes"));
	ED_SHORTCUT_OVERRIDE("editor/close_scene", "macos", KeyModifierMask::CMD_OR_CTRL + Key::W);

	ED_SHORTCUT_AND_COMMAND("editor/file_quit", TTRC("Quit"), KeyModifierMask::CMD_OR_CTRL + Key::Q);

	ED_SHORTCUT_AND_COMMAND("editor/find_in_files", TTRC("Find in Files..."), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::F);
	ED_SHORTCUT_AND_COMMAND("editor/replace_in_files", TTRC("Replace in Files..."), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::R);

	ED_SHORTCUT_AND_COMMAND("editor/export", TTRC("Export..."), Key::NONE, TTRC("Export"));

	ED_SHORTCUT_AND_COMMAND("editor/orphan_resource_explorer", TTRC("Orphan Resource Explorer..."));
	ED_SHORTCUT_AND_COMMAND("editor/engine_compilation_configuration_editor", TTRC("Engine Compilation Configuration Editor..."));
	ED_SHORTCUT_AND_COMMAND("editor/clear_project_cache", TTRC("Clear Project Cache..."));

	ED_SHORTCUT_AND_COMMAND("editor/reload_current_project", TTRC("Reload Current Project"));
	ED_SHORTCUT_AND_COMMAND("editor/quit_to_project_list", TTRC("Quit to Project List"), KeyModifierMask::CTRL + KeyModifierMask::SHIFT + Key::Q);
	ED_SHORTCUT_OVERRIDE("editor/quit_to_project_list", "macos", KeyModifierMask::META + KeyModifierMask::CTRL + KeyModifierMask::ALT + Key::Q);

	ED_SHORTCUT_AND_COMMAND("editor/take_screenshot", TTRC("Take Screenshot"), KeyModifierMask::CTRL | Key::F12);
	ED_SHORTCUT_OVERRIDE("editor/take_screenshot", "macos", KeyModifierMask::META | Key::F12);

	// Use the Ctrl modifier so F2 can be used to rename nodes in the Hierarchy dock.
	ED_SHORTCUT_AND_COMMAND("editor/editor_2d", TTRC("Open 2D Workspace"), KeyModifierMask::CTRL | Key::F1);
	ED_SHORTCUT_AND_COMMAND("editor/editor_3d", TTRC("Open 3D Workspace"), KeyModifierMask::CTRL | Key::F2);
	ED_SHORTCUT_AND_COMMAND("editor/script_editor", TTRC("Open Script Editor"), KeyModifierMask::CTRL | Key::F3);
	ED_SHORTCUT_AND_COMMAND("editor/editor_game", TTRC("Open Game View"), KeyModifierMask::CTRL | Key::F4);

	ED_SHORTCUT_OVERRIDE("editor/editor_2d", "macos", KeyModifierMask::META | KeyModifierMask::CTRL | Key::KEY_1);
	ED_SHORTCUT_OVERRIDE("editor/editor_3d", "macos", KeyModifierMask::META | KeyModifierMask::CTRL | Key::KEY_2);
	ED_SHORTCUT_OVERRIDE("editor/script_editor", "macos", KeyModifierMask::META | KeyModifierMask::CTRL | Key::KEY_3);
	ED_SHORTCUT_OVERRIDE("editor/editor_game", "macos", KeyModifierMask::META | KeyModifierMask::CTRL | Key::KEY_4);

	// Editor menu and toolbar.
	bool can_expand = DisplayServer::get_singleton()->has_feature(DisplayServerEnums::FEATURE_EXTEND_TO_TITLE);

	if (can_expand) {
		// Add spacer to avoid other controls under window minimize/maximize/close buttons (left side).
		_left_menu_spacer = memnew(Control);
		_left_menu_spacer->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
		_editor_header->get_left_section()->add_child(_left_menu_spacer);
		_editor_header->get_left_section()->move_child(_left_menu_spacer, 0);
	}

	_file_menu = memnew(PopupMenu);
	_file_menu->connect(SceneStringName(id_pressed), callable_mp(this, &EditorNode::_menu_option));
	_file_menu->connect("about_to_popup", callable_mp(this, &EditorNode::_update_file_menu_opened));
	_add_to_main_menu(TTRC("File"), _file_menu);

	_project_menu = memnew(PopupMenu);
	_project_menu->connect(SceneStringName(id_pressed), callable_mp(this, &EditorNode::_menu_option));
	_add_to_main_menu(TTRC("Project"), _project_menu);

	_settings_menu = memnew(PopupMenu);
	_settings_menu->connect(SceneStringName(id_pressed), callable_mp(this, &EditorNode::_menu_option));
	_add_to_main_menu(TTRC("Editor"), _settings_menu);

	_update_main_menu_type();

	_project_title_lbl = memnew(Label);
	_project_title_lbl->add_theme_font_override(SceneStringName(font_family), _theme->get_font(SNAME("bold"), EditorStringName(Editor)));
	_project_title_lbl->add_theme_font_size_override(SceneStringName(font_size), _theme->get_font_size(SNAME("bold_size"), EditorStringName(Editor)));
	_project_title_lbl->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	_editor_header->get_center_section()->add_child(_project_title_lbl);

	_editor_header->add_editor_controls();

	_tool_menu = _editor_header->get_tools_menu();
	_tool_menu->connect("index_pressed", callable_mp(this, &EditorNode::_tool_menu_option));
	_tool_menu->add_shortcut(ED_GET_SHORTCUT("editor/orphan_resource_explorer"), TOOLS_ORPHAN_RESOURCES);
	_tool_menu->add_shortcut(ED_GET_SHORTCUT("editor/engine_compilation_configuration_editor"), TOOLS_BUILD_PROFILE_MANAGER);
	_tool_menu->add_shortcut(ED_GET_SHORTCUT("editor/clear_project_cache"), TOOLS_CLEAR_PROJECT_CACHE);

	_background_progress = memnew(BackgroundProgress);

	_editor_layout_dialog = memnew(EditorLayoutsDialog);
	_editor_layout_dialog->connect("name_confirmed", callable_mp(this, &EditorNode::_dialog_action));
	_editor_layout_dialog->set_hide_on_ok(false);
	_editor_layout_dialog->set_size(Size2(225, 270) * EDSCALE);
	_editor_overlay->add_child(_editor_layout_dialog);

	// Instantiate and place editor docks.

	memnew(HierarchyDock(_edited_scene_viewport, _editor_selection, _editor_data));
	_editor_dock_manager->add_dock(HierarchyDock::get_singleton());

	memnew(ImportDock);
	_editor_dock_manager->add_dock(ImportDock::get_singleton());

	FileSystemDock *filesystem_dock = memnew(FileSystemDock);
	filesystem_dock->connect("inherit", callable_mp(this, &EditorNode::_inherit_request));
	filesystem_dock->connect("instantiate", callable_mp(this, &EditorNode::_instantiate_request));
	filesystem_dock->connect("display_mode_changed", callable_mp(this, &EditorNode::_save_editor_layout));
	get_project_settings()->connect_filesystem_dock_signals(filesystem_dock);
	_editor_dock_manager->add_dock(filesystem_dock);

	memnew(InspectorDock(_editor_data));
	_editor_dock_manager->add_dock(InspectorDock::get_singleton());

	memnew(SignalsDock);
	_editor_dock_manager->add_dock(SignalsDock::get_singleton());

	memnew(GroupsDock);
	_editor_dock_manager->add_dock(GroupsDock::get_singleton());

	_history_dock = memnew(HistoryDock);
	_editor_dock_manager->add_dock(_history_dock);

	// Apply the initial offsets once the default docks determine which columns are visible.
	constexpr int DEFAULT_DOCK_OFFSET = 280;
	const int scaled_dock_offset = DEFAULT_DOCK_OFFSET * EDSCALE;
	dock_columns_split->set_split_offsets({ scaled_dock_offset, -scaled_dock_offset });

	_log = memnew(EditorLog);
	_editor_dock_manager->add_dock(_log);

	_native_shader_source_visualizer = memnew(EditorNativeShaderSourceVisualizer);
	_editor_overlay->add_child(_native_shader_source_visualizer);

	_orphan_resources_dialog = memnew(OrphanResourcesDialog);
	_editor_overlay->add_child(_orphan_resources_dialog);

	_confirmation_dialog = memnew(Dialog(true));
	_discard_changes_btn = _confirmation_dialog->add_button(TTRC("Don't Save"), false, "discard");
	_confirmation_dialog->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::_menu_confirm_current));
	_confirmation_dialog->connect("custom_action", callable_mp(this, &EditorNode::_discard_changes));
	_confirmation_dialog->connect("canceled", callable_mp(this, &EditorNode::_cancel_confirmation));
	_confirmation_dialog->set_min_size(Vector2(450.0 * EDSCALE, 0));
	_editor_overlay->add_child(_confirmation_dialog);

	_save_confirmation_dialog = memnew(Dialog(true));
	_save_confirmation_dialog->add_button(TTRC("Don't Save"), false, "discard");
	_save_confirmation_dialog->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::_menu_confirm_current));
	_save_confirmation_dialog->connect("custom_action", callable_mp(this, &EditorNode::_discard_changes));
	_save_confirmation_dialog->connect("canceled", callable_mp(this, &EditorNode::_cancel_close_scene_tab));
	_save_confirmation_dialog->connect("about_to_popup", callable_mp(this, &EditorNode::_prepare_save_confirmation_popup));
	_save_confirmation_dialog->set_min_size(Vector2(450.0 * EDSCALE, 0));
	_editor_overlay->add_child(_save_confirmation_dialog);

	_missing_android_build_template_dialog = memnew(Dialog(true));
	_missing_android_build_template_dialog->add_button(TTR("Install from file"))->connect(SceneStringName(pressed), callable_mp(this, &EditorNode::_android_install_build_template));
	_missing_android_build_template_dialog->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::_menu_option).bind(EDITOR_MANAGE_EXPORT_TEMPLATES));
	_missing_android_build_template_dialog->set_flag(Window::FLAG_RESIZE_DISABLED, true);
	_missing_android_build_template_dialog->set_ok_button_text(TTR("Manage Templates"));
	_missing_android_build_template_dialog->set_text(TTR("Android build template is missing, please install relevant templates."));
	_editor_overlay->add_child(_missing_android_build_template_dialog);

	_android_build_source_file_dialog = memnew(EditorFileDialog);
	_android_build_source_file_dialog->add_filter("*.zip");
	_android_build_source_file_dialog->connect("file_selected", callable_mp(this, &EditorNode::_android_build_source_selected));
	_android_build_source_file_dialog->set_access(EditorFileDialog::ACCESS_FILESYSTEM);
	_android_build_source_file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILE);
	_android_build_source_file_dialog->set_title(TTR("Select Android sources file"));
	_editor_overlay->add_child(_android_build_source_file_dialog);

	_install_android_build_template_dialog = memnew(Dialog(true));
	_install_android_build_template_dialog->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::_menu_confirm_current));
	_install_android_build_template_dialog->set_flag(Window::FLAG_RESIZE_DISABLED, true);
	_install_android_build_template_dialog->set_min_size(Vector2(500.0 * EDSCALE, 0));
	_install_android_build_template_dialog->set_ok_button_text(TTR("Install"));
	_editor_overlay->add_child(_install_android_build_template_dialog);

	{
		VBoxContainer *install_build_template_vbox = memnew(VBoxContainer);
		_install_android_build_template_dialog->add_child(install_build_template_vbox);

		{
			_install_android_build_template_lbl = memnew(Label);
			_install_android_build_template_lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
			_install_android_build_template_lbl->set_custom_minimum_size(Size2(300 * EDSCALE, 1));
			install_build_template_vbox->add_child(_install_android_build_template_lbl);

			_android_export_profile_btn = memnew(OptionButton);
			_android_export_profile_btn->connect(SceneStringName(item_selected), callable_mp(this, &EditorNode::_android_export_preset_selected));
			install_build_template_vbox->add_child(_android_export_profile_btn);
		}
	}

	_remove_android_build_template_dialog = memnew(Dialog(true));
	_remove_android_build_template_dialog->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::_android_explore_build_templates));
	_remove_android_build_template_dialog->set_autowrap(true);
	_remove_android_build_template_dialog->set_ok_button_text(OS::get_singleton()->get_platform_string(OS::PLATFORM_STRING_FILE_MANAGER_OPEN));
	_editor_overlay->add_child(_remove_android_build_template_dialog);

	_export_template_import_dialog = memnew(EditorFileDialog);
	_export_template_import_dialog->clear_filters();
	_export_template_import_dialog->add_filter("*.tpz", TTR("Template Package"));
	_export_template_import_dialog->connect("file_selected", callable_mp(this, &EditorNode::_dialog_action));
	_export_template_import_dialog->set_access(EditorFileDialog::ACCESS_FILESYSTEM);
	_export_template_import_dialog->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILE);
	_export_template_import_dialog->set_title(TTR("Import Templates From ZIP File"));
	_editor_overlay->add_child(_export_template_import_dialog);

	_scene_resource_file_dialog = memnew(EditorFileDialog);
	_scene_resource_file_dialog->connect("file_selected", callable_mp(this, &EditorNode::_dialog_action));
	_scene_resource_file_dialog->set_current_dir("res://");
	_scene_resource_file_dialog->set_transient_to_focused(true);
	_editor_overlay->add_child(_scene_resource_file_dialog);

	_mesh_library_export_dialog = memnew(EditorFileDialog);
	_mesh_library_export_dialog->add_option(TTR("Merge With Existing"), Vector<String>(), true);
	_mesh_library_export_dialog->add_option(TTR("Apply MeshInstance Transforms"), Vector<String>(), false);
	_mesh_library_export_dialog->connect("file_selected", callable_mp(this, &EditorNode::_dialog_action));
	_mesh_library_export_dialog->set_file_mode(EditorFileDialog::FILE_MODE_SAVE_FILE);
	_mesh_library_export_dialog->set_title(TTR("Export Library"));
	_editor_overlay->add_child(_mesh_library_export_dialog);

	_project_zip_export_dialog = memnew(EditorFileDialog);
	_project_zip_export_dialog->add_filter("*.zip", "ZIP Archive");
	_project_zip_export_dialog->connect("file_selected", callable_mp(this, &EditorNode::_dialog_action));
	_project_zip_export_dialog->set_access(EditorFileDialog::ACCESS_FILESYSTEM);
	_project_zip_export_dialog->set_file_mode(EditorFileDialog::FILE_MODE_SAVE_FILE);
	_project_zip_export_dialog->set_title(TTR("Pack Project as ZIP..."));
	_editor_overlay->add_child(_project_zip_export_dialog);

	_audio_preview_generator = memnew(AudioStreamPreviewGenerator);
	add_child(_audio_preview_generator);

	add_editor_plugin(memnew(DebuggerEditorPlugin(_editor_header->get_debug_menu())));

	_external_changes_dialog = memnew(Dialog(true));
	_external_changes_dialog->add_button(TTR("Ignore external changes"), true, "resave");
	_external_changes_dialog->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::_reload_modified_scenes));
	_external_changes_dialog->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::_reload_project_settings));
	_external_changes_dialog->connect("custom_action", callable_mp(this, &EditorNode::_resave_externally_modified_scenes));
	_external_changes_dialog->set_ok_button_text(TTR("Reload from disk"));
	_external_changes_dialog->set_title(TTR("Files have been modified outside Godot"));
	_editor_overlay->add_child(_external_changes_dialog);

	{
		VBoxContainer *disk_changes_vbox = memnew(VBoxContainer);
		_external_changes_dialog->add_child(disk_changes_vbox);

		{
			Label *disk_changes_lbl = memnew(Label);
			disk_changes_lbl->set_text(TTR("The following files are newer on disk:"));
			disk_changes_vbox->add_child(disk_changes_lbl);

			_external_changes_tree = memnew(Tree);
			_external_changes_tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
			disk_changes_vbox->add_child(_external_changes_tree);

			Label *disk_change_action_lbl = memnew(Label);
			disk_change_action_lbl->set_text(TTR("What action should be taken?"));
			disk_changes_vbox->add_child(disk_change_action_lbl);
		}
	}

	_missing_project_data_dialog = memnew(Dialog(true));
	_missing_project_data_dialog->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::restart_editor).bind(false));
	_missing_project_data_dialog->set_flag(Window::FLAG_RESIZE_DISABLED, true);
	_missing_project_data_dialog->set_ok_button_text(TTRC("Restart"));
	_missing_project_data_dialog->set_text(TTRC("Project data folder (.godot) is missing. Please restart editor."));
	_editor_overlay->add_child(_missing_project_data_dialog);

	add_editor_plugin(memnew(CanvasItemEditorPlugin));
	add_editor_plugin(memnew(Node3DEditorPlugin));
	memnew(FindInFiles);
	_editor_dock_manager->add_dock(memnew(ScriptEditorDock));

	if (!Engine::get_singleton()->is_recovery_mode_hint()) {
		add_editor_plugin(get_game_view_plugin());
	}

	EditorAudioBuses *audio_bus_editor = EditorAudioBuses::register_editor();

	ScriptTextEditor::register_editor(); // Register one for text scripts.
	TextEditor::register_editor();
	ShaderTextEditor::register_editor();

	// More visually meaningful to have this later.
	add_editor_plugin(memnew(AnimationPlayerEditorPlugin));
	add_editor_plugin(memnew(AnimationTrackKeyEditEditorPlugin));
	add_editor_plugin(memnew(AnimationMarkerKeyEditEditorPlugin));

	add_editor_plugin(VersionControlEditorPlugin::get_singleton());

	add_editor_plugin(memnew(AudioBusesEditorPlugin(audio_bus_editor)));

	for (int i = 0; i < EditorPlugins::get_plugin_count(); i++) {
		add_editor_plugin(EditorPlugins::create(i));
	}

	_resource_preview->add_preview_generator(Ref<EditorTexturePreviewPlugin>(memnew(EditorTexturePreviewPlugin)));
	_resource_preview->add_preview_generator(Ref<EditorImagePreviewPlugin>(memnew(EditorImagePreviewPlugin)));
	_resource_preview->add_preview_generator(Ref<EditorPackedScenePreviewPlugin>(memnew(EditorPackedScenePreviewPlugin)));
	_resource_preview->add_preview_generator(Ref<EditorMaterialPreviewPlugin>(memnew(EditorMaterialPreviewPlugin)));
	_resource_preview->add_preview_generator(Ref<EditorScriptPreviewPlugin>(memnew(EditorScriptPreviewPlugin)));
	_resource_preview->add_preview_generator(Ref<EditorAudioStreamPreviewPlugin>(memnew(EditorAudioStreamPreviewPlugin)));
	_resource_preview->add_preview_generator(Ref<EditorMeshPreviewPlugin>(memnew(EditorMeshPreviewPlugin)));
	_resource_preview->add_preview_generator(Ref<EditorBitmapPreviewPlugin>(memnew(EditorBitmapPreviewPlugin)));
	_resource_preview->add_preview_generator(Ref<EditorFontPreviewPlugin>(memnew(EditorFontPreviewPlugin)));
	_resource_preview->add_preview_generator(Ref<EditorGradientPreviewPlugin>(memnew(EditorGradientPreviewPlugin)));

	{
		Ref<CanvasItemMaterialConversionPlugin> canvas_item_mat_convert;
		canvas_item_mat_convert.instantiate();
		_resource_conversion_plugins.push_back(canvas_item_mat_convert);

		Ref<BlitMaterialConversionPlugin> blit_mat_convert;
		blit_mat_convert.instantiate();
		_resource_conversion_plugins.push_back(blit_mat_convert);

		Ref<ParticleProcessMaterialConversionPlugin> particles_mat_convert;
		particles_mat_convert.instantiate();
		_resource_conversion_plugins.push_back(particles_mat_convert);

		Ref<StandardMaterial3DConversionPlugin> spatial_mat_convert;
		spatial_mat_convert.instantiate();
		_resource_conversion_plugins.push_back(spatial_mat_convert);

		Ref<ORMMaterial3DConversionPlugin> orm_mat_convert;
		orm_mat_convert.instantiate();
		_resource_conversion_plugins.push_back(orm_mat_convert);

		Ref<ProceduralSkyMaterialConversionPlugin> procedural_sky_mat_convert;
		procedural_sky_mat_convert.instantiate();
		_resource_conversion_plugins.push_back(procedural_sky_mat_convert);

		Ref<PanoramaSkyMaterialConversionPlugin> panorama_sky_mat_convert;
		panorama_sky_mat_convert.instantiate();
		_resource_conversion_plugins.push_back(panorama_sky_mat_convert);

		Ref<PhysicalSkyMaterialConversionPlugin> physical_sky_mat_convert;
		physical_sky_mat_convert.instantiate();
		_resource_conversion_plugins.push_back(physical_sky_mat_convert);

		Ref<FogMaterialConversionPlugin> fog_mat_convert;
		fog_mat_convert.instantiate();
		_resource_conversion_plugins.push_back(fog_mat_convert);
	}

	_editor_plugins_over = memnew(EditorPluginList);
	_editor_plugins_force_over = memnew(EditorPluginList);
	_editor_plugins_force_input_forwarding = memnew(EditorPluginList);

	Ref<DedicatedServerExportPlugin> dedicated_server_export_plugin;
	dedicated_server_export_plugin.instantiate();

	EditorExport::get_singleton()->add_export_plugin(dedicated_server_export_plugin);

#ifdef RD_ENABLED
	Ref<ShaderBakerExportPlugin> shader_baker_export_plugin;
	shader_baker_export_plugin.instantiate();

#ifdef VULKAN_ENABLED
	Ref<ShaderBakerExportPluginPlatformVulkan> shader_baker_export_plugin_platform_vulkan;
	shader_baker_export_plugin_platform_vulkan.instantiate();
	shader_baker_export_plugin->add_platform(shader_baker_export_plugin_platform_vulkan);
#endif

#ifdef D3D12_ENABLED
	Ref<ShaderBakerExportPluginPlatformD3D12> shader_baker_export_plugin_platform_d3d12;
	shader_baker_export_plugin_platform_d3d12.instantiate();
	shader_baker_export_plugin->add_platform(shader_baker_export_plugin_platform_d3d12);
#endif

#ifdef METAL_ENABLED
	Ref<ShaderBakerExportPluginPlatformMetal> shader_baker_export_plugin_platform_metal;
	shader_baker_export_plugin_platform_metal.instantiate();
	shader_baker_export_plugin->add_platform(shader_baker_export_plugin_platform_metal);
#endif

	EditorExport::get_singleton()->add_export_plugin(shader_baker_export_plugin);
#endif // RD_ENABLED

	Ref<PackedSceneEditorTranslationParserPlugin> packed_scene_translation_parser_plugin;
	packed_scene_translation_parser_plugin.instantiate();
	EditorTranslationParser::get_singleton()->add_parser(packed_scene_translation_parser_plugin, EditorTranslationParser::STANDARD);

	_edit_current();
	_saving_resource = Ref<Resource>();

	set_process(true);

	_open_imported_dialog = memnew(Dialog(true));
	_new_inherited_scene_btn = _open_imported_dialog->add_button(TTR("New Inherited"), true, "inherit");
	_open_imported_dialog->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::_open_imported));
	_open_imported_dialog->connect("custom_action", callable_mp(this, &EditorNode::_inherit_imported));
	_open_imported_dialog->set_flag(Window::FLAG_RESIZE_DISABLED, true);
	_open_imported_dialog->set_ok_button_text(TTR("Open Anyway"));
	_editor_overlay->add_child(_open_imported_dialog);

	_quick_open_dialog = memnew(EditorQuickOpenDialog);
	_editor_overlay->add_child(_quick_open_dialog);

	_quick_open_color_palette = memnew(EditorQuickOpenDialog);
	_editor_overlay->add_child(_quick_open_color_palette);

	_update_recent_scenes();

	set_process_shortcut_input(true);

	_load_error_dialog = memnew(Dialog);
	_load_error_dialog->connect(SceneStringName(visibility_changed), callable_mp(this, &EditorNode::_load_error_dialog_visibility_changed));
	_load_error_dialog->set_title(TTR("Load Errors"));
	_load_error_dialog->set_unparent_when_invisible(true);

	{
		_load_error_log = memnew(RichTextLabel);
		_load_error_dialog->add_child(_load_error_log);
	}

	_execute_output_dialog = memnew(Dialog);
	_execute_output_dialog->set_title("");
	_execute_output_dialog->set_unparent_when_invisible(true);

	{
		_process_output_log = memnew(RichTextLabel);
		_process_output_log->set_context_menu_enabled(true);
		_process_output_log->set_selection_enabled(true);
		_execute_output_dialog->add_child(_process_output_log);
	}

	EditorFileSystem::get_singleton()->connect("sources_changed", callable_mp(this, &EditorNode::_sources_changed));
	EditorFileSystem::get_singleton()->connect("filesystem_changed", callable_mp(this, &EditorNode::_filesystem_changed));
	EditorFileSystem::get_singleton()->connect("resources_reimporting", callable_mp(this, &EditorNode::_resources_reimporting));
	EditorFileSystem::get_singleton()->connect("resources_reimported", callable_mp(this, &EditorNode::_resources_reimported));
	EditorFileSystem::get_singleton()->connect("resources_reload", callable_mp(this, &EditorNode::_resources_changed));

	_build_icon_type_cache();

	_pick_main_scene_dialog = memnew(Dialog(true));
	_select_current_scene_btn = _pick_main_scene_dialog->add_button(TTR("Select Current"), true, "select_current");
	_pick_main_scene_dialog->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::_menu_option).bind(SETTINGS_PICK_MAIN_SCENE));
	_pick_main_scene_dialog->connect("custom_action", callable_mp(this, &EditorNode::_pick_main_scene_custom_action));
	_pick_main_scene_dialog->set_ok_button_text(TTR("Select"));
	_editor_overlay->add_child(_pick_main_scene_dialog);

	_open_project_settings_dialog = memnew(Dialog(true));
	_open_project_settings_dialog->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::_menu_option).bind(PROJECT_OPEN_SETTINGS));
	_open_project_settings_dialog->set_ok_button_text(TTRC("Open Project Settings"));
	_editor_overlay->add_child(_open_project_settings_dialog);

	for (int i = 0; i < _init_callbacks.size(); i++) {
		_init_callbacks[i]();
	}

	_editor_data.add_edited_scene(-1);
	_editor_data.set_edited_scene(0);
	_scene_tabs->update_scene_tabs();

	ImportDock::get_singleton()->initialize_import_options();

	FileAccess::set_file_close_fail_notify_callback(_file_access_close_error_notify);

	_print_handler_registration.printfunc = _print_handler;
	_print_handler_registration.userdata = this;
	add_print_handler(&_print_handler_registration);

	ResourceSaver::set_save_callback(_resource_saved);
	ResourceLoader::set_load_callback(_resource_loaded);

	// Apply setting presets in case the editor_settings file is missing values.
	EditorSettingsDialog::update_3d_navigation_preset();

	_screenshot_timer = memnew(Timer);
	_screenshot_timer->connect("timeout", callable_mp(this, &EditorNode::_request_screenshot));
	_screenshot_timer->set_one_shot(true);
	_screenshot_timer->set_wait_time(_settings_menu->get_submenu_popup_delay() + 0.1f);
	add_child(_screenshot_timer);
	_screenshot_timer->set_owner(get_owner());

	// Extend menu bar to window title.
	if (can_expand) {
		DisplayServer::get_singleton()->process_events();
		DisplayServer::get_singleton()->window_set_flag(DisplayServerEnums::WINDOW_FLAG_EXTEND_TO_TITLE, true, DisplayServerEnums::MAIN_WINDOW_ID);
		_editor_header->set_can_move_window(true);
	}

	{
		const String exec = OS::get_singleton()->get_executable_path();
		const String old_exec = EditorSettings::get_singleton()->get_project_metadata("editor_metadata", "executable_path", "");
		// Save editor executable path for third-party tools.
		if (exec != old_exec) {
			EditorSettings::get_singleton()->set_project_metadata("editor_metadata", "executable_path", exec);
		}
	}

	_follow_system_theme = EDITOR_GET("interface/theme/follow_system_theme");
	_use_system_accent_color = EDITOR_GET("interface/theme/use_system_accent_color");

	// Define the default layout after everything is initialized.

	_default_layout.instantiate();
	const String docks_section = "docks";
	{
		const String scene_key = HierarchyDock::get_singleton()->get_effective_layout_key();
		const String import_key = ImportDock::get_singleton()->get_effective_layout_key();
		_default_layout->set_value(docks_section, DockTabContainer::get_config_key(EditorDock::DOCK_SLOT_LEFT_UR), vformat("%s,%s", scene_key, import_key));
		_default_layout->set_value(docks_section, DockTabContainer::get_config_key(EditorDock::DOCK_SLOT_LEFT_UR) + "_selected_tab_idx", 0);
	}
	{
		const String filesystem_key = filesystem_dock->get_effective_layout_key();
		const String history_key = _history_dock->get_effective_layout_key();
		_default_layout->set_value(docks_section, DockTabContainer::get_config_key(EditorDock::DOCK_SLOT_LEFT_BR), vformat("%s,%s", filesystem_key, history_key));
		_default_layout->set_value(docks_section, DockTabContainer::get_config_key(EditorDock::DOCK_SLOT_LEFT_BR) + "_selected_tab_idx", 0);
	}
	{
		const String inspector_key = InspectorDock::get_singleton()->get_effective_layout_key();
		const String signals_key = SignalsDock::get_singleton()->get_effective_layout_key();
		const String groups_key = GroupsDock::get_singleton()->get_effective_layout_key();
		_default_layout->set_value(docks_section, DockTabContainer::get_config_key(EditorDock::DOCK_SLOT_RIGHT_UL), vformat("%s,%s,%s", inspector_key, signals_key, groups_key));
		_default_layout->set_value(docks_section, DockTabContainer::get_config_key(EditorDock::DOCK_SLOT_RIGHT_UL) + "_selected_tab_idx", 0);
	}
	{
		const String scene_editor_key = _scene_editor_dock->get_effective_layout_key();
		const String script_key = ScriptEditorDock::get_singleton()->get_effective_layout_key();
		String game_key;
		if (GameView::get_dock()) {
			game_key = GameView::get_dock()->get_effective_layout_key();
		}
		_default_layout->set_value(docks_section, DockTabContainer::get_config_key(EditorDock::CENTER_TOP), vformat("%s,%s,%s", scene_editor_key, script_key, game_key));
		_default_layout->set_value(docks_section, DockTabContainer::get_config_key(EditorDock::CENTER_TOP) + "_selected_tab_idx", 0);
	}

	const int horizontal_split_offsets[] = { 0, DEFAULT_DOCK_OFFSET, -DEFAULT_DOCK_OFFSET, 0 };
	for (int i = 0; i < static_cast<int>(std_size(horizontal_split_offsets)); i++) {
		_default_layout->set_value(docks_section, "dock_column_offset_" + itos(i + 1), horizontal_split_offsets[i]);
	}
	for (int i = 0; i < _editor_dock_manager->get_vsplit_count(); i++) {
		_default_layout->set_value(docks_section, "dock_column_split_" + itos(i + 1), i == 2 ? -270 * EDSCALE : 0);
	}

	_update_layouts_menu();
}

EditorNode::~EditorNode() {
	EditorInspector::cleanup_plugins();
	EditorTranslationParser::get_singleton()->clean_parsers();
	ResourceImporterScene::clean_up_importer_plugins();
	EditorContextMenuPluginManager::cleanup();

	remove_print_handler(&_print_handler_registration);
	EditorHelp::cleanup_doc();
#if defined(MODULE_GDSCRIPT_ENABLED)
	EditorHelpHighlighter::free_singleton();
#endif
	memdelete(_editor_selection);
	memdelete(_editor_plugins_over);
	memdelete(_editor_plugins_force_over);
	memdelete(_editor_plugins_force_input_forwarding);
	memdelete(_background_progress);
	if (FindInFiles *find_in_files = FindInFiles::get_singleton()) {
		memdelete(find_in_files);
	}
	memdelete(_editor_dock_manager);

	EditorSettings::destroy();
	EditorThemeManager::finalize();

	FileDialog::register_func = nullptr;
	FileDialog::unregister_func = nullptr;

	_file_dialogs.clear();

	_singleton = nullptr;
}

String EditorNode::adjust_scene_name_casing(const String &p_root_name) {
	switch (static_cast<SceneNameCasing>(int(GLOBAL_GET("editor/naming/scene_name_casing")))) {
		case SceneNameCasing::AUTO:
			// Use casing of the root node.
			break;
		case SceneNameCasing::PASCAL_CASE:
			return p_root_name.to_pascal_case();
		case SceneNameCasing::SNAKE_CASE:
			return p_root_name.to_snake_case();
		case SceneNameCasing::KEBAB_CASE:
			return p_root_name.to_kebab_case();
		case SceneNameCasing::CAMEL_CASE:
			return p_root_name.to_camel_case();
	}
	return p_root_name;
}

String EditorNode::adjust_script_name_casing(const String &p_file_name) {
	const int editor_casing = GLOBAL_GET("editor/naming/script_name_casing");

	switch (editor_casing) {
		case ScriptLanguage::SCRIPT_NAME_CASING_PASCAL_CASE:
			return p_file_name.to_pascal_case();
		case ScriptLanguage::SCRIPT_NAME_CASING_AUTO:
		case ScriptLanguage::SCRIPT_NAME_CASING_SNAKE_CASE:
			return p_file_name.to_snake_case();
		case ScriptLanguage::SCRIPT_NAME_CASING_KEBAB_CASE:
			return p_file_name.to_kebab_case();
		case ScriptLanguage::SCRIPT_NAME_CASING_CAMEL_CASE:
			return p_file_name.to_camel_case();
	}
	return p_file_name;
}

void EditorNode::disambiguate_filenames(const Vector<String> p_full_paths, Vector<String> &r_filenames) {
	ERR_FAIL_COND_MSG(p_full_paths.size() != r_filenames.size(), vformat("disambiguate_filenames requires two string vectors of same length (%d != %d).", p_full_paths.size(), r_filenames.size()));

	// Keep track of a list of "index sets," i.e. sets of indices
	// within disambiguated_scene_names which contain the same name.
	Vector<RBSet<int>> index_sets;
	HashMap<String, int> scene_name_to_set_index;
	for (int i = 0; i < r_filenames.size(); i++) {
		const String &scene_name = r_filenames[i];
		if (!scene_name_to_set_index.has(scene_name)) {
			index_sets.append(RBSet<int>());
			scene_name_to_set_index.insert(r_filenames[i], index_sets.size() - 1);
		}
		index_sets.write[scene_name_to_set_index[scene_name]].insert(i);
	}

	// For each index set with a size > 1, we need to disambiguate.
	for (int i = 0; i < index_sets.size(); i++) {
		RBSet<int> iset(index_sets[i]);
		while (iset.size() > 1) {
			// Append the parent folder to each scene name.
			for (const int &scene_index : iset) {
				int set_idx = scene_index;
				String scene_name = r_filenames[set_idx];
				String full_path = p_full_paths[set_idx];

				// Get rid of file extensions and res:// prefixes.
				scene_name = scene_name.get_basename();
				if (full_path.begins_with("res://")) {
					full_path = full_path.substr(6);
				}
				full_path = full_path.get_basename();

				// Normalize trailing slashes when normalizing directory names.
				scene_name = scene_name.trim_suffix("/");
				full_path = full_path.trim_suffix("/");

				int scene_name_size = scene_name.size();
				int full_path_size = full_path.size();
				int difference = full_path_size - scene_name_size;

				// Find just the parent folder of the current path and append it.
				// If the current name is foo.tscn, and the full path is /some/folder/foo.tscn
				// then slash_idx is the second '/', so that we select just "folder", and
				// append that to yield "folder/foo.tscn".
				if (difference > 0) {
					String parent = full_path.substr(0, difference);
					int slash_idx = parent.rfind_char('/');
					slash_idx = parent.rfind_char('/', slash_idx - 1);
					parent = (slash_idx >= 0 && parent.length() > 1) ? parent.substr(slash_idx + 1) : parent;
					r_filenames.write[set_idx] = parent + r_filenames[set_idx];
				}
			}

			// Loop back through scene names and remove non-ambiguous names.
			bool can_proceed = false;
			RBSet<int>::Element *base_scene_entry = iset.front();
			while (base_scene_entry) {
				String scene_name = r_filenames[base_scene_entry->get()];
				bool duplicate_found = false;
				for (const int &scene_index : iset) {
					if (base_scene_entry->get() == scene_index) {
						continue;
					}
					const String &other_scene_name = r_filenames[scene_index];
					if (other_scene_name == scene_name) {
						duplicate_found = true;
						break;
					}
				}

				RBSet<int>::Element *to_erase = duplicate_found ? nullptr : base_scene_entry;

				// We need to check that we could actually append anymore names
				// if we wanted to for disambiguation. If we can't, then we have
				// to abort even with ambiguous names. We clean the full path
				// and the scene name first to remove extensions so that this
				// comparison actually works.
				String path = p_full_paths[base_scene_entry->get()];

				// Get rid of file extensions and res:// prefixes.
				scene_name = scene_name.get_basename();
				if (path.begins_with("res://")) {
					path = path.substr(6);
				}
				path = path.get_basename();

				// Normalize trailing slashes when normalizing directory names.
				scene_name = scene_name.trim_suffix("/");
				path = path.trim_suffix("/");

				// We can proceed if the full path is longer than the scene name,
				// meaning that there is at least one more parent folder we can
				// tack onto the name.
				can_proceed = can_proceed || (path.size() - scene_name.size()) >= 1;

				base_scene_entry = base_scene_entry->next();
				if (to_erase) {
					iset.erase(to_erase);
				}
			}

			if (!can_proceed) {
				break;
			}
		}
	}
}

void EditorNode::add_io_error(const String &p_error) {
	DEV_ASSERT(Thread::get_caller_id() == Thread::get_main_id());
	_singleton->_load_error_log->add_image(_singleton->_theme->get_icon(SNAME("status_error"), EditorStringName(Editor)));
	_singleton->_load_error_log->add_text(p_error + "\n");
	// When a progress dialog is displayed, we will wait for it ot close before displaying
	// the io errors to prevent the io popup to set it's parent to the progress dialog.
	if (_singleton->_progress_dialog->is_visible()) {
		_singleton->_load_errors_queued_to_display = true;
	} else {
		EditorInterface::get_singleton()->popup_dialog_centered_ratio(_singleton->_load_error_dialog, 0.5);
	}
}

void EditorNode::add_io_warning(const String &p_warning) {
	DEV_ASSERT(Thread::get_caller_id() == Thread::get_main_id());
	_singleton->_load_error_log->add_image(_singleton->_theme->get_icon(SNAME("status_warning"), EditorStringName(Editor)));
	_singleton->_load_error_log->add_text(p_warning + "\n");
	// When a progress dialog is displayed, we will wait for it ot close before displaying
	// the io errors to prevent the io popup to set it's parent to the progress dialog.
	if (_singleton->_progress_dialog->is_visible()) {
		_singleton->_load_errors_queued_to_display = true;
	} else {
		EditorInterface::get_singleton()->popup_dialog_centered_ratio(_singleton->_load_error_dialog, 0.5);
	}
}

bool EditorNode::find_recursive_resources(const Variant &p_variant, HashSet<Resource *> &r_resources_found) {
	switch (p_variant.get_type()) {
		case Variant::ARRAY: {
			Array a = p_variant;
			for (int i = 0; i < a.size(); i++) {
				Variant v2 = a[i];
				if (v2.get_type() != Variant::ARRAY && v2.get_type() != Variant::DICTIONARY && v2.get_type() != Variant::OBJECT) {
					continue;
				}
				if (find_recursive_resources(v2, r_resources_found)) {
					return true;
				}
			}
		} break;
		case Variant::DICTIONARY: {
			Dictionary d = p_variant;
			for (const KeyValue<Variant, Variant> &kv : d) {
				const Variant &k = kv.key;
				const Variant &v2 = kv.value;
				if (k.get_type() == Variant::ARRAY || k.get_type() == Variant::DICTIONARY || k.get_type() == Variant::OBJECT) {
					if (find_recursive_resources(k, r_resources_found)) {
						return true;
					}
				}
				if (v2.get_type() == Variant::ARRAY || v2.get_type() == Variant::DICTIONARY || v2.get_type() == Variant::OBJECT) {
					if (find_recursive_resources(v2, r_resources_found)) {
						return true;
					}
				}
			}
		} break;
		case Variant::OBJECT: {
			Ref<Resource> r = p_variant;

			if (r.is_null()) {
				return false;
			}

			if (r_resources_found.has(r.ptr())) {
				return true;
			}

			r_resources_found.insert(r.ptr());

			List<PropertyInfo> plist;
			r->get_property_list(&plist);
			for (const PropertyInfo &pinfo : plist) {
				if (!(pinfo.usage & PROPERTY_USAGE_STORAGE)) {
					continue;
				}

				if (pinfo.type != Variant::ARRAY && pinfo.type != Variant::DICTIONARY && pinfo.type != Variant::OBJECT) {
					continue;
				}
				if (find_recursive_resources(r->get(pinfo.name), r_resources_found)) {
					return true;
				}
			}

			r_resources_found.erase(r.ptr());
		} break;
		default: {
		}
	}
	return false;
}

void EditorNode::progress_add_task(const String &p_task, const String &p_label, int p_steps, bool p_can_cancel) {
	if (!_singleton) {
		return;
	} else if (_singleton->_cmdline_mode) {
		print_line_rich(vformat("[   0%% ] [color=gray][b]%s[/b] | Started %s (%d steps)[/color]", p_task, p_label, p_steps));
		progress_total_steps[p_task] = p_steps;
	} else if (_singleton->_progress_dialog) {
		_singleton->_progress_dialog->add_task(p_task, p_label, p_steps, p_can_cancel);
	}
}

bool EditorNode::progress_task_step(const String &p_task, const String &p_state, int p_step, bool p_force_refresh) {
	if (!_singleton) {
		return false;
	} else if (_singleton->_cmdline_mode) {
		double current_time = USEC_TO_SEC(OS::get_singleton()->get_ticks_usec());
		double elapsed_time = current_time - last_progress_time;
		if (p_task != last_progress_task || p_state != last_progress_state || p_step != last_progress_step || elapsed_time >= 1.0) {
			// Only print the progress if it's changed since the last print, or if one second has passed.
			// This prevents multithreaded import from printing the same progress too often, which would bloat the log file.
			const int percent = (p_step / float(progress_total_steps[p_task] + 1)) * 100;
			print_line_rich(vformat("[%4d%% ] [color=gray][b]%s[/b] | %s[/color]", percent, p_task, p_state));
			last_progress_task = p_task;
			last_progress_state = p_state;
			last_progress_step = p_step;
			last_progress_time = current_time;
		}
		return false;
	} else if (_singleton->_progress_dialog) {
		return _singleton->_progress_dialog->task_step(p_task, p_state, p_step, p_force_refresh);
	} else {
		return false;
	}
}

void EditorNode::progress_end_task(const String &p_task) {
	if (!_singleton) {
		return;
	} else if (_singleton->_cmdline_mode) {
		progress_total_steps.erase(p_task);
		print_line_rich(vformat("[color=green][ DONE ][/color] [b]%s[/b]\n", p_task));
	} else if (_singleton->_progress_dialog) {
		_singleton->_progress_dialog->end_task(p_task);
	}
}

void EditorNode::progress_add_task_bg(const String &p_task, const String &p_label, int p_steps) {
	_singleton->_background_progress->add_task(p_task, p_label, p_steps);
}

void EditorNode::progress_task_step_bg(const String &p_task, int p_step) {
	_singleton->_background_progress->task_step(p_task, p_step);
}

void EditorNode::progress_end_task_bg(const String &p_task) {
	_singleton->_background_progress->end_task(p_task);
}

void EditorNode::add_editor_plugin(EditorPlugin *p_editor, bool p_config_changed) {
	_singleton->_editor_data.add_editor_plugin(p_editor);
	_singleton->add_child(p_editor);
	if (p_config_changed) {
		p_editor->enable_plugin();
	}
}

void EditorNode::remove_editor_plugin(EditorPlugin *r_editor, bool p_config_changed) {
	r_editor->make_visible(false);
	r_editor->clear();
	if (p_config_changed) {
		r_editor->disable_plugin();
	}
	_singleton->_editor_plugins_over->remove_plugin(r_editor);
	_singleton->_editor_plugins_force_over->remove_plugin(r_editor);
	_singleton->_editor_plugins_force_input_forwarding->remove_plugin(r_editor);
	_singleton->remove_child(r_editor);
	_singleton->_editor_data.remove_editor_plugin(r_editor);

	for (KeyValue<ObjectID, HashSet<EditorPlugin *>> &kv : _singleton->_active_plugins) {
		kv.value.erase(r_editor);
	}
}

bool EditorNode::immediate_confirmation_dialog(const String &p_text, const String &p_ok_text, const String &p_cancel_text, uint32_t p_wrap_width) {
	_singleton->_immediate_dialog_was_confirmed = false;

	Dialog *dialog = memnew(Dialog(true));
	dialog->connect(SceneStringName(confirmed), callable_mp(_singleton, &EditorNode::_immediate_dialog_confirmed));
	if (p_wrap_width > 0) {
		dialog->set_autowrap(true);
		dialog->get_label()->set_custom_minimum_size(Size2(p_wrap_width, 0) * EDSCALE);
	}
	dialog->set_cancel_button_text(p_cancel_text);
	dialog->set_flag(Window::FLAG_RESIZE_DISABLED, true);
	dialog->set_ok_button_text(p_ok_text);
	dialog->set_text(p_text);
	_singleton->_editor_overlay->add_child(dialog);

	dialog->popup_centered();

	while (true) {
		DisplayServer::get_singleton()->process_events();
		Main::iteration();
		if (_singleton->_immediate_dialog_was_confirmed || !dialog->is_visible()) {
			break;
		}
	}

	memdelete(dialog);
	return _singleton->_immediate_dialog_was_confirmed;
}

bool EditorNode::is_cmdline_mode() {
	ERR_FAIL_NULL_V(_singleton, false);
	return _singleton->_cmdline_mode;
}

// Returns the list of project settings to add to new projects. This is used by the
// hub creation dialog, but also applies to empty `project.godot` files
// to cover the command line workflow of creating projects using `touch project.godot`.
//
// This is used to set better defaults for new projects without affecting existing projects.
// Keep the list alphabetically sorted.
HashMap<String, Variant> EditorNode::get_initial_settings() {
	HashMap<String, Variant> settings;
	settings["display/window/stretch/aspect"] = "expand";
	settings["display/window/stretch/mode"] = "canvas_items";
	settings["input_devices/joypads/ignore_joypad_on_unfocused_application"] = true;
	settings["physics/3d/physics_engine"] = PhysicsServer3DManager::JOLT_PHYSICS_NAME;
	settings["rendering/rendering_device/driver.windows"] = "d3d12";
	settings["rendering/lights_and_shadows/multi_bounce_occlusion/enabled"] = true;
	return settings;
}

void EditorNode::cleanup() {
	_init_callbacks.clear();
}

bool EditorNode::call_build() {
	const bool builds_successful = _editor_data.call_build();

	if (!builds_successful) {
		ERR_PRINT("An EditorPlugin build callback failed.");
	}

	return builds_successful;
}

void EditorNode::call_run_scene(const String &p_scene, Vector<String> &r_args) {
	for (int i = 0; i < _editor_data.get_editor_plugin_count(); i++) {
		EditorPlugin *plugin = _editor_data.get_editor_plugin(i);
		plugin->run_scene(p_scene, r_args);
	}
}

void EditorNode::trigger_menu_option(int p_option, bool p_confirmed) {
	_menu_option_confirm(p_option, p_confirmed);
}

bool EditorNode::has_previous_closed_scenes() const {
	return !_prev_closed_scenes.is_empty();
}

void EditorNode::edit_node(Node *p_node) {
	push_item(p_node);
}

void EditorNode::edit_resource(const Ref<Resource> &p_resource) {
	InspectorDock::get_singleton()->edit_resource(p_resource);
}

void EditorNode::save_resource_in_path(const Ref<Resource> &r_resource, const String &p_path) {
	_editor_data.apply_changes_in_editors();

	if (_saving_resources_in_path.has(r_resource)) {
		return;
	}
	_saving_resources_in_path.insert(r_resource);

	int flg = 0;
	if (EDITOR_GET("filesystem/on_save/compress_binary_resources")) {
		flg |= ResourceSaver::FLAG_COMPRESS;
	}

	String path = ProjectSettings::get_singleton()->localize_path(p_path);
	Error err = ResourceSaver::save(r_resource, path, flg | ResourceSaver::FLAG_REPLACE_SUBRESOURCE_PATHS);

	if (err != OK) {
		if (ResourceLoader::is_imported(r_resource->get_path())) {
			show_warning(vformat(TTR("Cannot save resource at path \"%s\", as it is imported.\nImported resources can't be saved. Instead, modify the source file or change options in the Import dock."), path));
		} else {
			show_warning(vformat(TTR("Error saving resource at path \"%s\": %s."), path, TTR(error_names[err])));
		}

		_saving_resources_in_path.erase(r_resource);
		return;
	}

	Ref<Resource> prev_resource = ResourceCache::get_ref(p_path);
	if (prev_resource.is_null() || prev_resource != r_resource) {
		r_resource->set_path(path, true);
	}
	if (prev_resource.is_valid() && prev_resource != r_resource) {
		replace_resources_in_scenes({ prev_resource }, { r_resource });
	}
	_saving_resources_in_path.erase(r_resource);

	_resource_saved(r_resource, path);
	clear_node_reference(r_resource); // // Check if Resource is saved to disk to potentially remove it from resource_count
	emit_signal(SNAME("resource_saved"), r_resource);
	_editor_data.notify_resource_saved(r_resource);

	if (EDITOR_GET("filesystem/on_save/warn_on_saving_large_text_resources")) {
		if (p_path.ends_with(".tres")) {
			const int64_t file_size = FileAccess::get_size(p_path);
			if (file_size >= LARGE_RESOURCE_WARNING_SIZE_THRESHOLD) {
				// File is larger than 500 KiB, likely because it contains binary data serialized as Base64.
				// This is slow to save and load, so warn the user.
				EditorToasterContainer::get_singleton()->popup_str(
						vformat(TTR("The text-based resource at path \"%s\" is large on disk (%s), likely because it has embedded binary data.\nThis slows down resource saving and loading.\nConsider saving its binary subresource(s) to a binary `.res` file or saving the resource as a binary `.res` file.\nThis warning can be disabled in the Editor Settings (FileSystem > On Save > Warn on Saving Large Text Resources)."), p_path, String::humanize_size(file_size)), EditorToasterContainer::WARNING);
			}
		}
	}
}

void EditorNode::save_resource(const Ref<Resource> &p_resource) {
	// If built-in resource, save the scene instead.
	if (p_resource->is_built_in()) {
		const String scene_path = p_resource->get_path().get_slice("::", 0);
		if (!scene_path.is_empty()) {
			if (ResourceLoader::exists(scene_path) && ResourceLoader::get_resource_type(scene_path) == "PackedScene") {
				save_scene_if_open(scene_path);
			} else {
				// Not a packed scene, so save it as regular resource.
				Ref<Resource> parent_resource = ResourceCache::get_ref(scene_path);
				ERR_FAIL_COND_MSG(parent_resource.is_null(), "Parent resource not loaded, can't save.");
				save_resource(parent_resource);
			}
			return;
		}
	}

	// Imported and editor content originals must be saved to a different path.
	String path = p_resource->get_path();
	if (path.is_resource_file() && !FileAccess::exists(path + ".import") && !FileAccessContent::is_content_path(path)) {
		save_resource_in_path(p_resource, p_resource->get_path());
	} else {
		save_resource_as(p_resource);
	}
}

void EditorNode::save_resource_as(const Ref<Resource> &p_resource, const String &p_at_path) {
	String resource_path = p_resource->get_path();
	bool is_resource = resource_path.is_resource_file();
	const bool is_editor_content = FileAccessContent::is_content_path(resource_path);

	{
		// Early exit checks.

		if (is_resource) {
			if (FileAccess::exists(resource_path + ".import")) {
				show_warning(TTR("This resource can't be saved because it was imported from another file. Make it unique first."));
				return;
			}
		} else {
			int separator_pos = resource_path.find("::");
			if (separator_pos != -1) {
				String base = resource_path.substr(0, separator_pos);
				String base_resource_type = ResourceLoader::get_resource_type(base);
				if (base_resource_type == "PackedScene" && (!get_edited_scene() || get_edited_scene()->get_scene_file_path() != base)) {
					show_warning(TTR("This resource can't be saved because it does not belong to the edited scene. Make it unique first."));
					return;
				}
			}
		}
	}

	_scene_resource_file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_SAVE_FILE);
	// Keep the embedded original in the resource cache when saving an editable copy.
	_saving_resource = is_editor_content ? p_resource->duplicate_deep() : p_resource;
	ERR_FAIL_COND(_saving_resource.is_null());

	_current_menu_option = RESOURCE_SAVE_AS;
	List<String> extensions;
	ResourceSaver::get_recognized_extensions(p_resource, &extensions);
	_scene_resource_file_dialog->clear_filters();

	List<String> preferred;
	for (const String &extension : extensions) {
		if (p_resource->is_class("Script") && (extension == "tres" || extension == "res")) {
			// This serves no purpose and confused people.
			continue;
		}
		_scene_resource_file_dialog->add_filter("*." + extension, extension.to_upper());
		preferred.push_back(extension);
	}
	// Lowest provided extension priority.
	List<String>::Element *res_element = preferred.find("res");
	if (res_element) {
		preferred.move_to_back(res_element);
	}

	String resource_name_snake_case = p_resource->get_class().to_snake_case();
	String new_resource_name_snake_case;
	if (!preferred.is_empty()) {
		new_resource_name_snake_case = "new_" + resource_name_snake_case + "." + preferred.front()->get().to_lower();
	}

	if (is_editor_content) {
		_scene_resource_file_dialog->set_current_dir(p_at_path.is_empty() ? "res://" : p_at_path);
		String copy_filename = resource_path.get_file();
		if (!preferred.is_empty() && !preferred.find(resource_path.get_extension().to_lower())) {
			copy_filename = copy_filename.get_basename() + "." + preferred.front()->get().to_lower();
		}
		_scene_resource_file_dialog->set_current_file(copy_filename);
	} else if (!p_at_path.is_empty()) {
		_scene_resource_file_dialog->set_current_dir(p_at_path);
		if (is_resource) {
			_scene_resource_file_dialog->set_current_file(resource_path.get_file());
		} else {
			_scene_resource_file_dialog->set_current_file(new_resource_name_snake_case);
		}
	} else if (!p_resource->get_path().get_base_dir().is_empty()) {
		if (is_resource) {
			if (!extensions.is_empty()) {
				const String ext = resource_path.get_extension().to_lower();
				if (extensions.find(ext) == nullptr) {
					_scene_resource_file_dialog->set_current_path(resource_path.replacen("." + ext, "." + extensions.front()->get()));
				}
			}
		} else {
			_scene_resource_file_dialog->set_current_file(new_resource_name_snake_case);
		}
	} else if (!preferred.is_empty()) {
		_scene_resource_file_dialog->set_current_file(new_resource_name_snake_case);
		_scene_resource_file_dialog->set_current_path(new_resource_name_snake_case);
	}
	_scene_resource_file_dialog->set_title(TTR("Save Resource As..."));
	_scene_resource_file_dialog->popup_file_dialog();
}

bool EditorNode::is_resource_internal_to_scene(Ref<Resource> p_resource) {
	bool inside_scene = !get_edited_scene() || get_edited_scene()->get_scene_file_path() == p_resource->get_path().get_slice("::", 0);
	return inside_scene || p_resource->get_path().is_empty();
}

void EditorNode::gather_resources(const Variant &p_variant, List<Ref<Resource>> &r_list, HashSet<Object *> &r_scanned_objects, bool p_subresources, bool p_allow_external) {
	Variant::Type type = p_variant.get_type();
	if (type == Variant::OBJECT && p_variant.get_validated_object() == nullptr) {
		return;
	}

	if (type != Variant::OBJECT && type != Variant::ARRAY && type != Variant::DICTIONARY) {
		return;
	}

	if (type == Variant::ARRAY) {
		Array arr = p_variant;
		for (const Variant &v : arr) {
			Ref<Resource> res = v;
			if (res.is_valid()) {
				if (p_allow_external) {
					r_list.push_back(res);
				} else if (is_resource_internal_to_scene(res)) {
					r_list.push_back(res);
				}
			}
			if (Object::cast_to<Node>(v) == nullptr) {
				gather_resources(v, r_list, r_scanned_objects, p_subresources, p_allow_external);
			}
		}
		return;
	}

	if (type == Variant::DICTIONARY) {
		Dictionary dict = p_variant;
		for (const KeyValue<Variant, Variant> &kv : dict) {
			Ref<Resource> res_key = kv.key;
			Ref<Resource> res_value = kv.value;
			if (res_key.is_valid()) {
				if (p_allow_external) {
					r_list.push_back(res_key);
				} else if (is_resource_internal_to_scene(res_key)) {
					r_list.push_back(res_key);
				}
			}
			if (res_value.is_valid()) {
				if (p_allow_external) {
					r_list.push_back(res_value);
				} else if (is_resource_internal_to_scene(res_value)) {
					r_list.push_back(res_value);
				}
			}
			if (Object::cast_to<Node>(kv.key) == nullptr) {
				gather_resources(kv.key, r_list, r_scanned_objects, p_subresources, p_allow_external);
			}
			if (Object::cast_to<Node>(kv.value) == nullptr) {
				gather_resources(kv.value, r_list, r_scanned_objects, p_subresources, p_allow_external);
			}
		}
		return;
	}

	// Variant::Object

	Object *object = p_variant;
	if (r_scanned_objects.has(object)) {
		return;
	}
	r_scanned_objects.insert(object);

	List<PropertyInfo> pinfo;
	object->get_property_list(&pinfo);

	for (const PropertyInfo &property_info : pinfo) {
		if (!(property_info.usage & PROPERTY_USAGE_EDITOR)) {
			continue;
		}

		Variant property_value = object->get(property_info.name);
		Variant::Type property_type = property_value.get_type();
		if (property_type == Variant::ARRAY || property_type == Variant::DICTIONARY) {
			gather_resources(property_value, r_list, r_scanned_objects, p_subresources, p_allow_external);
			continue;
		}
		Ref<Resource> res = property_value;
		if (res.is_null()) {
			continue;
		}
		if (!p_allow_external) {
			if (!res->is_built_in() || res->get_path().get_slice("::", 0) != get_edited_scene()->get_scene_file_path()) {
				if (!res->get_path().is_empty()) {
					continue;
				}
			}
		}
		r_list.push_back(res);
		if (p_subresources) {
			gather_resources(res, r_list, r_scanned_objects, p_subresources, p_allow_external);
		}
	}
}

void EditorNode::update_resource_count(Node *p_node, bool p_remove) {
	if (!get_edited_scene()) {
		return;
	}

	List<Ref<Resource>> res_list;
	HashSet<Object *> scanned_objects;
	gather_resources(p_node, res_list, scanned_objects, true);

	for (Ref<Resource> &resource : res_list) {
		List<Node *>::Element *node_entry = _resource_node_references[resource].find(p_node);
		if (node_entry) {
			if (p_remove) {
				_resource_node_references[resource].erase(node_entry);
			}
		} else {
			_resource_node_references[resource].push_back(p_node);
		}
	}

	_queue_resource_counter_changed();
}

void EditorNode::update_node_reference(const Variant &p_value, Node *p_node, bool p_remove) {
	List<Ref<Resource>> list;
	Ref<Resource> res = p_value;
	HashSet<Object *> scanned_objects;
	gather_resources(p_value, list, scanned_objects, true); //Gather all Resources and their SubResources to remove p_node from their lists.

	if (res.is_valid() && is_resource_internal_to_scene(res)) {
		// Avoid external Resources from being added in.
		list.push_back(res);
	}

	for (Ref<Resource> &resource : list) {
		if (!p_remove) {
			_resource_node_references[resource].push_back(p_node);
		} else {
			List<Node *>::Element *node_entry = _resource_node_references[resource].find(p_node);
			if (node_entry) {
				_resource_node_references[resource].erase(node_entry);
			}
		}
	}
	_queue_resource_counter_changed();
}

void EditorNode::clear_node_reference(Ref<Resource> p_res) {
	if (is_resource_internal_to_scene(p_res)) {
		return;
	}
	List<Node *> *node_list = _resource_node_references.getptr(p_res);
	if (node_list != nullptr) {
		node_list->clear();
	}
}

int EditorNode::get_resource_count(Ref<Resource> p_res) {
	List<Node *> *nodes = _resource_node_references.getptr(p_res);
	return nodes ? nodes->size() : 0;
}

List<Node *> EditorNode::get_resource_node_list(Ref<Resource> p_res) {
	List<Node *> *nodes = _resource_node_references.getptr(p_res);
	return nodes == nullptr ? List<Node *>() : List<Node *>(*nodes);
}

void EditorNode::push_item(Object *p_object, const String &p_property, bool p_inspector_only) {
	if (!p_object) {
		InspectorDock::get_inspector_singleton()->edit(nullptr);
		SignalsDock::get_singleton()->set_object(nullptr);
		GroupsDock::get_singleton()->set_selection(Vector<Node *>());
		HierarchyDock::get_singleton()->set_selected(nullptr);
		InspectorDock::get_singleton()->update(nullptr);
		EditorDebuggerNode::get_singleton()->clear_remote_tree_selection();
		hide_unused_editors();
		return;
	}
	_add_to_history(p_object, p_property, p_inspector_only);
	_edit_current();
}

void EditorNode::push_item_no_inspector(Object *p_object) {
	_add_to_history(p_object, "", false);
	_edit_current(false, true);
}

void EditorNode::edit_previous_item() {
	if (_editor_history.previous()) {
		_edit_current();
	}
}

void EditorNode::edit_item(Object *p_object, Object *p_editing_owner, bool p_set_current) {
	ERR_FAIL_NULL(p_editing_owner);

	// Editing for this type of object may be disabled by user's feature profile.
	if (!p_object || _is_class_editor_disabled_by_feature_profile(p_object->get_class())) {
		// Nothing to edit, clean up the owner context and return.
		hide_unused_editors(p_editing_owner);
		return;
	}

	const ObjectID owner_id = p_editing_owner->get_instance_id();
	const ObjectID object_id = p_object->get_instance_id();
	if (ScriptEditorDock *script_editor = ScriptEditorDock::get_singleton()) {
		script_editor->edit_resource(p_object);
		// Opening a built-in script can load a scene and replace the editing context.
		if (!ObjectDB::get_instance(owner_id) || !ObjectDB::get_instance(object_id)) {
			return;
		}
	}

	// Get a list of editor plugins that can handle this type of object.
	Vector<EditorPlugin *> available_plugins = _editor_data.get_handling_sub_editors(p_object);
	if (available_plugins.is_empty()) {
		// None, clean up the owner context and return.
		hide_unused_editors(p_editing_owner);
		return;
	}

	// Remove editor plugins no longer used by this editing owner. Keep the ones that can
	// still be reused by the new edited object.

	List<EditorPlugin *> to_remove;
	for (EditorPlugin *plugin : _active_plugins[owner_id]) {
		if (!available_plugins.has(plugin)) {
			to_remove.push_back(plugin);
			if (plugin->can_auto_hide()) {
				_plugin_over_edit(plugin, nullptr);
			} else {
				// If plugin can't be hidden, make it own itself and become responsible for closing.
				_register_plugin_as_its_own_owner(plugin);
			}
		}
	}

	for (EditorPlugin *plugin : to_remove) {
		_active_plugins[owner_id].erase(plugin);
	}

	LocalVector<EditorPlugin *> to_over_edit;

	// Send the edited object to the plugins.
	for (EditorPlugin *plugin : available_plugins) {
		if (_active_plugins[owner_id].has(plugin)) {
			// Plugin was already active, just change the object and ensure it's visible.
			plugin->edit(p_object);
			plugin->make_visible(true);
			if (p_set_current) {
				plugin->set_current();
			}
			continue;
		}

		if (_active_plugins.has(plugin->get_instance_id())) {
			// Plugin is already active, but as self-owning, so it needs a separate check.
			plugin->edit(p_object);
			plugin->make_visible(true);
			if (p_set_current) {
				plugin->set_current();
			}
			continue;
		}

		bool need_to_add = true;
		List<EditorPropertyResource *> to_fold;

		// If plugin is already associated with another owner, remove it from there first.
		for (KeyValue<ObjectID, HashSet<EditorPlugin *>> &kv : _active_plugins) {
			if (kv.key == owner_id || !kv.value.has(plugin)) {
				continue;
			}
			EditorPropertyResource *epres = ObjectDB::get_instance<EditorPropertyResource>(kv.key);
			if (epres) {
				// If it's resource property editing the same resource type, fold it later to avoid premature modifications
				// that may result in unsafe iteration of active_plugins.
				to_fold.push_back(epres);
			} else {
				kv.value.erase(plugin);
				need_to_add = false;
			}
		}

		if (!need_to_add && to_fold.is_empty()) {
			plugin->edit(p_object);
			plugin->make_visible(true);
		} else {
			for (EditorPropertyResource *epres : to_fold) {
				epres->fold_resource();
			}

			// TODO: Call the function directly once a proper priority system is implemented.
			to_over_edit.push_back(plugin);
		}

		// Activate previously inactive plugin and edit the object.
		_active_plugins[owner_id].insert(plugin);
	}

	for (EditorPlugin *plugin : to_over_edit) {
		_plugin_over_edit(plugin, p_object, p_set_current);
	}
}

void EditorNode::push_node_item(Node *p_node) {
	if (p_node || !InspectorDock::get_inspector_singleton()->get_edited_object() || Object::cast_to<Node>(InspectorDock::get_inspector_singleton()->get_edited_object()) || Object::cast_to<MultiNodeEdit>(InspectorDock::get_inspector_singleton()->get_edited_object())) {
		// Don't push null if the currently edited object is not a Node.
		push_item(p_node);
	}
}

void EditorNode::hide_unused_editors(const Object *p_editing_owner) {
	if (p_editing_owner) {
		const ObjectID id = p_editing_owner->get_instance_id();
		for (EditorPlugin *plugin : _active_plugins[id]) {
			if (plugin->can_auto_hide()) {
				_plugin_over_edit(plugin, nullptr);
			} else {
				_register_plugin_as_its_own_owner(plugin);
			}
		}
		_active_plugins.erase(id);
	} else {
		// If no editing owner is provided, this method will go over all owners and check if they are valid.
		// This is to sweep properties that were removed from the inspector.
		List<ObjectID> to_remove;
		for (KeyValue<ObjectID, HashSet<EditorPlugin *>> &kv : _active_plugins) {
			Object *context = ObjectDB::get_instance(kv.key);
			if (context) {
				// In case of self-owning plugins, they are disabled here if they can auto hide.
				const EditorPlugin *self_owning = Object::cast_to<EditorPlugin>(context);
				if (self_owning && self_owning->can_auto_hide()) {
					context = nullptr;
				}
			}

			if (!context || context->call(SNAME("_should_stop_editing"))) {
				to_remove.push_back(kv.key);
				for (EditorPlugin *plugin : kv.value) {
					if (plugin->can_auto_hide()) {
						_plugin_over_edit(plugin, nullptr);
					} else {
						_register_plugin_as_its_own_owner(plugin);
					}
				}
			}
		}

		for (const ObjectID &id : to_remove) {
			_active_plugins.erase(id);
		}
	}
}

void EditorNode::replace_resources_in_object(Object *r_object, const Vector<Ref<Resource>> &p_source_resources, const Vector<Ref<Resource>> &p_target_resource) {
	List<PropertyInfo> pi;
	r_object->get_property_list(&pi);

	for (const PropertyInfo &property_info : pi) {
		if (!(property_info.usage & PROPERTY_USAGE_STORAGE)) {
			continue;
		}

		switch (property_info.type) {
			case Variant::OBJECT: {
				if (property_info.hint == PROPERTY_HINT_RESOURCE_TYPE) {
					const Variant &v = r_object->get(property_info.name);
					Ref<Resource> res = v;

					if (res.is_valid()) {
						int res_idx = p_source_resources.find(res);
						if (res_idx != -1) {
							r_object->set(property_info.name, p_target_resource.get(res_idx));
						} else {
							replace_resources_in_object(v, p_source_resources, p_target_resource);
						}
					}
				}
			} break;
			case Variant::ARRAY: {
				Array varray = r_object->get(property_info.name);
				int len = varray.size();
				bool array_requires_updating = false;
				for (int i = 0; i < len; i++) {
					const Variant &v = varray.get(i);
					Ref<Resource> res = v;

					if (res.is_valid()) {
						int res_idx = p_source_resources.find(res);
						if (res_idx != -1) {
							varray.set(i, p_target_resource.get(res_idx));
							array_requires_updating = true;
						} else {
							replace_resources_in_object(v, p_source_resources, p_target_resource);
						}
					}
				}
				if (array_requires_updating) {
					r_object->set(property_info.name, varray);
				}
			} break;
			case Variant::DICTIONARY: {
				Dictionary d = r_object->get(property_info.name);
				bool dictionary_requires_updating = false;
				for (const Variant &dictionary_key : d.get_key_list()) {
					Variant v = d[dictionary_key];
					Ref<Resource> res = v;

					if (res.is_valid()) {
						int res_idx = p_source_resources.find(res);
						if (res_idx != -1) {
							d[dictionary_key] = p_target_resource.get(res_idx);
							dictionary_requires_updating = true;
						} else {
							replace_resources_in_object(v, p_source_resources, p_target_resource);
						}
					}
				}
				if (dictionary_requires_updating) {
					r_object->set(property_info.name, d);
				}
			} break;
			default: {
			}
		}
	}

	Node *n = Object::cast_to<Node>(r_object);
	if (n) {
		for (int i = 0; i < n->get_child_count(); i++) {
			replace_resources_in_object(n->get_child(i), p_source_resources, p_target_resource);
		}
	}
}

void EditorNode::replace_resources_in_scenes(const Vector<Ref<Resource>> &p_source_resources, const Vector<Ref<Resource>> &p_target_resource) {
	for (int i = 0; i < _editor_data.get_edited_scene_count(); i++) {
		Node *edited_scene_root = _editor_data.get_edited_scene_root(i);
		if (edited_scene_root) {
			replace_resources_in_object(edited_scene_root, p_source_resources, p_target_resource);
		}
	}
}

void EditorNode::edit_foreign_resource(Ref<Resource> p_resource) {
	open_scene(p_resource->get_path().get_slice("::", 0));
	callable_mp(InspectorDock::get_singleton(), &InspectorDock::edit_resource).call_deferred(p_resource);
}

bool EditorNode::is_resource_read_only(Ref<Resource> p_resource, bool p_foreign_resources_are_writable) {
	ERR_FAIL_COND_V(p_resource.is_null(), false);

	String path = p_resource->get_path();
	if (FileAccessContent::is_content_path(path)) {
		return true;
	}
	if (!path.is_resource_file()) {
		// If the resource name contains '::', that means it is a subresource embedded in another resource.
		int srpos = path.find("::");
		if (srpos != -1) {
			String base = path.substr(0, srpos);
			// If the base resource is a packed scene, we treat it as read-only if it is not the currently edited scene.
			if (ResourceLoader::get_resource_type(base) == "PackedScene") {
				if (!get_tree()->get_edited_scene_root() || get_tree()->get_edited_scene_root()->get_scene_file_path() != base) {
					// If we have not flagged foreign resources as writable or the base scene the resource is
					// part was imported, it can be considered read-only.
					if (!p_foreign_resources_are_writable || FileAccess::exists(base + ".import")) {
						return true;
					}
				}
			} else {
				// If a corresponding .import file exists for the base file, we assume it to be imported and should therefore treated as read-only.
				if (FileAccess::exists(base + ".import")) {
					return true;
				}
			}
		}
	} else if (FileAccess::exists(path + ".import")) {
		// The resource is not a subresource, but if it has an .import file, it's imported so treat it as read only.
		return true;
	}

	return false;
}

String EditorNode::get_multiwindow_support_tooltip_text() const {
	if (SceneTree::get_singleton()->get_root()->is_embedding_subwindows()) {
		if (DisplayServer::get_singleton()->has_feature(DisplayServerEnums::FEATURE_SUBWINDOWS)) {
			return TTR("Multi-window support is not available because the `--single-window` command line argument was used to start the editor.");
		} else {
			return TTR("Multi-window support is not available because the current platform doesn't support multiple windows.");
		}
	} else if (EDITOR_GET("interface/editor/display/single_window_mode")) {
		return TTR("Multi-window support is not available because Interface > Editor > Single Window Mode is enabled in the editor settings.");
	}

	return TTR("Multi-window support is not available because Interface > Multi Window > Enable is disabled in the editor settings.");
}

bool EditorNode::is_changing_scene() const {
	return _changing_scene;
}

void EditorNode::set_edited_scene(Node *p_scene) {
	set_edited_scene_root(p_scene, true);
}

void EditorNode::set_edited_scene_root(Node *p_scene, bool p_auto_add) {
	Node *old_edited_scene_root = get_editor_data().get_edited_scene_root();
	ERR_FAIL_COND_MSG(p_scene && p_scene != old_edited_scene_root && p_scene->get_parent(), "Non-null nodes that are set as edited scene should not have a parent node.");

	if (p_auto_add && old_edited_scene_root && old_edited_scene_root->get_parent() == _edited_scene_viewport) {
		_edited_scene_viewport->remove_child(old_edited_scene_root);
	}
	get_editor_data().set_edited_scene_root(p_scene);

	if (Object::cast_to<Popup>(p_scene)) {
		Object::cast_to<Popup>(p_scene)->show();
	}
	HierarchyDock::get_singleton()->set_edited_scene(p_scene);
	if (get_tree()) {
		get_tree()->set_edited_scene_root(p_scene);
	}

	if (p_auto_add && p_scene) {
		_edited_scene_viewport->add_child(p_scene, true);
	}
}

String EditorNode::get_preview_locale() const {
	const Ref<TranslationDomain> &main_domain = TranslationServer::get_singleton()->get_main_domain();
	return main_domain->is_enabled() ? main_domain->get_locale_override() : String();
}

bool EditorNode::is_pseudolocalization_enabled() const {
	const Ref<TranslationDomain> &main_domain = TranslationServer::get_singleton()->get_main_domain();
	return main_domain->is_pseudolocalization_enabled();
}

void EditorNode::set_preview_locale(const String &p_locale, bool p_pseudolocalization) {
	const String &prev_locale = get_preview_locale();
	bool prev_pseudo = is_pseudolocalization_enabled();
	if (prev_locale == p_locale && prev_pseudo == p_pseudolocalization) {
		return;
	}

	// Texts set in the editor could be identifiers that should never be translated.
	// So we need to disable translation entirely.
	Ref<TranslationDomain> main_domain = TranslationServer::get_singleton()->get_main_domain();
	if (p_locale.is_empty()) {
		// Disable preview. Use the source locale context.
		main_domain->set_locale_override(TranslationServer::get_singleton()->get_default_context());
	} else {
		// Preview a specific locale.
		main_domain->set_locale_override(p_locale);
	}
	main_domain->set_pseudolocalization_enabled(p_pseudolocalization);
	// Texts set in the editor could be identifiers that should never be translated.
	// So we need to disable translation entirely.
	main_domain->set_enabled(p_pseudolocalization || !p_locale.is_empty());

	EditorSettings::get_singleton()->set_project_metadata("editor_metadata", "preview_locale", p_locale);

	_translation_resources_changed();
}

int EditorNode::new_scene() {
	int idx = _editor_data.add_edited_scene(-1);
	_set_current_scene(idx); // Before trying to remove an empty scene, set the current tab index to the newly added tab index.

	// Remove placeholder empty scene.
	if (_editor_data.get_edited_scene_count() > 1) {
		for (int i = 0; i < _editor_data.get_edited_scene_count() - 1; i++) {
			bool unsaved = EditorUndoRedoManager::get_singleton()->is_history_unsaved(_editor_data.get_scene_history_id(i));
			if (!unsaved && _editor_data.get_scene_path(i).is_empty() && _editor_data.get_edited_scene_root(i) == nullptr) {
				_editor_data.remove_scene(i);
				idx--;
			}
		}
	}

	_editor_data.clear_editor_states();
	_scene_tabs->update_scene_tabs();
	return idx;
}

Error EditorNode::load_scene(const String &p_scene, bool p_ignore_broken_deps, bool p_set_inherited, bool p_force_open_imported, bool p_update_tabs) {
	const String lpath = ProjectSettings::get_singleton()->localize_path(ResourceUID::ensure_path(p_scene));
	_update_prev_closed_scenes(lpath, false);

	if (!p_set_inherited) {
		for (int i = 0; i < _editor_data.get_edited_scene_count(); i++) {
			if (_editor_data.get_scene_path(i) == lpath) {
				// Already loaded, do nothing.
				return OK;
			}
		}

		if (!p_force_open_imported && FileAccess::exists(lpath + ".import")) {
			_open_imported_dialog->set_text(vformat(TTR("Scene '%s' was automatically imported, so it can't be modified.\nTo make changes to it, a new inherited scene can be created."), lpath.get_file()));
			_open_imported_dialog->popup_centered();
			_new_inherited_scene_btn->grab_focus();
			_open_import_request = lpath;
			return OK;
		}
	}

	if (!lpath.begins_with("res://")) {
		show_warning(TTR("Error loading scene, it must be inside the project path. Use 'Import' to open the scene, then save it inside the project path."));
		return ERR_FILE_NOT_FOUND;
	}

	_dependency_errors.clear();

	Error err;
	Ref<PackedScene> sdata = ResourceLoader::load(lpath, "", ResourceFormatLoader::CACHE_MODE_REPLACE, &err);

	if (!p_ignore_broken_deps && !_dependency_errors.is_empty()) {
		_current_menu_option = -1;
		_dependency_error_dialog->show(lpath, _dependency_errors);
		_dependency_errors.clear();
		return ERR_FILE_MISSING_DEPENDENCIES;
	}

	if (sdata.is_null()) {
		_dialog_display_load_error(lpath, err);
		return ERR_FILE_NOT_FOUND;
	}

	_dependency_errors.erase(lpath); // At least not self path.

	for (KeyValue<String, HashSet<String>> &dependency_error : _dependency_errors) {
		String txt = vformat(TTR("Scene '%s' has broken dependencies:"), dependency_error.key) + "\n";
		for (const String &dependency : dependency_error.value) {
			txt += "\t" + dependency + "\n";
		}
		add_io_error(txt);
	}

	if (ResourceCache::has(lpath)) {
		// Used from somewhere else? No problem! Update state and replace sdata.
		Ref<PackedScene> ps = ResourceCache::get_ref(lpath);
		if (ps.is_valid()) {
			ps->replace_state(sdata->get_state());
			ps->set_last_modified_time(sdata->get_last_modified_time());
			sdata = ps;
		}
	} else {
		sdata->set_path(lpath, true); // Take over path.
	}

	Node *new_scene = sdata->instantiate(p_set_inherited ? PackedScene::GEN_EDIT_STATE_MAIN_INHERITED : PackedScene::GEN_EDIT_STATE_MAIN);
	if (!new_scene) {
		sdata.unref();
		_dialog_display_load_error(lpath, ERR_FILE_CORRUPT);
		return ERR_FILE_CORRUPT;
	}

	if (p_set_inherited) {
		Ref<SceneState> state = sdata->get_state();
		state->set_path(lpath);
		new_scene->set_scene_inherited_state(state);
		new_scene->set_scene_file_path(String());
	}

	new_scene->set_scene_instance_state(Ref<SceneState>());

	if (!_restoring_scenes) {
		save_editor_layout_delayed();
		_add_to_recent_scenes(lpath);
	}

	int idx = _editor_data.get_edited_scene();
	if (idx == -1 || _editor_data.get_edited_scene_root() || !_editor_data.get_scene_path(idx).is_empty()) {
		idx = _editor_data.add_edited_scene(-1);
	}
	_editor_data.set_scene_root(idx, new_scene);
	_editor_data.set_scene_resource(idx, sdata);

	const Ref<ConfigFile> editor_state_cf = _load_scene_config(lpath);
	if (editor_state_cf->has_section("editor_states")) {
		_editor_data.load_editor_plugin_states_from_config(editor_state_cf, idx);
	}

	if (_editor_folding.has_folding_data(lpath)) {
		_editor_folding.load_scene_folding(new_scene, lpath);
	} else if (EDITOR_GET("interface/inspector/auto_unfold_foreign_scenes")) {
		_editor_folding.unfold_scene(new_scene);
		_editor_folding.save_scene_folding(new_scene, lpath);
	}

	if (p_update_tabs) {
		_scene_tabs->update_scene_tabs();
	}
	return OK;
}

Error EditorNode::open_scene(const String &p_scene, bool p_ignore_broken_deps, bool p_set_inherited, bool p_force_open_imported) {
	if (!is_inside_tree()) {
		_defer_load_scene = p_scene;
		return OK;
	}

	const String lpath = ProjectSettings::get_singleton()->localize_path(ResourceUID::ensure_path(p_scene));
	if (!p_set_inherited) {
		for (int i = 0; i < _editor_data.get_edited_scene_count(); i++) {
			if (_editor_data.get_scene_path(i) == lpath) {
				_set_current_scene(i);
				_scene_tabs->update_scene_tabs();
				return OK;
			}
		}
	}

	bool ignore_state = !_editor_data.get_edited_scene_root();

	RETURN_IF_ERROR(load_scene(p_scene, p_ignore_broken_deps, p_set_inherited, p_force_open_imported));

	int current_scene_idx = _editor_data.get_edited_scene_count() - 1;
	Node *new_scene = _editor_data.get_edited_scene_root(current_scene_idx);
	ERR_FAIL_NULL_V(new_scene, ERR_BUG);

	_apply_edited_scene(current_scene_idx, ignore_state);

	// When editor plugins load in, they might use node transforms during their own setup, so make sure they're up to date.
	get_tree()->flush_transform_notifications();

	EditorDebuggerNode::get_singleton()->update_live_edit_root();

	if (_restoring_scenes) {
		// Initialize history for restored scenes.
		ObjectID id = new_scene->get_instance_id();
		if (id != _editor_history.get_current()) {
			_editor_history.add_object(id);
		}
	}

	if (p_set_inherited) {
		EditorUndoRedoManager::get_singleton()->set_history_as_unsaved(_editor_data.get_current_edited_scene_history_id());
	}

	_update_title();
	return OK;
}

Error EditorNode::load_resource(const String &p_resource, bool p_ignore_broken_deps) {
	_dependency_errors.clear();

	Error err;

	Ref<Resource> res;
	if (_force_textfile_extensions.has(p_resource.get_extension())) {
		const String resource_type = ResourceLoader::get_resource_type(p_resource);
		if (resource_type != "Translation" && ResourceLoader::exists(p_resource, "")) {
			res = ResourceLoader::load(p_resource, "", ResourceFormatLoader::CACHE_MODE_REUSE, &err);
		}
		if (res.is_null()) {
			res = ResourceCache::get_ref(p_resource);
			if (res.is_null() || !res->is_class("TextFile")) {
				res = ScriptEditorDock::get_singleton()->open_file(p_resource);
			}
		}
	} else if (ResourceLoader::exists(p_resource, "")) {
		res = ResourceLoader::load(p_resource, "", ResourceFormatLoader::CACHE_MODE_REUSE, &err);
	} else if (_textfile_extensions.has(p_resource.get_extension())) {
		res = ScriptEditorDock::get_singleton()->open_file(p_resource);
	} else if (_other_file_extensions.has(p_resource.get_extension())) {
		OS::get_singleton()->shell_open(ProjectSettings::get_singleton()->globalize_path(p_resource));
		return OK;
	}
	ERR_FAIL_COND_V(res.is_null(), ERR_CANT_OPEN);

	if (!p_ignore_broken_deps && !_dependency_errors.is_empty()) {
		_dependency_error_dialog->show(p_resource, _dependency_errors);
		_dependency_errors.clear();

		return ERR_FILE_MISSING_DEPENDENCIES;
	}

	InspectorDock::get_singleton()->edit_resource(res);
	return OK;
}

Error EditorNode::load_scene_or_resource(const String &p_path, bool p_ignore_broken_deps, bool p_change_scene_tab_if_already_open) {
	if (ClassDB::is_parent_class(ResourceLoader::get_resource_type(p_path), "PackedScene")) {
		if (!p_change_scene_tab_if_already_open && EditorNode::get_singleton()->is_scene_open(p_path)) {
			return OK;
		}
		return EditorNode::get_singleton()->open_scene(p_path, p_ignore_broken_deps);
	}
	return EditorNode::get_singleton()->load_resource(p_path, p_ignore_broken_deps);
}

HashMap<StringName, Variant> EditorNode::get_modified_properties_for_node(Node *p_node, bool p_node_references_only) {
	HashMap<StringName, Variant> modified_property_map;

	List<PropertyInfo> pinfo;
	p_node->get_property_list(&pinfo);
	for (const PropertyInfo &property_info : pinfo) {
		if (property_info.usage & PROPERTY_USAGE_STORAGE) {
			bool node_reference = (property_info.type == Variant::OBJECT && property_info.hint == PROPERTY_HINT_NODE_TYPE);
			if (p_node_references_only && !node_reference) {
				continue;
			}
			bool is_valid_revert = false;
			Variant revert_value = EditorPropertyRevert::get_property_revert_value(p_node, property_info.name, &is_valid_revert);
			Variant current_value = p_node->get(property_info.name);
			if (is_valid_revert) {
				if (PropertyUtils::is_property_value_different(p_node, current_value, revert_value)) {
					// If this property is a direct node reference, save a NodePath instead to prevent corrupted references.
					if (node_reference) {
						Node *target_node = Object::cast_to<Node>(current_value);
						if (target_node) {
							modified_property_map[property_info.name] = p_node->get_path_to(target_node);
						}
					} else {
						modified_property_map[property_info.name] = current_value;
					}
				}
			}
		}
	}

	return modified_property_map;
}

HashMap<StringName, Variant> EditorNode::get_modified_properties_reference_to_nodes(Node *p_node, const List<Node *> &p_nodes_referenced_by) {
	HashMap<StringName, Variant> modified_property_map;

	List<PropertyInfo> pinfo;
	p_node->get_property_list(&pinfo);
	for (const PropertyInfo &property_info : pinfo) {
		if (property_info.usage & PROPERTY_USAGE_STORAGE) {
			if (property_info.type != Variant::OBJECT || property_info.hint != PROPERTY_HINT_NODE_TYPE) {
				continue;
			}
			Variant current_value = p_node->get(property_info.name);
			Node *target_node = Object::cast_to<Node>(current_value);
			if (target_node && p_nodes_referenced_by.find(target_node)) {
				modified_property_map[property_info.name] = p_node->get_path_to(target_node);
			}
		}
	}

	return modified_property_map;
}

void EditorNode::set_unfocused_low_processor_usage_mode_enabled(bool p_enabled) {
	_unfocused_low_processor_usage_mode_enabled = p_enabled;
}

void EditorNode::update_node_from_node_modification_entry(Node *r_node, const ModificationNodeEntry &p_node_modification) {
	if (r_node) {
		// First, attempt to restore the script property since it may affect the get_property_list method.
		const Variant *script_property_table_entry = p_node_modification.property_table.getptr(CoreStringName(script));
		if (script_property_table_entry) {
			r_node->set_script(*script_property_table_entry);
		}

		// Get properties for this node.
		List<PropertyInfo> pinfo;
		r_node->get_property_list(&pinfo);

		// Get names of all valid property names.
		HashMap<StringName, bool> property_node_reference_table;
		for (const PropertyInfo &property_info : pinfo) {
			if (property_info.usage & PROPERTY_USAGE_STORAGE) {
				if (property_info.type == Variant::OBJECT && property_info.hint == PROPERTY_HINT_NODE_TYPE) {
					property_node_reference_table[property_info.name] = true;
				} else {
					property_node_reference_table[property_info.name] = false;
				}
			}
		}

		// Restore the modified properties for this node.
		for (const KeyValue<StringName, Variant> &property_entry : p_node_modification.property_table) {
			bool *property_node_reference_table_entry = property_node_reference_table.getptr(property_entry.key);
			if (property_node_reference_table_entry) {
				// If the property is a node reference, attempt to restore from the node path instead.
				bool is_node_reference = *property_node_reference_table_entry;
				if (is_node_reference) {
					if (property_entry.value.get_type() == Variant::NODE_PATH) {
						r_node->set(property_entry.key, r_node->get_node_or_null(property_entry.value));
					}
				} else {
					r_node->set(property_entry.key, property_entry.value);
				}
			}
		}

		// Restore the connections to other nodes.
		for (const ConnectionWithNodePath &saved_connection : p_node_modification.connections_to) {
			Connection conn = saved_connection.connection;

			// Get the node the callable is targeting.
			Node *target_node = Object::cast_to<Node>(conn.callable.get_object());

			// If the callable object no longer exists or is marked for deletion,
			// attempt to reaccquire the closest match by using the node path
			// we saved earlier.
			if (!target_node || !target_node->is_queued_for_deletion()) {
				target_node = r_node->get_node_or_null(saved_connection.node_path);
			}

			if (target_node) {
				// Reconstruct the callable.
				Callable new_callable = Callable(target_node, conn.callable.get_method());

				if (!r_node->is_connected(conn.signal.get_name(), new_callable)) {
					ERR_FAIL_COND(r_node->connect(conn.signal.get_name(), new_callable, conn.flags) != OK);
				}
			}
		}

		// Restore the connections from other nodes.
		for (const Connection &saved_connection : p_node_modification.connections_from) {
			Connection conn = saved_connection;

			bool valid = r_node->has_method(conn.callable.get_method()) || Ref<Script>(r_node->get_script()).is_null() || Ref<Script>(r_node->get_script())->has_method(conn.callable.get_method());
			ERR_CONTINUE_MSG(!valid, vformat("Attempt to connect signal '%s.%s' to nonexistent method '%s.%s'.", conn.signal.get_object()->get_class(), conn.signal.get_name(), conn.callable.get_object()->get_class(), conn.callable.get_method()));

			// Get the object which the signal is connected from.
			Object *source_object = conn.signal.get_object();

			if (source_object) {
				ERR_FAIL_COND(source_object->connect(conn.signal.get_name(), Callable(r_node, conn.callable.get_method()), conn.flags) != OK);
			}
		}

		// Re-add the groups.
		for (const Node::GroupInfo &saved_group : p_node_modification.groups) {
			r_node->add_to_group(saved_group.name, saved_group.persistent);
		}
	}
}

void EditorNode::get_scene_editor_data_for_node(Node *p_root, Node *p_node, HashMap<NodePath, SceneEditorDataEntry> &r_table) {
	SceneEditorDataEntry new_entry;
	new_entry.is_display_folded = p_node->is_displayed_folded();

	if (p_root != p_node) {
		new_entry.is_editable = p_root->is_editable_instance(p_node);
	}

	r_table.insert(p_root->get_path_to(p_node), new_entry);

	for (int i = 0; i < p_node->get_child_count(); i++) {
		get_scene_editor_data_for_node(p_root, p_node->get_child(i), r_table);
	}
}

void EditorNode::get_preload_scene_modification_table(
		Node *p_edited_scene,
		Node *p_reimported_root,
		Node *p_node,
		InstanceModificationsEntry &r_instance_modifications) {
	if (is_additional_node_in_scene(p_edited_scene, p_reimported_root, p_node)) {
		// Only save additional nodes which have an owner since this was causing issues transient ownerless nodes
		// which get recreated upon scene tree entry.
		// For now instead, assume all ownerless nodes are transient and will have to be recreated.
		if (p_node->get_owner()) {
			HashMap<StringName, Variant> modified_properties = get_modified_properties_for_node(p_node, true);
			if (p_node->get_owner() == p_edited_scene) {
				AdditiveNodeEntry new_additive_node_entry;
				new_additive_node_entry.node = p_node;
				new_additive_node_entry.parent = p_reimported_root->get_path_to(p_node->get_parent());
				new_additive_node_entry.owner = p_node->get_owner();
				new_additive_node_entry.index = p_node->get_index();

				CanvasItem *canvas_item = Object::cast_to<CanvasItem>(p_node);
				if (canvas_item) {
					new_additive_node_entry.transform_2d = canvas_item->get_transform();
				}
				Node3D *node_3d = Object::cast_to<Node3D>(p_node);
				if (node_3d) {
					new_additive_node_entry.transform_3d = node_3d->get_transform();
				}

				r_instance_modifications.addition_list.push_back(new_additive_node_entry);
			}
			if (!modified_properties.is_empty()) {
				ModificationNodeEntry modification_node_entry;
				modification_node_entry.property_table = modified_properties;

				r_instance_modifications.modifications[p_reimported_root->get_path_to(p_node)] = modification_node_entry;
			}
		}
	} else {
		HashMap<StringName, Variant> modified_properties = get_modified_properties_for_node(p_node, false);

		// Find all valid connections to other nodes.
		List<Connection> connections_to;
		p_node->get_all_signal_connections(&connections_to);

		List<ConnectionWithNodePath> valid_connections_to;
		for (const Connection &c : connections_to) {
			Node *connection_target_node = Object::cast_to<Node>(c.callable.get_object());
			if (connection_target_node) {
				// TODO: add support for reinstating custom callables
				if (!c.callable.is_custom()) {
					ConnectionWithNodePath connection_to;
					connection_to.connection = c;
					connection_to.node_path = p_node->get_path_to(connection_target_node);
					valid_connections_to.push_back(connection_to);
				}
			}
		}

		// Find all valid connections from other nodes.
		List<Connection> connections_from;
		p_node->get_signals_connected_to_this(&connections_from);

		List<Connection> valid_connections_from;
		for (const Connection &c : connections_from) {
			Node *source_node = Object::cast_to<Node>(c.signal.get_object());

			Node *valid_source_owner = nullptr;
			if (source_node) {
				valid_source_owner = source_node->get_owner();
				while (valid_source_owner) {
					if (valid_source_owner == p_reimported_root) {
						break;
					}
					valid_source_owner = valid_source_owner->get_owner();
				}
			}

			if (!source_node || valid_source_owner == nullptr) {
				// TODO: add support for reinstating custom callables
				if (!c.callable.is_custom()) {
					valid_connections_from.push_back(c);
				}
			}
		}

		// Find all node groups.
		List<Node::GroupInfo> groups;
		p_node->get_groups(&groups);

		if (!modified_properties.is_empty() || !valid_connections_to.is_empty() || !valid_connections_from.is_empty() || !groups.is_empty()) {
			ModificationNodeEntry modification_node_entry;
			modification_node_entry.property_table = modified_properties;
			modification_node_entry.connections_to = valid_connections_to;
			modification_node_entry.connections_from = valid_connections_from;
			modification_node_entry.groups = groups;

			r_instance_modifications.modifications[p_reimported_root->get_path_to(p_node)] = modification_node_entry;
		}
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		get_preload_scene_modification_table(p_edited_scene, p_reimported_root, p_node->get_child(i), r_instance_modifications);
	}
}

void EditorNode::get_preload_modifications_reference_to_nodes(
		Node *p_root,
		Node *p_node,
		const HashSet<Node *> &p_excluded_nodes,
		const List<Node *> &p_instance_list_with_children,
		HashMap<NodePath, ModificationNodeEntry> &r_modification_table) {
	if (!p_excluded_nodes.find(p_node)) {
		HashMap<StringName, Variant> modified_properties = get_modified_properties_reference_to_nodes(p_node, p_instance_list_with_children);

		if (!modified_properties.is_empty()) {
			ModificationNodeEntry modification_node_entry;
			modification_node_entry.property_table = modified_properties;

			r_modification_table[p_root->get_path_to(p_node)] = modification_node_entry;
		}

		for (int i = 0; i < p_node->get_child_count(); i++) {
			get_preload_modifications_reference_to_nodes(p_root, p_node->get_child(i), p_excluded_nodes, p_instance_list_with_children, r_modification_table);
		}
	}
}

void EditorNode::get_children_nodes(Node *p_node, List<Node *> &r_nodes) {
	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node *child = p_node->get_child(i);
		r_nodes.push_back(child);
		get_children_nodes(child, r_nodes);
	}
}

bool EditorNode::is_additional_node_in_scene(Node *p_edited_scene, Node *p_reimported_root, Node *p_node) {
	if (p_node == p_reimported_root) {
		return false;
	}

	bool node_part_of_subscene = p_node != p_edited_scene &&
			p_edited_scene->get_scene_inherited_state().is_valid() &&
			p_edited_scene->get_scene_inherited_state()->find_node_by_path(p_edited_scene->get_path_to(p_node)) >= 0 &&
			// It's important to process added nodes from the base scene in the inherited scene as
			// additional nodes to ensure they do not disappear on reload.
			// When p_reimported_root == p_edited_scene that means the edited scene
			// is the reimported scene, in that case the node is in the root base scene,
			// so it's not an addition, otherwise, the node would be added twice on reload.
			(p_node->get_owner() != p_edited_scene || p_reimported_root == p_edited_scene);

	if (node_part_of_subscene) {
		return false;
	}

	// Loop through the owners until either we reach the root node or nullptr
	Node *valid_node_owner = p_node->get_owner();
	while (valid_node_owner) {
		if (valid_node_owner == p_reimported_root) {
			break;
		}
		valid_node_owner = valid_node_owner->get_owner();
	}

	// When the owner is the imported scene and the owner is also the edited scene,
	// that means the node was added in the current edited scene.
	// We can be sure here because if the node that the node does not come from
	// the base scene because we checked just over with 'get_scene_inherited_state()->find_node_by_path'.
	if (valid_node_owner == p_reimported_root && p_reimported_root != p_edited_scene) {
		return false;
	}

	return true;
}

void EditorNode::replace_history_reimported_nodes(Node *p_original_root_node, Node *p_new_root_node, Node *p_node) {
	NodePath scene_path_to_node = p_original_root_node->get_path_to(p_node);
	Node *new_node = p_new_root_node->get_node_or_null(scene_path_to_node);
	if (new_node) {
		_editor_history.replace_object(p_node->get_instance_id(), new_node->get_instance_id());
	} else {
		_editor_history.replace_object(p_node->get_instance_id(), ObjectID());
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		replace_history_reimported_nodes(p_original_root_node, p_new_root_node, p_node->get_child(i));
	}
}

bool EditorNode::is_scene_open(const String &p_path) {
	for (int i = 0; i < _editor_data.get_edited_scene_count(); i++) {
		if (_editor_data.get_scene_path(i) == p_path) {
			return true;
		}
	}

	return false;
}

bool EditorNode::is_multi_window_enabled() const {
	return !SceneTree::get_singleton()->get_root()->is_embedding_subwindows() && !EDITOR_GET("interface/editor/display/single_window_mode") && EDITOR_GET("interface/multi_window/enable");
}

void EditorNode::setup_built_in_resource(const Ref<Resource> &r_resource, const String &p_owner_path) {
	String resource_class;
	const Ref<MissingResource> &missing_resource = r_resource;
	if (missing_resource.is_valid()) {
		resource_class = missing_resource->get_original_class();
	} else {
		resource_class = r_resource->get_class();
	}

	String unique_id;
	String final_path;
	while (true) {
		unique_id = resource_class + "_" + Resource::generate_scene_unique_id();
		final_path = p_owner_path + "::" + unique_id;

		if (!ResourceCache::has(final_path)) {
			break;
		}
	}
	r_resource->set_scene_unique_id(unique_id);
	r_resource->set_path(final_path);
}

void EditorNode::setup_color_picker(ColorPicker *r_picker) {
	r_picker->set_editor_settings(EditorSettings::get_singleton());
	int default_color_mode = EditorSettings::get_singleton()->get_project_metadata("color_picker", "color_mode", EDITOR_GET("interface/inspector/default_color_picker_mode"));
	int picker_shape = EditorSettings::get_singleton()->get_project_metadata("color_picker", "picker_shape", EDITOR_GET("interface/inspector/default_color_picker_shape"));
	bool show_intensity = EDITOR_GET("interface/inspector/color_picker_show_intensity");

	r_picker->set_color_mode((ColorPicker::ColorModeType)default_color_mode);
	r_picker->set_picker_shape((ColorPicker::PickerShapeType)picker_shape);
	r_picker->set_edit_intensity(show_intensity);

	r_picker->set_quick_open_callback(callable_mp(this, &EditorNode::_palette_quick_open_dialog));
	r_picker->set_palette_saved_callback(callable_mp(EditorFileSystem::get_singleton(), &EditorFileSystem::update_file));
	_palette_file_selected_callback = callable_mp(r_picker, &ColorPicker::_quick_open_palette_file_selected);
}

void EditorNode::register_hdr_viewport(Viewport *r_viewport) {
	const bool hdr_requested = GLOBAL_GET("display/window/hdr/request_hdr_output");
	const bool hdr_2d_enabled = GLOBAL_GET("rendering/viewport/hdr_2d");
	r_viewport->set_use_hdr_2d(hdr_2d_enabled || hdr_requested);

	_hdr_viewports.push_back(r_viewport->get_instance_id());
}

void EditorNode::request_instantiate_scenes(const Vector<String> &p_files) {
	HierarchyDock::get_singleton()->instantiate_scenes(p_files);
}

void EditorNode::handle_dropped_files(const Vector<String> &p_files) {
	String to_path = FileSystemDock::get_singleton()->get_folder_path_at_mouse_position();
	if (to_path.is_empty()) {
		to_path = FileSystemDock::get_singleton()->get_current_directory();
	}
	to_path = ProjectSettings::get_singleton()->globalize_path(to_path);

	_add_dropped_files_recursive(p_files, to_path);

	EditorFileSystem::get_singleton()->scan_changes();
}

void EditorNode::notify_all_debug_sessions_exited() {
	_project_run_bar->stop_playing();
}

ProcessID EditorNode::has_child_process(ProcessID p_pid) const {
	return _project_run_bar->has_child_process(p_pid);
}

void EditorNode::stop_child_process(ProcessID p_pid) {
	_project_run_bar->stop_child_process(p_pid);
}

void EditorNode::update_preview_themes() {
	if (!_edited_scene_viewport->is_inside_tree()) {
		return; // Too early.
	}

	const CanvasItemEditor::ThemePreviewMode mode = CanvasItemEditor::get_singleton()->get_theme_preview();
	ThemeDB *theme_db = ThemeDB::get_singleton();
	const Ref<Theme> default_theme = theme_db->get_default_theme();
	const Ref<Theme> selected_theme = mode == CanvasItemEditor::ThemePreviewMode::EDITOR ? get_editor_theme() : theme_db->get_project_theme();
	Vector<Ref<Theme>> preview_themes;
	if (selected_theme != default_theme) {
		preview_themes.push_back(selected_theme);
	}
	preview_themes.push_back(default_theme);

	ThemeContext *preview_context = theme_db->get_theme_context(_edited_scene_viewport);
	if (preview_context) {
		preview_context->set_themes(preview_themes);
	} else {
		theme_db->create_theme_context(_edited_scene_viewport, preview_themes);
	}
}

Ref<Script> EditorNode::get_object_custom_type_base(const Object *p_object) const {
	ERR_FAIL_NULL_V(p_object, nullptr);

	const Node *node = Object::cast_to<const Node>(p_object);
	if (node && node->has_meta(SceneStringName(_custom_type_script))) {
		return PropertyUtils::get_custom_type_script(node);
	}

	Ref<Script> scr = p_object->get_script();

	if (scr.is_valid()) {
		// Uncommenting would break things! Consider adding a parameter if you need it.
		// StringName name = EditorNode::get_editor_data().script_class_get_name(base_script->get_path());
		// if (name != StringName()) {
		// 	return name;
		// }

		// TODO: Should probably be deprecated in 4.x
		StringName base = scr->get_instance_base_type();
		if (base != StringName() && EditorNode::get_editor_data().get_custom_types().has(base)) {
			const Vector<EditorData::CustomType> &types = EditorNode::get_editor_data().get_custom_types()[base];

			Ref<Script> base_scr = scr;
			while (base_scr.is_valid()) {
				for (int i = 0; i < types.size(); ++i) {
					if (types[i].script == base_scr) {
						return types[i].script;
					}
				}
				base_scr = base_scr->get_base_script();
			}
		}
	}

	return nullptr;
}

StringName EditorNode::get_object_custom_type_name(const Object *p_object) const {
	ERR_FAIL_NULL_V(p_object, StringName());

	Ref<Script> scr = p_object->get_script();
	if (scr.is_null() && Object::cast_to<Script>(p_object)) {
		scr = p_object;
	}

	if (scr.is_valid()) {
		Ref<Script> base_scr = scr;
		while (base_scr.is_valid()) {
			StringName name = EditorNode::get_editor_data().script_class_get_name(base_scr->get_path());
			if (name != StringName()) {
				return name;
			}

			// TODO: Should probably be deprecated in 4.x.
			StringName base = base_scr->get_instance_base_type();
			if (base != StringName() && EditorNode::get_editor_data().get_custom_types().has(base)) {
				const Vector<EditorData::CustomType> &types = EditorNode::get_editor_data().get_custom_types()[base];
				for (int i = 0; i < types.size(); ++i) {
					if (types[i].script == base_scr) {
						return types[i].name;
					}
				}
			}
			base_scr = base_scr->get_base_script();
		}
	}

	return StringName();
}

Ref<Texture2D> EditorNode::get_object_icon(const Object *p_object, const String &p_fallback) {
	ERR_FAIL_NULL_V_MSG(p_object, nullptr, "Object cannot be null.");

	Ref<Script> scr = p_object->get_script();

	const EditorDebuggerRemoteObjects *robjs = Object::cast_to<EditorDebuggerRemoteObjects>(p_object);
	if (robjs) {
		String class_name;
		if (scr.is_valid()) {
			class_name = scr->get_global_name();

			if (class_name.is_empty()) {
				// If there is no class_name in this script we just take the script path.
				class_name = scr->get_path();
			}
		}

		if (class_name.is_empty()) {
			return get_class_icon(robjs->type_name, p_fallback);
		}

		return get_class_icon(class_name, p_fallback);
	}

	if (scr.is_null() && p_object->is_class("Script")) {
		scr = p_object;
	}

	if (Object::cast_to<MultiNodeEdit>(p_object)) {
		return get_class_icon(Object::cast_to<MultiNodeEdit>(p_object)->get_edited_class_name(), p_fallback);
	} else {
		return _get_class_or_script_icon(p_object->get_class(), scr.is_valid() ? scr->get_path() : String(), p_fallback);
	}
}

Ref<Texture2D> EditorNode::get_class_icon(const String &p_class, const String &p_fallback) {
	ERR_FAIL_COND_V_MSG(p_class.is_empty(), nullptr, "Class name cannot be empty.");
	const Pair<String, String> key(p_class, p_fallback);

	// Take from the local cache, if available.
	{
		Ref<Texture2D> *icon = _class_icon_cache.getptr(key);
		if (icon) {
			return *icon;
		}
	}

	String script_path;
	if (ScriptServer::is_global_class(p_class)) {
		script_path = ScriptServer::get_global_class_path(p_class);
	} else if (!p_class.get_extension().is_empty() && ResourceLoader::exists(p_class)) { // If the script is not a class_name we check if the script resource exists.
		script_path = p_class;
	}

	Ref<Texture2D> icon = _get_class_or_script_icon(p_class, script_path, p_fallback, true);
	_class_icon_cache[key] = icon;
	return icon;
}

bool EditorNode::is_object_of_custom_type(const Object *p_object, const StringName &p_class) {
	ERR_FAIL_NULL_V(p_object, false);

	Ref<Script> scr = p_object->get_script();
	if (scr.is_null() && Object::cast_to<Script>(p_object)) {
		scr = p_object;
	}

	if (scr.is_valid()) {
		Ref<Script> base_script = scr;
		while (base_script.is_valid()) {
			StringName name = EditorNode::get_editor_data().script_class_get_name(base_script->get_path());
			if (name == p_class) {
				return true;
			}
			base_script = base_script->get_base_script();
		}
	}
	return false;
}

void EditorNode::show_save_accept(const String &p_text, const String &p_ok_text) {
	_current_menu_option = -1;
	if (_save_message_dialog) {
		_close_save_scene_progress();
		_save_message_dialog->set_ok_button_text(p_ok_text);
		_save_message_dialog->set_text(p_text);
		_save_message_dialog->reset_size();
		EditorInterface::get_singleton()->popup_dialog_centered_clamped(_save_message_dialog, Size2i(), 0.0);
	}
}

void EditorNode::show_warning(const String &p_text, const String &p_title) {
	if (_warning_dialog) {
		_close_save_scene_progress();
		_warning_dialog->set_text(p_text);
		_warning_dialog->set_title(p_title);
		_warning_dialog->reset_size();
		EditorInterface::get_singleton()->popup_dialog_centered_clamped(_warning_dialog, Size2i(), 0.0);
	} else {
		WARN_PRINT(p_title + " " + p_text);
	}
}

Error EditorNode::export_preset(const String &p_preset, const String &p_path, bool p_debug, bool p_pack_only, bool p_android_build_template, bool p_patch, const Vector<String> &p_patches) {
	_deferred_export.preset = p_preset;
	_deferred_export.path = p_path;
	_deferred_export.debug = p_debug;
	_deferred_export.pack_only = p_pack_only;
	_deferred_export.android_build_template = p_android_build_template;
	_deferred_export.patch = p_patch;
	_deferred_export.patches = p_patches;
	_cmdline_mode = true;
	return OK;
}

bool EditorNode::close_scene() {
	int tab_index = _editor_data.get_edited_scene();
	if (tab_index == 0 && get_edited_scene() == nullptr && _editor_data.get_scene_path(tab_index).is_empty()) {
		return false;
	}

	_tab_closing_idx = tab_index;
	_current_menu_option = SCENE_CLOSE;
	_discard_changes();
	_changing_scene = false;
	return true;
}

void EditorNode::save_editor_layout_delayed() {
	_editor_layout_save_delay_timer->start();
}

void EditorNode::save_default_environment() {
	Ref<Environment> fallback = get_tree()->get_root()->get_world_3d()->get_fallback_environment();

	if (fallback.is_valid() && fallback->get_path().is_resource_file()) {
		HashMap<Ref<Resource>, bool> processed;
		_find_and_save_edited_subresources(fallback.ptr(), processed, 0);
		save_resource_in_path(fallback, fallback->get_path());
	}
}

void EditorNode::open_export_template_manager() {
	_export_template_manager->popup_manager();
}

void EditorNode::reload_scene(const String &p_path) {
	int scene_idx = -1;

	const String lpath = ProjectSettings::get_singleton()->localize_path(p_path);
	for (int i = 0; i < _editor_data.get_edited_scene_count(); i++) {
		if (_editor_data.get_scene_path(i) == lpath) {
			scene_idx = i;
			break;
		}
	}
	ERR_FAIL_COND_MSG(scene_idx == -1, vformat("Can't reload scene %s, as it's not opened.", p_path));

	int current_tab = _editor_data.get_edited_scene();
	bool is_current_scene = current_tab == scene_idx;
	if (is_current_scene) {
		_editor_data.apply_changes_in_editors();
	}

	// Reload scene.
	_remove_scene(scene_idx, false);
	Error err = load_scene(p_path, true, false, true, false);
	if (err != OK) {
		return;
	}

	// Adjust index so tab is back a the previous position.
	_editor_data.move_scene_to_index(_editor_data.get_edited_scene_count() - 1, scene_idx);
	EditorUndoRedoManager::get_singleton()->clear_history(_editor_data.get_scene_history_id(scene_idx), false);

	// Recover the current tab.
	if (is_current_scene) {
		_apply_edited_scene(current_tab, true);
	} else {
		_editor_data.set_edited_scene(current_tab);
		_scene_tabs->update_scene_tabs();
	}
}

void EditorNode::find_all_instances_inheriting_path_in_node(Node *p_root, Node *p_node, const String &p_instance_path, HashSet<Node *> &r_instance_list) {
	bool valid_instance_found = false;

	// Attempt to find all the instances matching path we're going to reload.
	if (p_node->get_scene_file_path() == p_instance_path) {
		valid_instance_found = true;
	} else {
		Node *current_node = p_node;

		Ref<SceneState> inherited_state = current_node->get_scene_inherited_state();
		while (inherited_state.is_valid()) {
			String inherited_path = inherited_state->get_path();
			if (inherited_path == p_instance_path) {
				valid_instance_found = true;
				break;
			}

			inherited_state = inherited_state->get_base_scene_state();
		}
	}

	// Instead of adding this instance directly, if its not owned by the scene, walk its ancestors
	// and find the first node still owned by the scene. This is what we will reloading instead.
	if (valid_instance_found) {
		Node *current_node = p_node;
		while (true) {
			if (current_node->get_owner() == p_root || current_node->get_owner() == nullptr) {
				r_instance_list.insert(current_node);
				break;
			}
			current_node = current_node->get_parent();
		}
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		find_all_instances_inheriting_path_in_node(p_root, p_node->get_child(i), p_instance_path, r_instance_list);
	}
}

void EditorNode::preload_reimporting_with_path_in_edited_scenes(const List<String> &p_scenes) {
	EditorProgress progress("preload_reimporting_scene", TTR("Preparing scenes for reload"), _editor_data.get_edited_scene_count());

	int original_edited_scene_idx = _editor_data.get_edited_scene();

	// Walk through each opened scene to get a global list of all instances which match
	// the current reimported scenes.
	for (int current_scene_idx = 0; current_scene_idx < _editor_data.get_edited_scene_count(); current_scene_idx++) {
		progress.step(vformat(TTR("Analyzing scene %s"), _editor_data.get_scene_title(current_scene_idx)), current_scene_idx);

		Node *edited_scene_root = _editor_data.get_edited_scene_root(current_scene_idx);

		if (edited_scene_root) {
			SceneModificationsEntry scene_modifications;

			for (const String &instance_path : p_scenes) {
				if (_editor_data.get_scene_path(current_scene_idx) == instance_path) {
					continue;
				}

				HashSet<Node *> instances_to_reimport;
				find_all_instances_inheriting_path_in_node(edited_scene_root, edited_scene_root, instance_path, instances_to_reimport);
				if (instances_to_reimport.size() > 0) {
					_editor_data.set_edited_scene(current_scene_idx);

					List<Node *> instance_list_with_children;
					for (Node *original_node : instances_to_reimport) {
						InstanceModificationsEntry instance_modifications;

						// Fetching all the modified properties of the nodes reimported scene.
						get_preload_scene_modification_table(edited_scene_root, original_node, original_node, instance_modifications);

						instance_modifications.original_node = original_node;
						instance_modifications.instance_path = instance_path;
						scene_modifications.instance_list.push_back(instance_modifications);

						instance_list_with_children.push_back(original_node);
						get_children_nodes(original_node, instance_list_with_children);
					}

					// Search the scene to find nodes that references the nodes will be recreated.
					get_preload_modifications_reference_to_nodes(edited_scene_root, edited_scene_root, instances_to_reimport, instance_list_with_children, scene_modifications.other_instances_modifications);
				}
			}

			if (scene_modifications.instance_list.size() > 0) {
				_scenes_modification_table[current_scene_idx] = scene_modifications;
			}
		}
	}

	_editor_data.set_edited_scene(original_edited_scene_idx);

	progress.step(TTR("Preparation done."), _editor_data.get_edited_scene_count());
}

void EditorNode::reload_instances_with_path_in_edited_scenes() {
	if (_scenes_modification_table.is_empty()) {
		return;
	}
	EditorProgress progress("reloading_scene", TTR("Scenes reloading"), _editor_data.get_edited_scene_count());
	progress.step(TTR("Reloading..."), 0, true);

	Error err;
	Array replaced_nodes;
	HashMap<String, Ref<PackedScene>> local_scene_cache;

	// Reload the new instances.
	for (KeyValue<int, SceneModificationsEntry> &scene_modifications_elem : _scenes_modification_table) {
		for (InstanceModificationsEntry instance_modifications : scene_modifications_elem.value.instance_list) {
			if (!local_scene_cache.has(instance_modifications.instance_path)) {
				Ref<PackedScene> instance_scene_packed_scene = ResourceLoader::load(instance_modifications.instance_path, "", ResourceFormatLoader::CACHE_MODE_REPLACE, &err);

				ERR_FAIL_COND(err != OK);
				ERR_FAIL_COND(instance_scene_packed_scene.is_null());

				local_scene_cache[instance_modifications.instance_path] = instance_scene_packed_scene;
			}
		}
	}

	// Save the current scene state/selection in case of lost.
	Dictionary editor_state = _get_main_scene_state();
	_editor_data.save_edited_scene_state(_editor_selection, &_editor_history, editor_state);
	_editor_selection->clear();

	int original_edited_scene_idx = _editor_data.get_edited_scene();

	for (KeyValue<int, SceneModificationsEntry> &scene_modifications_elem : _scenes_modification_table) {
		// Set the current scene.
		int current_scene_idx = scene_modifications_elem.key;
		SceneModificationsEntry *scene_modifications = &scene_modifications_elem.value;

		_editor_data.set_edited_scene(current_scene_idx);
		Node *current_edited_scene = _editor_data.get_edited_scene_root(current_scene_idx);

		// Make sure the node is in the tree so that editor_selection can add node smoothly.
		if (original_edited_scene_idx != current_scene_idx) {
			// Prevent scene roots with the same name from being in the tree at the same time.
			Node *original_edited_scene_root = _editor_data.get_edited_scene_root(original_edited_scene_idx);
			if (original_edited_scene_root && original_edited_scene_root->get_name() == current_edited_scene->get_name()) {
				_edited_scene_viewport->remove_child(original_edited_scene_root);
			}
			_edited_scene_viewport->add_child(current_edited_scene);
		}

		// Restore the state so that the selection can be updated.
		editor_state = _editor_data.restore_edited_scene_state(_editor_selection, &_editor_history);

		int current_history_id = _editor_data.get_current_edited_scene_history_id();
		bool is_unsaved = EditorUndoRedoManager::get_singleton()->is_history_unsaved(current_history_id);

		// Clear the history for this affected tab.
		EditorUndoRedoManager::get_singleton()->clear_history(current_history_id, false);

		// Update the version
		_editor_data.is_scene_changed(current_scene_idx);

		for (InstanceModificationsEntry instance_modifications : scene_modifications->instance_list) {
			Node *original_node = instance_modifications.original_node;
			String original_node_file_path = original_node->get_scene_file_path();
			Ref<PackedScene> instance_scene_packed_scene = local_scene_cache[instance_modifications.instance_path];

			// Load a replacement scene for the node.
			Ref<PackedScene> current_packed_scene;
			Ref<PackedScene> base_packed_scene;
			if (original_node_file_path == instance_modifications.instance_path) {
				// If the node file name directly matches the scene we're replacing,
				// just load it since we already cached it.
				current_packed_scene = instance_scene_packed_scene;
			} else {
				// Otherwise, check the inheritance chain, reloading and caching any scenes
				// we require along the way.
				List<String> required_load_paths;

				// Do we need to check if the paths are empty?
				if (!original_node_file_path.is_empty()) {
					required_load_paths.push_front(original_node_file_path);
				}
				Ref<SceneState> inherited_state = original_node->get_scene_inherited_state();
				while (inherited_state.is_valid()) {
					String inherited_path = inherited_state->get_path();
					// Do we need to check if the paths are empty?
					if (!inherited_path.is_empty()) {
						required_load_paths.push_front(inherited_path);
					}
					inherited_state = inherited_state->get_base_scene_state();
				}

				// Ensure the inheritance chain is loaded in the correct order so that cache can
				// be properly updated.
				for (String path : required_load_paths) {
					if (current_packed_scene.is_valid()) {
						base_packed_scene = current_packed_scene;
					}
					if (!local_scene_cache.find(path)) {
						current_packed_scene = ResourceLoader::load(path, "", ResourceFormatLoader::CACHE_MODE_REPLACE, &err);
						local_scene_cache[path] = current_packed_scene;
					} else {
						current_packed_scene = local_scene_cache[path];
					}
				}
			}

			ERR_FAIL_COND(current_packed_scene.is_null());

			// Instantiate early so that caches cleared on load in SceneState can be rebuilt early.
			Node *instantiated_node = nullptr;

			// If we are in a inherited scene, it's easier to create a new base scene and
			// grab the node from there.
			// When scene_path_to_node is '.' and we have scene_inherited_state, it's because
			// it's a multi-level inheritance scene. We should use
			NodePath scene_path_to_node = current_edited_scene->get_path_to(original_node);
			Ref<SceneState> scene_state = current_edited_scene->get_scene_inherited_state();
			if (String(scene_path_to_node) != "." && scene_state.is_valid() && scene_state->get_path() != instance_modifications.instance_path && scene_state->find_node_by_path(scene_path_to_node) >= 0) {
				Node *root_node = scene_state->instantiate(SceneState::GenEditState::GEN_EDIT_STATE_INSTANCE);
				instantiated_node = root_node->get_node(scene_path_to_node);

				if (instantiated_node) {
					if (instantiated_node->get_parent()) {
						// Remove from the root so we can delete it from memory.
						instantiated_node->get_parent()->remove_child(instantiated_node);
						// No need of the additional children that could have been added to the node
						// in the base scene. That will be managed by the 'addition_list' later.
						_remove_all_not_owned_children(instantiated_node, instantiated_node);
						memdelete(root_node);
					}
				} else {
					// Should not happen because we checked with find_node_by_path before, just in case.
					memdelete(root_node);
				}
			}

			if (!instantiated_node) {
				// If no base scene was found to create the node, we will use the reimported packed scene directly.
				// But, when the current edited scene is the reimported scene, it's because it's an inherited scene
				// derived from the reimported scene. In that case, we will not instantiate current_packed_scene, because
				// we would reinstantiate ourself. Using the base scene is better.
				if (current_edited_scene == original_node) {
					if (base_packed_scene.is_valid()) {
						instantiated_node = base_packed_scene->instantiate(PackedScene::GEN_EDIT_STATE_INSTANCE);
					} else {
						instantiated_node = instance_scene_packed_scene->instantiate(PackedScene::GEN_EDIT_STATE_INSTANCE);
					}
				} else {
					instantiated_node = current_packed_scene->instantiate(PackedScene::GEN_EDIT_STATE_INSTANCE);
				}
			}
			ERR_FAIL_NULL(instantiated_node);

			// Disconnect all relevant connections, all connections from and persistent connections to.
			for (const KeyValue<NodePath, ModificationNodeEntry> &modification_table_entry : instance_modifications.modifications) {
				for (Connection conn : modification_table_entry.value.connections_from) {
					conn.signal.get_object()->disconnect(conn.signal.get_name(), conn.callable);
				}
				for (ConnectionWithNodePath cwnp : modification_table_entry.value.connections_to) {
					Connection conn = cwnp.connection;
					if (conn.flags & CONNECT_PERSIST) {
						conn.signal.get_object()->disconnect(conn.signal.get_name(), conn.callable);
					}
				}
			}

			// Store all the paths for any selected nodes which are ancestors of the node we're replacing.
			List<NodePath> selected_node_paths;
			for (Node *selected_node : _editor_selection->get_top_selected_node_list()) {
				if (selected_node == original_node || original_node->is_ancestor_of(selected_node)) {
					selected_node_paths.push_back(original_node->get_path_to(selected_node));
					_editor_selection->remove_node(selected_node);
				}
			}

			// Remove all nodes which were added as additional elements (they will be restored later).
			for (AdditiveNodeEntry additive_node_entry : instance_modifications.addition_list) {
				Node *addition_node = additive_node_entry.node;
				addition_node->get_parent()->remove_child(addition_node);
			}

			// Clear ownership of the nodes (kind of hack to workaround an issue with
			// replace_by when called on nodes in other tabs).
			List<Node *> nodes_owned_by_original_node;
			original_node->get_owned_by(original_node, &nodes_owned_by_original_node);
			for (Node *owned_node : nodes_owned_by_original_node) {
				owned_node->set_owner(nullptr);
			}

			// Replace the old nodes in the history with the new ones.
			// Otherwise, the history will contain old nodes, and some could still be
			// instantiated if used elsewhere, causing the "current edited item" to be
			// linked to a node that will be destroyed later. This caused the editor to
			// crash when reimporting scenes with animations when "Editable children" was enabled.
			replace_history_reimported_nodes(original_node, instantiated_node, original_node);

			// Reset the editable instance state.
			HashMap<NodePath, SceneEditorDataEntry> scene_editor_data_table;
			Node *owner = original_node->get_owner();
			if (!owner) {
				owner = original_node;
			}

			get_scene_editor_data_for_node(owner, original_node, scene_editor_data_table);

			// The current node being reloaded may also be an additional node for another node
			// that is in the process of being reloaded.
			// Replacing the additional node with the new one prevents a crash where nodes
			// in 'addition_list' are removed from the scene tree and queued for deletion.
			for (InstanceModificationsEntry &im : scene_modifications->instance_list) {
				for (AdditiveNodeEntry &additive_node_entry : im.addition_list) {
					if (additive_node_entry.node == original_node) {
						additive_node_entry.node = instantiated_node;
					}
				}
			}

			bool original_node_scene_instance_load_placeholder = original_node->get_scene_instance_load_placeholder();

			// Delete all the remaining node children.
			while (original_node->get_child_count()) {
				Node *child = original_node->get_child(0);

				original_node->remove_child(child);
				child->queue_free();
			}

			// Update the name to match
			instantiated_node->set_name(original_node->get_name());

			// Is this replacing the edited root node?

			if (current_edited_scene == original_node) {
				// Set the instance as un inherited scene of itself.
				instantiated_node->set_scene_inherited_state(instantiated_node->get_scene_instance_state());
				instantiated_node->set_scene_instance_state(nullptr);
				instantiated_node->set_scene_file_path(original_node_file_path);
				current_edited_scene = instantiated_node;
				_editor_data.set_edited_scene_root(current_edited_scene);

				if (original_edited_scene_idx == current_scene_idx) {
					// How that the editor executes a redraw while destroying or progressing the EditorProgress,
					// it crashes when the root scene has been replaced because the edited scene
					// was freed and no longer in the scene tree.
					HierarchyDock::get_singleton()->set_edited_scene(current_edited_scene);
					if (get_tree()) {
						get_tree()->set_edited_scene_root(current_edited_scene);
					}
				}
			}

			// Replace the original node with the instantiated version.
			original_node->replace_by(instantiated_node, false);
			instantiated_node->set_unique_scene_id(original_node->get_unique_scene_id());

			// Mark the old node for deletion.
			original_node->queue_free();

			// Restore the placeholder state from the original node.
			instantiated_node->set_scene_instance_load_placeholder(original_node_scene_instance_load_placeholder);

			// Attempt to re-add all the additional nodes.
			for (AdditiveNodeEntry additive_node_entry : instance_modifications.addition_list) {
				Node *parent_node = instantiated_node->get_node_or_null(additive_node_entry.parent);

				if (!parent_node) {
					parent_node = current_edited_scene;
				}

				parent_node->add_child(additive_node_entry.node);
				parent_node->move_child(additive_node_entry.node, additive_node_entry.index);
				// If the additive node's owner was the node which got replaced, update it.
				if (additive_node_entry.owner == original_node) {
					additive_node_entry.owner = instantiated_node;
				}

				additive_node_entry.node->set_owner(additive_node_entry.owner);

				// If the parent node was lost, attempt to restore the original global transform.
				{
					Node2D *node_2d = Object::cast_to<Node2D>(additive_node_entry.node);
					if (node_2d) {
						node_2d->set_transform(additive_node_entry.transform_2d);
					}

					Node3D *node_3d = Object::cast_to<Node3D>(additive_node_entry.node);
					if (node_3d) {
						node_3d->set_transform(additive_node_entry.transform_3d);
					}
				}
			}

			// Restore the scene's editable instance and folded states.
			for (HashMap<NodePath, SceneEditorDataEntry>::Iterator scene_data_entry = scene_editor_data_table.begin(); scene_data_entry; ++scene_data_entry) {
				Node *node = owner->get_node_or_null(scene_data_entry->key);
				if (node) {
					if (owner != node) {
						owner->set_editable_instance(node, scene_data_entry->value.is_editable);
					}
					node->set_display_folded(scene_data_entry->value.is_display_folded);
				}
			}

			// Restore the selection.
			if (selected_node_paths.size()) {
				for (NodePath selected_node_path : selected_node_paths) {
					Node *selected_node = instantiated_node->get_node_or_null(selected_node_path);
					if (selected_node) {
						_editor_selection->add_node(selected_node);
					}
				}
				_editor_selection->update();
			}

			// Attempt to restore the modified properties and signals for the instantitated node and all its owned children.
			for (KeyValue<NodePath, ModificationNodeEntry> &modification_entry : instance_modifications.modifications) {
				NodePath new_current_path = modification_entry.key;
				Node *modifiable_node = instantiated_node->get_node_or_null(new_current_path);

				update_node_from_node_modification_entry(modifiable_node, modification_entry.value);
			}
			// Add the newly instantiated node to the edited scene's replaced node list.
			replaced_nodes.push_back(instantiated_node);
		}

		// Attempt to restore the modified properties and signals for the instantitated node and all its owned children.
		for (KeyValue<NodePath, ModificationNodeEntry> &modification_entry : scene_modifications->other_instances_modifications) {
			NodePath new_current_path = modification_entry.key;
			Node *modifiable_node = current_edited_scene->get_node_or_null(new_current_path);

			if (modifiable_node) {
				update_node_from_node_modification_entry(modifiable_node, modification_entry.value);
			}
		}

		if (is_unsaved) {
			EditorUndoRedoManager::get_singleton()->set_history_as_unsaved(current_history_id);
		}

		// Save the current handled scene state.
		_editor_data.save_edited_scene_state(_editor_selection, &_editor_history, editor_state);
		_editor_selection->clear();

		// Cleanup the history of the changes.
		_editor_history.cleanup_history();

		if (original_edited_scene_idx != current_scene_idx) {
			_edited_scene_viewport->remove_child(current_edited_scene);

			// Ensure the current edited scene is re-added if removed earlier because it has the same name
			// as the reimported scene. The editor could crash when reloading HierarchyDock if the current
			// edited scene is not in the scene tree.
			Node *original_edited_scene_root = _editor_data.get_edited_scene_root(original_edited_scene_idx);
			if (original_edited_scene_root && !original_edited_scene_root->get_parent()) {
				_edited_scene_viewport->add_child(original_edited_scene_root);
			}
		}
	}

	// For the whole editor, call the _notify_nodes_scene_reimported with a list of replaced nodes.
	// To inform anything that depends on them that they should update as appropriate.
	_notify_nodes_scene_reimported(this, replaced_nodes);

	_editor_data.set_edited_scene(original_edited_scene_idx);

	_editor_data.restore_edited_scene_state(_editor_selection, &_editor_history);

	progress.step(TTR("Reloading done."), _editor_data.get_edited_scene_count());
}

Dictionary EditorNode::drag_resource(const Ref<Resource> &p_res, Control *r_from) {
	Ref<Texture2D> preview;

	{
		// TODO: make proper previews
		const Ref<Texture2D> texture = _theme->get_icon(SNAME("file_big_thumb"), EditorStringName(Editor));
		if (texture.is_valid()) {
			Ref<Image> img = texture->get_image();
			if (img.is_valid()) {
				img = img->duplicate();
				img->resize(48, 48);
				preview = ImageTexture::create_from_image(img);
			}
		}
	}

	Label *resource_lbl;
	Control *drag_control = memnew(Control);

	{
		TextureRect *drag_preview = memnew(TextureRect);
		drag_preview->set_texture(preview);
		drag_control->add_child(drag_preview);

		resource_lbl = memnew(Label);
		resource_lbl->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
		if (p_res->get_path().is_resource_file()) {
			resource_lbl->set_text(p_res->get_path().get_file());
		} else if (!p_res->get_name().is_empty()) {
			resource_lbl->set_text(p_res->get_name());
		} else {
			resource_lbl->set_text(p_res->get_class());
		}
		drag_control->add_child(resource_lbl);
	}

	r_from->set_drag_preview(drag_control);

	// Measure the label after it inherits the editor theme.
	if (preview.is_valid()) {
		resource_lbl->set_position(Point2((preview->get_width() - resource_lbl->get_minimum_size().width) / 2, preview->get_height()));
	}

	Dictionary drag_data;
	drag_data["type"] = "resource";
	drag_data["resource"] = p_res;
	drag_data["from"] = r_from;

	return drag_data;
}

Dictionary EditorNode::drag_files_and_dirs(const Vector<String> &p_paths, Control *r_from) {
	bool has_folder = false;
	bool has_file = false;
	for (int i = 0; i < p_paths.size(); i++) {
		bool is_folder = p_paths[i].ends_with("/");
		has_folder |= is_folder;
		has_file |= !is_folder;
	}

	constexpr int MAX_ROWS = 6;
	const int num_rows = p_paths.size() > MAX_ROWS ? MAX_ROWS - 1 : p_paths.size(); // Don't waste a row to say "1 more file" - list it instead.
	VBoxContainer *preview_vbox = memnew(VBoxContainer);

	for (int i = 0; i < num_rows; i++) {
		HBoxContainer *file_hbox = memnew(HBoxContainer);
		preview_vbox->add_child(file_hbox);

		{
			const String &path = p_paths[i];
			const bool is_folder = path.ends_with("/");

			TextureRect *icon = memnew(TextureRect);
			icon->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
			icon->set_texture(_theme->get_icon(is_folder ? SNAME("folder_folder") : SNAME("file_file"), EditorStringName(Editor)));
			icon->set_size(Size2(16, 16));
			file_hbox->add_child(icon);

			Label *preview_lbl = memnew(Label);
			preview_lbl->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
			preview_lbl->set_text(is_folder ? path.substr(0, path.length() - 1).get_file() : path.get_file());
			file_hbox->add_child(preview_lbl);
		}
	}

	if (p_paths.size() > num_rows) {
		Label *preview_lbl = memnew(Label);
		if (has_file && has_folder) {
			preview_lbl->set_text(vformat(TTR("%d more files or folders"), p_paths.size() - num_rows));
		} else if (has_folder) {
			preview_lbl->set_text(vformat(TTR("%d more folders"), p_paths.size() - num_rows));
		} else {
			preview_lbl->set_text(vformat(TTR("%d more files"), p_paths.size() - num_rows));
		}
		preview_vbox->add_child(preview_lbl);
	}

	r_from->set_drag_preview(preview_vbox);

	Dictionary drag_data;
	drag_data["type"] = has_folder ? "files_and_dirs" : "files";
	drag_data["files"] = p_paths;
	drag_data["from"] = r_from;
	return drag_data;
}

void EditorNode::add_tool_menu_item(const String &p_name, const Callable &p_callback) {
	int idx = _tool_menu->get_item_count();
	_tool_menu->add_item(p_name, TOOLS_CUSTOM);
	_tool_menu->set_item_metadata(idx, p_callback);
}

void EditorNode::add_tool_submenu_item(const String &p_name, PopupMenu *p_submenu) {
	ERR_FAIL_NULL(p_submenu);
	ERR_FAIL_COND(p_submenu->get_parent() != nullptr);
	_tool_menu->add_submenu_node_item(p_name, p_submenu, TOOLS_CUSTOM);
}

void EditorNode::remove_tool_menu_item(const String &p_name) {
	for (int i = 0; i < _tool_menu->get_item_count(); i++) {
		if (_tool_menu->get_item_id(i) != TOOLS_CUSTOM) {
			continue;
		}

		if (_tool_menu->get_item_text(i) == p_name) {
			if (_tool_menu->get_item_submenu(i) != "") {
				Node *n = _tool_menu->get_node(_tool_menu->get_item_submenu(i));
				_tool_menu->remove_child(n);
				memdelete(n);
			}
			_tool_menu->remove_item(i);
			_tool_menu->reset_size();
			return;
		}
	}
}

PopupMenu *EditorNode::get_export_as_menu() {
	return _export_as_menu;
}

void EditorNode::save_all_scenes() {
	_project_run_bar->stop_playing();
	_save_all_scenes();
}

void EditorNode::save_scene_if_open(const String &p_scene_path) {
	int idx = _editor_data.get_edited_scene_from_path(p_scene_path);
	if (idx >= 0) {
		_save_scene(p_scene_path, idx);
	}
}

void EditorNode::save_scene_list(const HashSet<String> &p_scene_paths) {
	for (int i = 0; i < _editor_data.get_edited_scene_count(); i++) {
		Node *scene = _editor_data.get_edited_scene_root(i);

		if (scene && p_scene_paths.has(scene->get_scene_file_path())) {
			_save_scene(scene->get_scene_file_path(), i);
		}
	}
}

void EditorNode::save_before_run() {
	_current_menu_option = SAVE_AND_RUN;
	_menu_option_confirm(SCENE_SAVE_AS_SCENE, true);
	_scene_resource_file_dialog->set_title(TTR("Save scene before running..."));
}

void EditorNode::try_autosave() {
	if (!bool(EDITOR_GET("run/auto_save/save_before_running"))) {
		return;
	}

	if (_unsaved_cache) {
		Node *scene = _editor_data.get_edited_scene_root();

		if (scene && !scene->get_scene_file_path().is_empty()) { // Only autosave if there is a scene and if it has a path.
			_save_scene_with_preview(scene->get_scene_file_path());
		}
	}
	_menu_option(SCENE_SAVE_ALL_SCENES);
	_editor_data.save_editor_external_data();
}

void EditorNode::restart_editor(bool p_goto_hub) {
	_menu_option_confirm(p_goto_hub ? PROJECT_QUIT_TO_HUB : PROJECT_RELOAD_CURRENT_PROJECT, false);
}

void EditorNode::open_setting_override(const String &p_property) {
	_editor_settings_dialog->hide();
	_project_settings_editor->popup_for_override(p_property);
}

void EditorNode::notify_settings_overrides_changed() {
	_settings_overrides_changed = true;
}

void EditorNode::dim_editor(bool p_dimming) {
	_dimmed = p_dimming;
	const Color modulate = p_dimming ? Color(0.5, 0.5, 0.5) : Color(1, 1, 1);
	_main_layout_pc->set_modulate(modulate);
	_editor_overlay->set_modulate(modulate);
}

bool EditorNode::has_scenes_in_session() {
	if (!bool(EDITOR_GET("interface/scene_tabs/restore_scenes_on_load"))) {
		return false;
	}
	Ref<ConfigFile> config;
	config.instantiate();
	Error err = config->load(EditorPaths::get_singleton()->get_project_settings_dir().path_join("editor_layout.cfg"));
	if (err != OK) {
		return false;
	}
	if (!config->has_section(EDITOR_NODE_CONFIG_SECTION) || !config->has_section_key(EDITOR_NODE_CONFIG_SECTION, "open_scenes")) {
		return false;
	}
	Array scenes = config->get_value(EDITOR_NODE_CONFIG_SECTION, "open_scenes");
	return !scenes.is_empty();
}

void EditorNode::undo() {
	_menu_option_confirm(SCENE_UNDO, true);
}

void EditorNode::redo() {
	_menu_option_confirm(SCENE_REDO, true);
}

int EditorNode::execute_and_show_output(const String &p_title, const String &p_path, const List<String> &p_arguments, bool p_close_on_ok, bool p_close_on_errors, String *r_output) {
	if (_execute_output_dialog) {
		_execute_output_dialog->set_title(p_title);
		_execute_output_dialog->get_ok_button()->set_disabled(true);
		_process_output_log->clear();
		_process_output_log->set_scroll_follow(true);
		EditorInterface::get_singleton()->popup_dialog_centered_ratio(_execute_output_dialog);
	}

	ExecuteThreadArgs execution;
	execution.path = p_path;
	execution.args = p_arguments;
	execution.exit_code = 255;

	int prev_len = 0;

	execution.execute_output_thread.start(_execute_thread, &execution);

	while (!execution.done.is_set()) {
		{
			MutexLock lock(execution.execute_output_mutex);
			if (prev_len != execution.output.length()) {
				String to_add = execution.output.substr(prev_len);
				prev_len = execution.output.length();
				_process_output_log->add_text(to_add);
				DisplayServer::get_singleton()->process_events(); // Get rid of pending events.
				Main::iteration();
			}
		}
		OS::get_singleton()->delay_usec(1000);
	}

	execution.execute_output_thread.wait_to_finish();
	_process_output_log->add_text("\nExit Code: " + itos(execution.exit_code));

	if (_execute_output_dialog) {
		if (p_close_on_errors && execution.exit_code != 0) {
			_execute_output_dialog->hide();
		}
		if (p_close_on_ok && execution.exit_code == 0) {
			_execute_output_dialog->hide();
		}

		_execute_output_dialog->get_ok_button()->set_disabled(false);
	}

	if (r_output) {
		*r_output = execution.output;
	}
	return execution.exit_code;
}

Error EditorNode::setup_android_build_template(const Ref<EditorExportPreset> &p_preset, bool p_confirmed) {
	_android_export_preset = p_preset;
	if (_export_template_manager->is_android_template_installed(p_preset)) {
		if (_export_template_manager->is_android_build_version_valid(p_preset)) {
			if (!p_confirmed) {
				// The setup is valid, maybe the user wants to delete and reinstall it...
				_android_remove_build_templates(false);
			}
			return OK;
		}

		// If we get here, then the installed android build directory is no longer valid (e.g. editor upgrade).
		// Let's check whether we can automatically delete the build directory. If not, we return an error and let the
		// user handle the deletion.
		bool can_delete_automatically = EDITOR_GET("export/android/build/automatically_delete_build_directory");
		if (can_delete_automatically) {
			if (!p_confirmed) {
				_install_android_build_template_dialog->popup_centered();
				return ERR_UNCONFIGURED;
			}

			if (_export_template_manager->delete_android_build_directory(_android_export_preset) == OK) {
				return setup_android_build_template(_android_export_preset, p_confirmed);
			}
		}

		// If we get here, we need to prompt the user to delete the android build directory, either because we cannot do
		// it automatically, or because we tried to do it automatically and failed.
		_android_remove_build_templates(true);
		return ERR_UNCONFIGURED;
	} else if (!_export_template_manager->can_install_android_template(p_preset)) {
		_missing_android_build_template_dialog->popup_centered();
	} else {
		if (p_confirmed) {
			return _export_template_manager->install_android_template(p_preset);
		} else {
			_install_android_build_template_dialog->popup_centered();
		}
	}

	return ERR_UNCONFIGURED;
}

void EditorNode::add_resource_conversion_plugin(const Ref<EditorResourceConversionPlugin> &p_plugin) {
	_resource_conversion_plugins.push_back(p_plugin);
}

void EditorNode::remove_resource_conversion_plugin(const Ref<EditorResourceConversionPlugin> &p_plugin) {
	_resource_conversion_plugins.erase(p_plugin);
}

Vector<Ref<EditorResourceConversionPlugin>> EditorNode::find_resource_conversion_plugin_for_resource(const Ref<Resource> &p_for_resource) {
	if (p_for_resource.is_null()) {
		return Vector<Ref<EditorResourceConversionPlugin>>();
	}

	Vector<Ref<EditorResourceConversionPlugin>> ret;
	for (Ref<EditorResourceConversionPlugin> resource_conversion_plugin : _resource_conversion_plugins) {
		if (resource_conversion_plugin.is_valid() && resource_conversion_plugin->handles(p_for_resource)) {
			ret.push_back(resource_conversion_plugin);
		}
	}

	return ret;
}

Vector<Ref<EditorResourceConversionPlugin>> EditorNode::find_resource_conversion_plugin_for_type_name(const String &p_type) {
	Vector<Ref<EditorResourceConversionPlugin>> ret;

	if (ClassDB::class_exists(p_type) && ClassDB::can_instantiate(p_type)) {
		Ref<Resource> temp = Object::cast_to<Resource>(ClassDB::instantiate(p_type));
		if (temp.is_valid()) {
			for (Ref<EditorResourceConversionPlugin> resource_conversion_plugin : _resource_conversion_plugins) {
				if (resource_conversion_plugin.is_valid() && resource_conversion_plugin->handles(temp)) {
					ret.push_back(resource_conversion_plugin);
				}
			}
		}
	}

	return ret;
}

bool EditorNode::ensure_main_scene(bool p_from_native) {
	_pick_main_scene_dialog->set_meta("from_native", p_from_native); // Whether from play button or native run.
	String main_scene = GLOBAL_GET("application/run/main_scene");

	if (main_scene.is_empty()) {
		_current_menu_option = -1;
		_pick_main_scene_dialog->set_text(TTR("No main scene has ever been defined. Select one?\nYou can change it later in \"Project Settings\" under the 'application' category."));
		_pick_main_scene_dialog->popup_centered();

		if (_editor_data.get_edited_scene_root()) {
			_select_current_scene_btn->set_disabled(false);
			_select_current_scene_btn->grab_focus();
		} else {
			_select_current_scene_btn->set_disabled(true);
		}

		return false;
	}

	if (!FileAccess::exists(main_scene)) {
		_current_menu_option = -1;
		_pick_main_scene_dialog->set_text(vformat(TTR("Selected scene '%s' does not exist. Select a valid one?\nYou can change it later in \"Project Settings\" under the 'application' category."), main_scene));
		_pick_main_scene_dialog->popup_centered();
		return false;
	}

	if (ResourceLoader::get_resource_type(main_scene) != "PackedScene") {
		_current_menu_option = -1;
		_pick_main_scene_dialog->set_text(vformat(TTR("Selected scene '%s' is not a scene file. Select a valid one?\nYou can change it later in \"Project Settings\" under the 'application' category."), main_scene));
		_pick_main_scene_dialog->popup_centered();
		return false;
	}

	return true;
}

bool EditorNode::validate_custom_directory() {
	const String custom_dir = OS::get_singleton()->get_safe_dir_name(GLOBAL_GET("application/config/settings/custom_user_directory"), true);

	if (!custom_dir.is_empty()) {
		String data_dir = OS::get_singleton()->get_user_data_dir();
		Ref<DirAccess> dir = DirAccess::create(DirAccess::ACCESS_USERDATA);
		if (dir->change_dir(data_dir) != OK) {
			dir->make_dir_recursive(data_dir);
			if (dir->change_dir(data_dir) != OK) {
				_open_project_settings_dialog->set_text(vformat(TTR("User data dir '%s' is not valid. Change to a valid one?"), data_dir));
				_open_project_settings_dialog->popup_centered();
				return false;
			}
		}
	}

	return true;
}

void EditorNode::run_editor_script(const Ref<Script> &p_script) {
	Error err = p_script->reload(true); // Always hard reload the script before running.
	if (err != OK || !p_script->is_script_valid()) {
		EditorToasterContainer::get_singleton()->popup_str(TTR("Cannot run the script because it contains errors, check the output log."), EditorToasterContainer::WARNING);
		return;
	}

	// Perform additional checks on the script to evaluate if it's runnable.

	bool is_runnable = true;
	if (!ClassDB::is_parent_class(p_script->get_instance_base_type(), "EditorScript")) {
		is_runnable = false;

		EditorToasterContainer::get_singleton()->popup_str(TTR("Cannot run the script because it doesn't extend EditorScript."), EditorToasterContainer::WARNING);
	}
	if (!p_script->is_tool()) {
		is_runnable = false;

		EditorToasterContainer::get_singleton()->popup_str(TTR("Cannot run the script because it's not a tool script (add the @tool annotation at the top)."), EditorToasterContainer::WARNING);
	}
	if (!is_runnable) {
		return;
	}

	Ref<EditorScript> es = memnew(EditorScript);
	es->set_script(p_script);
	es->run();
}

bool EditorNode::is_scene_unsaved(int p_idx) {
	const Node *scene = _editor_data.get_edited_scene_root(p_idx);
	if (!scene) {
		return false;
	}

	if (EditorUndoRedoManager::get_singleton()->is_history_unsaved(_editor_data.get_scene_history_id(p_idx))) {
		return true;
	}

	const String &scene_path = scene->get_scene_file_path();
	if (!scene_path.is_empty()) {
		// Check if scene has unsaved changes in built-in resources.
		if (!_editor_data.get_unsaved_status(scene_path).is_empty()) {
			return true;
		}
	}
	return false;
}

void EditorNode::_bind_methods() {
	ClassDB::bind_method(D_METHOD("push_item", "object", "property", "inspector_only"), &EditorNode::push_item, DEFVAL(""), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("set_edited_scene", "edited_scene"), &EditorNode::set_edited_scene);
	ClassDB::bind_method(D_METHOD("stop_child_process", "pid"), &EditorNode::stop_child_process);
	ClassDB::bind_method(D_METHOD("update_node_reference", "value", "node", "remove"), &EditorNode::update_node_reference, DEFVAL(false));

	ADD_SIGNAL(MethodInfo("request_help_search"));
	ADD_SIGNAL(MethodInfo("script_add_function_request", PropertyInfo(Variant::OBJECT, "obj"), PropertyInfo(Variant::STRING, "function"), PropertyInfo(Variant::PACKED_STRING_ARRAY, "args")));
	ADD_SIGNAL(MethodInfo("resource_saved", PropertyInfo(Variant::OBJECT, "obj")));
	ADD_SIGNAL(MethodInfo("scene_saved", PropertyInfo(Variant::STRING, "path")));
	ADD_SIGNAL(MethodInfo("scene_changed"));
	ADD_SIGNAL(MethodInfo("scene_closed", PropertyInfo(Variant::STRING, "path")));
	ADD_SIGNAL(MethodInfo("preview_locale_changed"));
	ADD_SIGNAL(MethodInfo("resource_counter_changed"));
}

void EditorNode::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_TRANSLATION_CHANGED: {
			_update_title();
			callable_mp(this, &EditorNode::_titlebar_resized).call_deferred();

			EditorHelpBit::clear_cache();
		} break;

		case NOTIFICATION_POSTINITIALIZE: {
			EditorHelp::generate_doc();
#if defined(MODULE_GDSCRIPT_ENABLED)
			EditorHelpHighlighter::create_singleton();
#endif
		} break;

		case NOTIFICATION_PROCESS: {
			_update_main_layout_style();

			if (_editor_data.is_scene_changed(-1)) {
				_scene_tabs->update_scene_tabs();
			}

			_editor_selection->update();

			ResourceImporterTexture::get_singleton()->update_imports();
			ResourceImporterStreamedTexture::get_singleton()->update_imports();

			if (_requested_first_scan) {
				_requested_first_scan = false;

				OS::get_singleton()->benchmark_begin_measure("Editor", "First Scan");

				EditorFileSystem::get_singleton()->scan();
			}

			if (_settings_overrides_changed) {
				EditorSettings::get_singleton()->notify_changes();
				EditorSettings::get_singleton()->emit_signal(SNAME("settings_changed"));
				_settings_overrides_changed = false;
			}
		} break;

		case NOTIFICATION_ENTER_TREE: {
			get_tree()->set_disable_node_threading(true); // No node threading while running editor.

			Engine::get_singleton()->set_editor_hint(true);

			Window *window = get_window();
			if (window) {
				// Handle macOS fullscreen and extend-to-title changes.
				window->connect("titlebar_changed", callable_mp(this, &EditorNode::_titlebar_resized));
			}

			// Theme has already been created in the constructor, so we can skip that step.
			_update_theme(true);

			OS::get_singleton()->set_low_processor_usage_mode_sleep_usec(int(EDITOR_GET("interface/editor/timers/low_processor_mode_sleep_usec")));
			get_tree()->get_root()->set_as_audio_listener_3d(false);
			get_tree()->get_root()->set_as_audio_listener_2d(false);
			get_tree()->get_root()->set_snap_2d_transforms_to_pixel(false);
			get_tree()->get_root()->set_snap_2d_vertices_to_pixel(false);
			get_tree()->set_auto_accept_quit(false);
#ifdef ANDROID_ENABLED
			get_tree()->set_quit_on_go_back(false);
			DisplayServer::get_singleton()->connect("orientation_changed", callable_mp(this, &EditorNode::_screen_orientation_changed));
#endif
			get_tree()->get_root()->connect("files_dropped", callable_mp(this, &EditorNode::handle_dropped_files));

			_command_palette->register_shortcuts_as_command();

			_begin_first_scan();

			_last_dark_mode_state = DisplayServer::get_singleton()->is_dark_mode();
			_last_system_accent_color = DisplayServer::get_singleton()->get_accent_color();
			_last_system_base_color = DisplayServer::get_singleton()->get_base_color();
			DisplayServer::get_singleton()->set_system_theme_change_callback(callable_mp(this, &EditorNode::_check_system_theme_changed));

			get_viewport()->connect("size_changed", callable_mp(this, &EditorNode::_viewport_resized));

			/* DO NOT LOAD SCENES HERE, WAIT FOR FILE SCANNING AND REIMPORT TO COMPLETE */
		} break;

		case NOTIFICATION_EXIT_TREE: {
			_singleton->_active_plugins.clear();

			if (_progress_dialog) {
				_progress_dialog->queue_free();
			}
			if (_load_error_dialog) {
				_load_error_dialog->queue_free();
			}
			if (_execute_output_dialog) {
				_execute_output_dialog->queue_free();
			}
			if (_warning_dialog) {
				_warning_dialog->queue_free();
			}
			if (_save_message_dialog) {
				_save_message_dialog->queue_free();
			}
			EditorHelp::save_script_doc_cache();
			_editor_data.save_editor_external_data();
			EditorSettings::get_singleton()->save_project_metadata();
			FileAccess::set_file_close_fail_notify_callback(nullptr);
			_log->deinit(); // Do not get messages anymore.
			_editor_data.clear_edited_scenes();
			get_viewport()->disconnect("size_changed", callable_mp(this, &EditorNode::_viewport_resized));
		} break;

		case NOTIFICATION_READY: {
			// Store the default order of the center lower docks. It can only be determined dynamically.
			DockTabContainer *center_bottom = _editor_dock_manager->get_dock_container(EditorDock::CENTER_BOTTOM);
			center_bottom->save_docks_to_config(_default_layout, "docks");

			RenderingServer::get_singleton()->viewport_set_disable_2d(get_scene_root()->get_viewport_rid(), true);
			RenderingServer::get_singleton()->viewport_set_environment_mode(get_viewport()->get_viewport_rid(), RSE::VIEWPORT_ENVIRONMENT_DISABLED);
			DisplayServer::get_singleton()->screen_set_keep_on(EDITOR_GET("interface/editor/display/keep_screen_on"));

			_feature_profile_manager->notify_changed();

			// Save the project after opening to mark it as last modified, except in headless mode.
			// Also use this opportunity to ensure default settings are applied to new projects created from the command line
			// using `touch project.godot`.
			if (!_cmdline_mode) {
				const String project_settings_path = ProjectSettings::get_singleton()->get_resource_path().path_join("project.godot");
				// Check the file's size in bytes as an optimization. If it's under 10 bytes, the file is assumed to be empty.
				if (FileAccess::get_size(project_settings_path) < 10) {
					const HashMap<String, Variant> initial_settings = get_initial_settings();
					for (const KeyValue<String, Variant> &initial_setting : initial_settings) {
						ProjectSettings::get_singleton()->set_setting(initial_setting.key, initial_setting.value);
					}
				}
				ProjectSettings::get_singleton()->save();
			}

			_titlebar_resized();

			// Set up a theme context for the 2D preview viewport using the stored preview theme.
			update_preview_themes();

			// Remember the selected locale to preview node translations.
			const String preview_locale = EditorSettings::get_singleton()->get_project_metadata("editor_metadata", "preview_locale", String());
			if (!preview_locale.is_empty() && TranslationServer::get_singleton()->has_translation_for_locale(preview_locale, true)) {
				set_preview_locale(preview_locale, false);
			}

			if (Engine::get_singleton()->is_recovery_mode_hint()) {
				EditorToasterContainer::get_singleton()->popup_str(TTR("Recovery Mode is enabled. Editor functionality has been restricted."), EditorToasterContainer::WARNING);
			}

			/* DO NOT LOAD SCENES HERE, WAIT FOR FILE SCANNING AND REIMPORT TO COMPLETE */
		} break;

		case NOTIFICATION_APPLICATION_FOCUS_IN: {
			// Restore the original FPS cap after focusing back on the editor.
			OS::get_singleton()->set_low_processor_usage_mode_sleep_usec(int(EDITOR_GET("interface/editor/timers/low_processor_mode_sleep_usec")));

			if (_is_project_data_missing()) {
				_missing_project_data_dialog->popup_centered();
			} else {
				EditorFileSystem::get_singleton()->scan_changes();
			}
			_scan_external_changes();

		} break;

		case NOTIFICATION_APPLICATION_FOCUS_OUT: {
			// Save on focus loss before applying the FPS limit to avoid slowing down the saving process.
			if (EDITOR_GET("interface/editor/behavior/save_on_focus_loss")) {
				_save_scene_silently();
			}

			// Set a low FPS cap to decrease CPU/GPU usage while the editor is unfocused.
			if (_unfocused_low_processor_usage_mode_enabled) {
				OS::get_singleton()->set_low_processor_usage_mode_sleep_usec(int(EDITOR_GET("interface/editor/timers/unfocused_low_processor_mode_sleep_usec")));
			}
		} break;

		case NOTIFICATION_WM_ABOUT: {
			EditorDialogManager::get_singleton()->show_about();
		} break;

		case NOTIFICATION_WM_CLOSE_REQUEST: {
			_menu_option_confirm(SCENE_QUIT, false);
		} break;

		case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
			if (EditorSettings::get_singleton()->check_changed_settings_in_group("filesystem/file_dialog")) {
				FileDialog::set_default_show_hidden_files(EDITOR_GET("filesystem/file_dialog/show_hidden_files"));
				FileDialog::set_default_display_mode(EDITOR_GET("filesystem/file_dialog/display_mode"));
			}

			if (EditorSettings::get_singleton()->check_changed_settings_in_group("interface/editor/input/tablet_driver")) {
				String tablet_driver = GLOBAL_GET("input_devices/pen_tablet/driver");
				int tablet_driver_idx = EDITOR_GET("interface/editor/input/tablet_driver");
				if (tablet_driver_idx != -1) {
					tablet_driver = DisplayServer::get_singleton()->tablet_get_driver_name(tablet_driver_idx);
				}
				if (tablet_driver.is_empty()) {
					tablet_driver = DisplayServer::get_singleton()->tablet_get_driver_name(0);
				}
				DisplayServer::get_singleton()->tablet_set_current_driver(tablet_driver);
				print_verbose("Using \"" + DisplayServer::get_singleton()->tablet_get_current_driver() + "\" pen tablet driver...");
			}

			if (EDITOR_GET("interface/editor/behavior/import_resources_when_unfocused")) {
				_scan_changes_timer->start();
			} else {
				_scan_changes_timer->stop();
			}

			_follow_system_theme = EDITOR_GET("interface/theme/follow_system_theme");
			_use_system_accent_color = EDITOR_GET("interface/theme/use_system_accent_color");

			if (EditorThemeManager::is_generated_theme_outdated()) {
				_class_icon_cache.clear();
				_update_theme();
				_build_icon_type_cache();
				_recent_scenes_menu->reset_size();
			}

			if (EditorSettings::get_singleton()->check_changed_settings_in_group("interface/editor/timers/dragging_")) {
				_theme->set_constant("dragging_unfold_wait_msec", "Tree", (float)EDITOR_GET("interface/editor/timers/dragging_hover_wait_seconds") * 1000);
				_theme->set_constant("hover_switch_wait_msec", "TabBar", (float)EDITOR_GET("interface/editor/timers/dragging_hover_wait_seconds") * 1000);
			}

			if (EditorSettings::get_singleton()->check_changed_settings_in_group("interface/editor/docks")) {
				_editor_dock_manager->update_tab_styles();
			}

			if (EditorSettings::get_singleton()->check_changed_settings_in_group("interface/scene_tabs")) {
				_scene_tabs->update_scene_tabs();
			}

			if (EditorSettings::get_singleton()->check_changed_settings_in_group("docks/filesystem")) {
				HashSet<String> updated_textfile_extensions;
				HashSet<String> updated_other_file_extensions;
				bool extensions_match = true;
				const Vector<String> textfile_ext = ((String)(EDITOR_GET("docks/filesystem/textfile_extensions"))).split(",", false);
				for (const String &extension : textfile_ext) {
					updated_textfile_extensions.insert(extension);
					if (extensions_match && !_textfile_extensions.has(extension)) {
						extensions_match = false;
					}
				}
				const Vector<String> other_file_ext = ((String)(EDITOR_GET("docks/filesystem/other_file_extensions"))).split(",", false);
				for (const String &extension : other_file_ext) {
					updated_other_file_extensions.insert(extension);
					if (extensions_match && !_other_file_extensions.has(extension)) {
						extensions_match = false;
					}
				}

				if (!extensions_match || updated_textfile_extensions.size() < _textfile_extensions.size() || updated_other_file_extensions.size() < _other_file_extensions.size()) {
					_textfile_extensions = updated_textfile_extensions;
					_other_file_extensions = updated_other_file_extensions;
					EditorFileSystem::get_singleton()->scan();
				}
			}

			if (EditorSettings::get_singleton()->check_changed_settings_in_group("interface/editor/appearance")) {
				_update_main_menu_type();
			}
			if (EditorSettings::get_singleton()->check_changed_settings_in_group("interface/editor/display")) {
				_update_vsync_mode();
				DisplayServer::get_singleton()->screen_set_keep_on(EDITOR_GET("interface/editor/display/keep_screen_on"));
			}

		} break;
	}
}

void EditorNode::input(const Ref<InputEvent> &p_event) {
	// EditorNode::get_singleton()->set_process_input is set to true in ProgressDialog
	// only when the progress dialog is visible.
	// We need to discard all key events to disable all shortcuts while the progress
	// dialog is displayed, simulating an exclusive popup. Mouse events are
	// captured by a full-screen container in front of the EditorNode in ProgressDialog,
	// allowing interaction with the actual dialog where a Cancel button may be visible.
	Ref<InputEventKey> k = p_event;
	if (k.is_valid()) {
		get_tree()->get_root()->set_input_as_handled();
	}
}

void EditorNode::shortcut_input(const Ref<InputEvent> &p_event) {
	ERR_FAIL_COND(p_event.is_null());

	Ref<InputEventKey> k = p_event;
	if ((k.is_valid() && k->is_pressed() && !k->is_echo()) || Object::cast_to<InputEventShortcut>(*p_event)) {
		bool is_handled = true;
		if (ED_IS_SHORTCUT("editor/filter_files", p_event)) {
			FileSystemDock::get_singleton()->focus_on_filter();
		} else if (ED_IS_SHORTCUT("editor/editor_help", p_event)) {
			emit_signal(SNAME("request_help_search"), "");
		} else if (ED_IS_SHORTCUT("editor/editor_2d", p_event) || ED_IS_SHORTCUT("editor/editor_3d", p_event)) {
			_scene_editor_dock->set_3d_mode(ED_IS_SHORTCUT("editor/editor_3d", p_event));
			_editor_dock_manager->force_focus_dock(_scene_editor_dock);
		} else if (ED_IS_SHORTCUT("editor/command_palette", p_event)) {
			_open_command_palette();
		} else if (ED_IS_SHORTCUT("editor/toggle_selected_nodes_visibility", p_event)) {
			EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
			undo_redo->create_action(TTR("Toggle Selected Node(s) Visibility"));
			const List<Node *> &selection = _editor_selection->get_top_selected_node_list();

			for (Node *selected_node : selection) {
				Node *node_with_visibility;
				node_with_visibility = Object::cast_to<CanvasItem>(selected_node);
				if (!node_with_visibility || !node_with_visibility->is_inside_tree()) {
					node_with_visibility = Object::cast_to<Node3D>(selected_node);
					if (!node_with_visibility || !node_with_visibility->is_inside_tree()) {
						continue;
					}
				}

				undo_redo->add_do_method(node_with_visibility, "set_visible", !node_with_visibility->get("visible"));
				undo_redo->add_undo_method(node_with_visibility, "set_visible", node_with_visibility->get("visible"));
			}

			undo_redo->commit_action();
		} else {
			is_handled = false;
		}

		if (is_handled) {
			get_tree()->get_root()->set_input_as_handled();
		}
	}
}

Ref<Texture2D> EditorNode::_file_dialog_get_icon(const String &p_path) {
	EditorFileSystemDirectory *efsd = EditorFileSystem::get_singleton()->get_filesystem_path(p_path.get_base_dir());
	if (efsd) {
		String filename = p_path.get_file();
		for (int i = 0; i < efsd->get_file_count(); i++) {
			if (efsd->get_file(i) == filename) {
				String type = efsd->get_file_type(i);

				if (_singleton->_icon_type_cache.has(type)) {
					return _singleton->_icon_type_cache[type];
				} else {
					return _singleton->_icon_type_cache["Object"];
				}
			}
		}
	}

	return _singleton->_icon_type_cache["Object"];
}

Ref<Texture2D> EditorNode::_file_dialog_get_thumbnail(const String &p_path) {
	Ref<ImageTexture> texture = _singleton->_default_thumbnail->duplicate();
	EditorResourcePreview::get_singleton()->queue_resource_preview(p_path, callable_mp_static(EditorNode::_file_dialog_thumbnail_callback).bind(texture));
	return texture;
}

void EditorNode::_file_dialog_thumbnail_callback(const String &p_path, const Ref<Texture2D> &p_preview, const Ref<Texture2D> &p_small_preview, Ref<ImageTexture> r_texture) {
	ERR_FAIL_COND(r_texture.is_null());
	if (p_preview.is_valid()) {
		r_texture->set_image(p_preview->get_image());
	}
}

void EditorNode::_file_dialog_register(FileDialog *p_dialog) {
	_singleton->_file_dialogs.insert(p_dialog);
}

void EditorNode::_file_dialog_unregister(FileDialog *p_dialog) {
	_singleton->_file_dialogs.erase(p_dialog);
}

void EditorNode::_file_access_close_error_notify(const String &p_str) {
	callable_mp_static(&EditorNode::_file_access_close_error_notify_impl).call_deferred(p_str);
}

void EditorNode::_file_access_close_error_notify_impl(const String &p_str) {
	add_io_error(vformat(TTR("Unable to write to file '%s', file in use, locked or lacking permissions."), p_str));
}

void EditorNode::_print_handler(void *p_this, const String &p_string, bool p_error, bool p_rich) {
	if (!Thread::is_main_thread()) {
		callable_mp_static(&EditorNode::_print_handler_impl).call_deferred(p_string, p_error, p_rich);
	} else {
		_print_handler_impl(p_string, p_error, p_rich);
	}
}

void EditorNode::_print_handler_impl(const String &p_string, bool p_error, bool p_rich) {
	if (!_singleton) {
		return;
	}
	if (p_error) {
		_singleton->_log->add_message(p_string, EditorLog::MSG_TYPE_ERROR);
	} else if (p_rich) {
		_singleton->_log->add_message(p_string, EditorLog::MSG_TYPE_STD_RICH);
	} else {
		_singleton->_log->add_message(p_string, EditorLog::MSG_TYPE_STD);
	}
}

void EditorNode::_resource_saved(Ref<Resource> p_resource, const String &p_path) {
	if (_singleton->_saving_resources_in_path.has(p_resource)) {
		// This is going to be handled by save_resource_in_path when the time is right.
		return;
	}

	if (EditorFileSystem::get_singleton()) {
		EditorFileSystem::get_singleton()->update_file(p_path);
	}

	_singleton->_editor_folding.save_resource_folding(p_resource, p_path);
}

void EditorNode::_resource_loaded(Ref<Resource> p_resource, const String &p_path) {
	_singleton->_editor_folding.load_resource_folding(p_resource, p_path);
}

void EditorNode::_update_main_menu_icons() {
	_file_menu->set_item_icon(_file_menu->get_item_index(SCENE_NEW_SCENE), _theme->get_icon(SNAME("action_create_new_scene_from"), EditorStringName(Editor)));
	_file_menu->set_item_icon(_file_menu->get_item_index(SCENE_OPEN_SCENE), _theme->get_icon(SNAME("resource_packed_scene"), EditorStringName(Editor)));
	_file_menu->set_item_icon(_file_menu->get_item_index(SCENE_SAVE_SCENE), _theme->get_icon(SNAME("action_save"), EditorStringName(Editor)));
	_file_menu->set_item_icon(_file_menu->get_item_index(SCENE_QUICK_OPEN), _theme->get_icon(SNAME("action_load"), EditorStringName(Editor)));
	_file_menu->set_item_icon(_file_menu->get_item_index(SCENE_CLOSE), _theme->get_icon(SNAME("action_close_scene"), EditorStringName(Editor)));
	_file_menu->set_item_icon(_file_menu->get_item_index(SCENE_QUIT), _theme->get_icon(SNAME("action_close"), EditorStringName(Editor)));

	_project_menu->set_item_icon(_project_menu->get_item_index(PROJECT_OPEN_PLUGINS), _theme->get_icon(SNAME("action_plugin"), EditorStringName(Editor)));
	_project_menu->set_item_icon(_project_menu->get_item_index(PROJECT_OPEN_AUTOLOADS), _theme->get_icon(SNAME("gui_autoload"), EditorStringName(Editor)));
	_project_menu->set_item_icon(_project_menu->get_item_index(PROJECT_EXPORT), _theme->get_icon(SNAME("node_resource_preloader"), EditorStringName(Editor)));
	_project_menu->set_item_icon(_project_menu->get_item_index(PROJECT_QUIT_TO_HUB), _theme->get_icon(SNAME("action_close"), EditorStringName(Editor)));
}

void EditorNode::_update_theme(bool p_skip_creation) {
	if (!p_skip_creation) {
		_theme = EditorThemeManager::generate_theme(_theme);
		DisplayServer::set_early_window_clear_color_override(true, _theme->get_color(SNAME("background"), EditorStringName(Editor)));

#if defined(MODULE_GDSCRIPT_ENABLED)
		if (EditorHelpHighlighter::get_singleton()) {
			// Update syntax colors.
			EditorHelpHighlighter::get_singleton()->clear_cache();
		}
#endif
	}

	Vector<Ref<Theme>> editor_themes;
	editor_themes.push_back(_theme);
	editor_themes.push_back(ThemeDB::get_singleton()->get_default_theme());

	ThemeContext *node_tc = ThemeDB::get_singleton()->get_theme_context(this);
	if (node_tc) {
		node_tc->set_themes(editor_themes);
	} else {
		ThemeDB::get_singleton()->create_theme_context(this, editor_themes);
	}

	Window *window = get_window();
	if (window) {
		ThemeContext *window_tc = ThemeDB::get_singleton()->get_theme_context(window);
		if (window_tc) {
			window_tc->set_themes(editor_themes);
		} else {
			ThemeDB::get_singleton()->create_theme_context(window, editor_themes);
		}
	}

	if (CanvasItemEditor::get_singleton()->get_theme_preview() == CanvasItemEditor::ThemePreviewMode::EDITOR) {
		update_preview_themes();
	}

	// Update styles.
	{
		_update_main_layout_style(true);
		_dock_layout_pc->add_theme_style_override(SceneStringName(panel), _theme->get_stylebox(SNAME("dock_background"), EditorStringName(Editor)));

		if (_main_menu_btn != nullptr) {
			_main_menu_btn->set_button_icon(_theme->get_icon(SNAME("gui_triple_bar"), EditorStringName(Editor)));
		}

		_update_main_menu_icons();
	}

	Ref<Texture2D> thumbnail_icon = _editor_overlay->get_theme_icon(SNAME("file_thumbnail"), SNAME("FileDialog"));
	_default_thumbnail.instantiate();
	_default_thumbnail->set_image(thumbnail_icon->get_image());

	_editor_dock_manager->update_tab_styles();
	_editor_dock_manager->update_docks_menu();
	_editor_dock_manager->set_tab_icon_max_width(_theme->get_constant(SNAME("class_icon_size"), EditorStringName(Editor)));

#ifdef ANDROID_ENABLED
	DisplayServer::get_singleton()->window_set_color(_theme->get_color(SNAME("background"), EditorStringName(Editor)));
#endif
}

void EditorNode::_update_main_layout_style(bool p_theme_changed) {
	const Window *window = get_window();
	const bool use_padded_style = window && window->get_mode() != Window::MODE_MAXIMIZED;
	if (!p_theme_changed && use_padded_style == _uses_padded_layout_style) {
		return;
	}

	Ref<StyleBox> background = _theme->get_stylebox(SNAME("background"), EditorStringName(Editor));
	if (use_padded_style) {
		if (_theme->has_stylebox(SNAME("background_padded"), EditorStringName(Editor))) {
			background = _theme->get_stylebox(SNAME("background_padded"), EditorStringName(Editor));
		} else {
			background = background->duplicate();
			ERR_FAIL_COND(background.is_null());
			background->set_padding_all(5.0f);
		}
	}
	_uses_padded_layout_style = use_padded_style;
	_main_layout_pc->add_theme_style_override(SceneStringName(panel), background);
}

void EditorNode::_build_icon_type_cache() {
	LocalVector<StringName> classes;
	ClassDB::get_class_list(classes);
	for (const StringName &class_name : classes) {
		const StringName icon_name = editor_get_type_icon_name(class_name);
		if (!icon_name.is_empty() && _theme->has_icon(icon_name, EditorStringName(Editor))) {
			_icon_type_cache[class_name] = _theme->get_icon(icon_name, EditorStringName(Editor));
		}
	}
}

void EditorNode::_dialog_action(String p_file) {
	switch (_current_menu_option) {
		case SCENE_NEW_INHERITED_SCENE: {
			Node *scene = _editor_data.get_edited_scene_root();
			// If the previous scene is rootless, just close it in favor of the new one.
			if (!scene) {
				_menu_option_confirm(SCENE_CLOSE, true);
			}

			open_scene(p_file, false, true);
		} break;
		case SCENE_OPEN_SCENE: {
			open_scene(p_file);
		} break;
		case SETTINGS_PICK_MAIN_SCENE: {
			ProjectSettings::get_singleton()->set("application/run/main_scene", ResourceUID::path_to_uid(p_file));
			ProjectSettings::get_singleton()->save();
			// TODO: Would be nice to show the hub opened with the highlighted field.

			_project_run_bar->play_main_scene((bool)_pick_main_scene_dialog->get_meta("from_native", false));
		} break;
		case SCENE_CLOSE:
		case SCENE_TAB_CLOSE:
		case SCENE_SAVE_SCENE:
		case SCENE_MULTI_SAVE_AS_SCENE:
		case SCENE_SAVE_AS_SCENE:
		case SCENE_TAB_SAVE_SCENE:
		case SCENE_TAB_SAVE_AS_MAIN_SCENE:
		case SCENE_TAB_SAVE_AS_AND_RUN: {
			int scene_idx = -1;
			if (_current_menu_option == SCENE_CLOSE || _current_menu_option == SCENE_TAB_CLOSE) {
				scene_idx = _tab_closing_idx;
			} else if (_current_menu_option == SCENE_TAB_SAVE_SCENE || _current_menu_option == SCENE_TAB_SAVE_AS_MAIN_SCENE || _current_menu_option == SCENE_TAB_SAVE_AS_AND_RUN) {
				scene_idx = _scene_tabs->get_option_tab();
			}

			if (_scene_resource_file_dialog->get_file_mode() == EditorFileDialog::FILE_MODE_SAVE_FILE) {
				bool same_open_scene = false;
				for (int i = 0; i < _editor_data.get_edited_scene_count(); i++) {
					if (_editor_data.get_scene_path(i) == p_file && i != scene_idx) {
						same_open_scene = true;
					}
				}

				if (same_open_scene) {
					show_warning(TTR("Can't overwrite scene that is still open!"));
					return;
				}

				save_default_environment();
				_save_scene_with_preview(p_file, scene_idx);
				_add_to_recent_scenes(p_file);
				save_editor_layout_delayed();

				if (scene_idx != -1) {
					_discard_changes();
				} else {
					// Update the path of the edited scene to ensure later do/undo action history matches.
					_editor_data.set_scene_path(_editor_data.get_edited_scene(), p_file);
				}
			}

			if (_current_menu_option == SCENE_MULTI_SAVE_AS_SCENE) {
				_save_next_scene_tab_as();
			} else if (_current_menu_option == SCENE_TAB_SAVE_AS_MAIN_SCENE) {
				_menu_option_confirm(EditorSceneTabs::SCENE_SET_AS_MAIN_SCENE, true);
			} else if (_current_menu_option == SCENE_TAB_SAVE_AS_AND_RUN) {
				_project_run_bar->play_custom_scene(p_file);
			}
		} break;

		case SAVE_AND_RUN: {
			if (_scene_resource_file_dialog->get_file_mode() == EditorFileDialog::FILE_MODE_SAVE_FILE) {
				save_default_environment();
				_save_scene_with_preview(p_file);
				_project_run_bar->play_custom_scene(p_file);
			}
		} break;

		case SAVE_AND_RUN_MAIN_SCENE: {
			ProjectSettings::get_singleton()->set("application/run/main_scene", ResourceUID::path_to_uid(p_file));
			ProjectSettings::get_singleton()->save();

			if (_scene_resource_file_dialog->get_file_mode() == EditorFileDialog::FILE_MODE_SAVE_FILE) {
				save_default_environment();
				_save_scene_with_preview(p_file);
				_project_run_bar->play_main_scene((bool)_pick_main_scene_dialog->get_meta("from_native", false));
			}
		} break;

		case FILE_EXPORT_MESH_LIBRARY: {
			const Dictionary &fd_options = _mesh_library_export_dialog->get_selected_options();
			bool merge_with_existing_library = fd_options.get(TTR("Merge With Existing"), true);
			bool apply_mesh_instance_transforms = fd_options.get(TTR("Apply MeshInstance Transforms"), false);

			Ref<MeshLibrary> ml;
			if (merge_with_existing_library && FileAccess::exists(p_file)) {
				ml = ResourceLoader::load(p_file, "MeshLibrary");

				if (ml.is_null()) {
					show_warning(TTR("Can't load MeshLibrary for merging!"));
					return;
				}
			}

			if (ml.is_null()) {
				ml.instantiate();
			}

			MeshLibraryEditor::update_library_file(_editor_data.get_edited_scene_root(), ml, merge_with_existing_library, apply_mesh_instance_transforms);

			Error err = ResourceSaver::save(ml, p_file);
			if (err) {
				show_warning(TTR("Error saving MeshLibrary!"));
				return;
			} else if (ResourceCache::has(p_file)) {
				// Make sure MeshLibrary is updated in the editor.
				ResourceLoader::load(p_file)->reload_from_file();
			}

		} break;

		case PROJECT_PACK_AS_ZIP: {
			ProjectZIPPacker::pack_project_zip(p_file);
			{
				Ref<FileAccess> f = FileAccess::open(p_file, FileAccess::READ);
				ERR_FAIL_COND_MSG(f.is_null(), vformat("Unable to create ZIP file at: %s. Check for write permissions and whether you have enough disk space left.", p_file));
			}

		} break;

		case RESOURCE_SAVE:
		case RESOURCE_SAVE_AS: {
			ERR_FAIL_COND(_saving_resource.is_null());
			save_resource_in_path(_saving_resource, p_file);

			_saving_resource = Ref<Resource>();
			ObjectID current_id = _editor_history.get_current();
			Object *current_obj = current_id.is_valid() ? ObjectDB::get_instance(current_id) : nullptr;
			ERR_FAIL_NULL(current_obj);
			InspectorDock::get_inspector_singleton()->update_properties_recursive();
		} break;
		case LAYOUT_SAVE: {
			if (p_file.is_empty()) {
				return;
			}

			Ref<ConfigFile> config;
			config.instantiate();
			Error err = config->load(EditorSettings::get_singleton()->get_editor_layouts_config());

			if (err == ERR_FILE_CANT_OPEN || err == ERR_FILE_NOT_FOUND) {
				config.instantiate();
			} else if (err != OK) {
				show_warning(TTR("An error occurred while trying to save the editor layout.\nMake sure the editor's user data path is writable."));
				return;
			}

			_editor_dock_manager->save_docks_to_config(config, p_file);

			config->save(EditorSettings::get_singleton()->get_editor_layouts_config());

			_editor_layout_dialog->hide();
			_update_layouts_menu();

			if (p_file == "Default") {
				show_warning(TTR("Default editor layout overridden.\nTo restore the Default layout to its base settings, use the Delete Layout option and delete the Default layout."));
			}

		} break;
		case LAYOUT_DELETE: {
			Ref<ConfigFile> config;
			config.instantiate();

			Error err = config->load(EditorSettings::get_singleton()->get_editor_layouts_config());
			if (err != OK || !config->has_section(p_file)) {
				show_warning(TTR("Layout name not found!"));
				return;
			}

			for (const String &section : config->get_sections()) {
				// Erase sections related to the layout.
				if (section == p_file || section.begins_with(p_file + "/")) {
					config->erase_section(section);
				}
			}

			config->save(EditorSettings::get_singleton()->get_editor_layouts_config());

			_editor_layout_dialog->hide();
			_update_layouts_menu();

			if (p_file == "Default") {
				show_warning(TTR("Restored the Default layout to its base settings."));
			}

		} break;
		default: {
			// Save scene?
			if (_scene_resource_file_dialog->get_file_mode() == EditorFileDialog::FILE_MODE_SAVE_FILE) {
				_save_scene_with_preview(p_file);
			}

		} break;
	}
}

void EditorNode::_add_to_history(const Object *p_object, const String &p_property, bool p_inspector_only) {
	ObjectID id = p_object->get_instance_id();
	ObjectID history_id = _editor_history.get_current();
	if (id != history_id) {
		const MultiNodeEdit *multi_node_edit = Object::cast_to<const MultiNodeEdit>(p_object);
		const MultiNodeEdit *history_multi_node_edit = ObjectDB::get_instance<MultiNodeEdit>(history_id);
		if (multi_node_edit && history_multi_node_edit && multi_node_edit->is_same_selection(history_multi_node_edit)) {
			return;
		}
		if (p_inspector_only) {
			_editor_history.add_object(id, String(), true);
		} else if (p_property.is_empty()) {
			_editor_history.add_object(id);
		} else {
			_editor_history.add_object(id, p_property);
		}
	}
}

void EditorNode::_edit_current(bool p_skip_foreign, bool p_skip_inspector_update) {
	ObjectID current_id = _editor_history.get_current();
	Object *current_obj = current_id.is_valid() ? ObjectDB::get_instance(current_id) : nullptr;

	Ref<Resource> res = Object::cast_to<Resource>(current_obj);
	if (p_skip_foreign && res.is_valid()) {
		const int current_tab = _scene_tabs->get_current_tab();
		if (res->get_path().contains("::") && res->get_path().get_slice("::", 0) != _editor_data.get_scene_path(current_tab)) {
			// Trying to edit resource that belongs to another scene; abort.
			current_obj = nullptr;
		}
	}

	bool inspector_only = _editor_history.is_current_inspector_only();

	if (!current_obj) {
		HierarchyDock::get_singleton()->set_selected(nullptr);
		InspectorDock::get_inspector_singleton()->edit(nullptr);
		SignalsDock::get_singleton()->set_object(nullptr);
		GroupsDock::get_singleton()->set_selection(Vector<Node *>());
		InspectorDock::get_singleton()->update(nullptr);
		EditorDebuggerNode::get_singleton()->clear_remote_tree_selection();
		hide_unused_editors();
		return;
	}

	// Update the use folding setting and state.
	bool disable_folding = EDITOR_GET("interface/inspector/disable_folding");
	if (InspectorDock::get_inspector_singleton()->is_using_folding() == disable_folding) {
		InspectorDock::get_inspector_singleton()->set_use_folding(!disable_folding, false);
	}

	bool is_resource = Object::cast_to<Resource>(current_obj);
	bool is_node = Object::cast_to<Node>(current_obj);

	String editable_info; // None by default.
	bool info_is_warning = false;

	if (current_obj->call(SNAME("_is_read_only")).operator bool()) {
		editable_info = TTR("This object is marked as read-only, so it's not editable.");
	}

	if (is_resource) {
		Resource *current_res = Object::cast_to<Resource>(current_obj);
		ERR_FAIL_NULL(current_res);

		if (!p_skip_inspector_update) {
			InspectorDock::get_inspector_singleton()->edit(current_res);
			HierarchyDock::get_singleton()->set_selected(nullptr);
			SignalsDock::get_singleton()->set_object(current_res);
			GroupsDock::get_singleton()->set_selection(Vector<Node *>());
			InspectorDock::get_singleton()->update(nullptr);
			EditorDebuggerNode::get_singleton()->clear_remote_tree_selection();
			ImportDock::get_singleton()->set_edit_path(current_res->get_path());
		}

		int subr_idx = current_res->get_path().find("::");
		if (FileAccessContent::is_content_path(current_res->get_path())) {
			editable_info = TTR("This resource is supplied with the editor and is read-only. Use Make Unique or Save As to create an editable copy in your project.");
		} else if (subr_idx != -1) {
			String base_path = current_res->get_path().substr(0, subr_idx);
			if (FileAccess::exists(base_path + ".import")) {
				if (!base_path.is_resource_file()) {
					if (get_edited_scene() && get_edited_scene()->get_scene_file_path() == base_path) {
						info_is_warning = true;
					}
				}
				editable_info = TTR("This resource belongs to a scene that was imported, so it's not editable.\nPlease read the documentation relevant to importing scenes to better understand this workflow.");
			} else if ((!get_edited_scene() || get_edited_scene()->get_scene_file_path() != base_path) && ResourceLoader::get_resource_type(base_path) == "PackedScene") {
				editable_info = TTR("This resource belongs to a scene that was instantiated or inherited.\nChanges to it must be made inside the original scene.");
			}
		} else if (current_res->get_path().is_resource_file()) {
			if (FileAccess::exists(current_res->get_path() + ".import")) {
				editable_info = TTR("This resource was imported, so it's not editable. Change its settings in the import panel and then re-import.");
			}
		}
	} else if (is_node) {
		Node *current_node = Object::cast_to<Node>(current_obj);
		ERR_FAIL_NULL(current_node);

		InspectorDock::get_inspector_singleton()->edit(current_node);
		if (current_node->is_inside_tree()) {
			SignalsDock::get_singleton()->set_object(current_node);
			GroupsDock::get_singleton()->set_selection(Vector<Node *>{ current_node });
			HierarchyDock::get_singleton()->set_selected(current_node);
			HierarchyDock::get_singleton()->set_selection({ current_node });
			InspectorDock::get_singleton()->update(current_node);
		} else {
			SignalsDock::get_singleton()->set_object(nullptr);
			GroupsDock::get_singleton()->set_selection(Vector<Node *>());
			HierarchyDock::get_singleton()->set_selected(nullptr);
			InspectorDock::get_singleton()->update(nullptr);
		}
		EditorDebuggerNode::get_singleton()->clear_remote_tree_selection();

		if (get_edited_scene() && !get_edited_scene()->get_scene_file_path().is_empty()) {
			String source_scene = get_edited_scene()->get_scene_file_path();
			if (FileAccess::exists(source_scene + ".import")) {
				editable_info = TTR("This scene was imported, so changes to it won't be kept.\nInstantiating or inheriting it will allow you to make changes to it.\nPlease read the documentation relevant to importing scenes to better understand this workflow.");
				info_is_warning = true;
			}
		}
	} else {
		Node *selected_node = nullptr;

		Vector<Node *> multi_nodes;
		if (current_obj->is_class("MultiNodeEdit")) {
			Node *scene = get_edited_scene();
			if (scene) {
				MultiNodeEdit *multi_node_edit = Object::cast_to<MultiNodeEdit>(current_obj);
				int node_count = multi_node_edit->get_node_count();
				if (node_count > 0) {
					for (int node_index = 0; node_index < node_count; ++node_index) {
						Node *node = scene->get_node(multi_node_edit->get_node(node_index));
						if (node) {
							multi_nodes.push_back(node);
						}
					}
					if (!multi_nodes.is_empty()) {
						// Pick the top-most node.
						selected_node = multi_nodes[0];
						Node::Comparator comparator;
						for (Node *node : multi_nodes) {
							if (comparator(node, selected_node)) {
								selected_node = node;
							}
						}
					}
				}
			}
		}

		if (!current_obj->is_class("EditorDebuggerRemoteObjects")) {
			EditorDebuggerNode::get_singleton()->clear_remote_tree_selection();
		}

		InspectorDock::get_inspector_singleton()->edit(current_obj);
		SignalsDock::get_singleton()->set_object(nullptr);
		GroupsDock::get_singleton()->set_selection(multi_nodes);
		HierarchyDock::get_singleton()->set_selected(selected_node);
		HierarchyDock::get_singleton()->set_selection(multi_nodes);
		InspectorDock::get_singleton()->update(nullptr);
	}

	InspectorDock::get_singleton()->set_info(
			info_is_warning ? TTR("Changes may be lost!") : TTR("This object is read-only."),
			editable_info,
			info_is_warning);

	Object *editor_owner = (is_node || current_obj->is_class("MultiNodeEdit")) ? (Object *)HierarchyDock::get_singleton() : is_resource ? (Object *)InspectorDock::get_inspector_singleton()
																																		: (Object *)this;

	// Take care of the main editor plugin.

	if (!inspector_only) {
		edit_item(current_obj, editor_owner);
	}

	InspectorDock::get_singleton()->update(current_obj);
}

void EditorNode::_dialog_display_save_error(String p_file, Error p_error) {
	if (p_error) {
		switch (p_error) {
			case ERR_FILE_CANT_WRITE: {
				show_warning(TTR("Can't open file for writing:") + " " + p_file.get_extension());
			} break;
			case ERR_FILE_UNRECOGNIZED: {
				show_warning(TTR("Requested file format unknown:") + " " + p_file.get_extension());
			} break;
			default: {
				show_warning(TTR("Error while saving."));
			} break;
		}
	}
}

void EditorNode::_dialog_display_load_error(String p_file, Error p_error) {
	if (p_error) {
		switch (p_error) {
			case ERR_CANT_OPEN: {
				show_warning(vformat(TTR("Can't open file '%s'. The file could have been moved or deleted."), p_file.get_file()));
			} break;
			case ERR_PARSE_ERROR: {
				show_warning(vformat(TTR("Error while parsing file '%s'."), p_file.get_file()));
			} break;
			case ERR_FILE_CORRUPT: {
				show_warning(vformat(TTR("Scene file '%s' appears to be invalid/corrupt."), p_file.get_file()));
			} break;
			case ERR_FILE_NOT_FOUND: {
				show_warning(vformat(TTR("Missing file '%s' or one of its dependencies."), p_file.get_file()));
			} break;
			case ERR_FILE_UNRECOGNIZED: {
				show_warning(vformat(TTR("File '%s' is saved in a format that is newer than the formats supported by this version of Godot, so it can't be opened."), p_file.get_file()));
			} break;
			default: {
				show_warning(vformat(TTR("Error while loading file '%s'."), p_file.get_file()));
			} break;
		}
	}
}

void EditorNode::_menu_option(int p_option) {
	_menu_option_confirm(p_option, false);
}

void EditorNode::_menu_confirm_current() {
	_menu_option_confirm(_current_menu_option, true);
}

void EditorNode::_menu_option_confirm(int p_option, bool p_confirmed) {
	if (!p_confirmed) { // FIXME: this may be a hack.
		_current_menu_option = (MenuOptions)p_option;
	}

	constexpr int BUTTON_MASK_FULL = int(MouseButtonMask::LEFT | MouseButtonMask::RIGHT | MouseButtonMask::MIDDLE | MouseButtonMask::MB_XBUTTON1 | MouseButtonMask::MB_XBUTTON2);

	switch (p_option) {
		case SCENE_NEW_SCENE: {
			new_scene();

		} break;
		case SCENE_NEW_INHERITED_SCENE:
		case SCENE_OPEN_SCENE: {
			_scene_resource_file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILE);
			List<String> extensions;
			ResourceLoader::get_recognized_extensions_for_type("PackedScene", &extensions);
			_scene_resource_file_dialog->clear_filters();
			for (const String &extension : extensions) {
				_scene_resource_file_dialog->add_filter("*." + extension, extension.to_upper());
			}

			Node *scene = _editor_data.get_edited_scene_root();
			if (scene) {
				_scene_resource_file_dialog->set_current_path(scene->get_scene_file_path());
			};
			_scene_resource_file_dialog->set_title(p_option == SCENE_OPEN_SCENE ? TTR("Open Scene") : TTR("Open Base Scene"));
			_scene_resource_file_dialog->popup_file_dialog();

		} break;
		case SCENE_QUICK_OPEN: {
			_quick_open_dialog->popup_dialog({ "Resource" }, callable_mp(this, &EditorNode::_quick_opened), true);
		} break;
		case SCENE_QUICK_OPEN_SCENE: {
			_quick_open_dialog->popup_dialog({ "PackedScene" }, callable_mp(this, &EditorNode::_quick_opened), true);
		} break;
		case SCENE_QUICK_OPEN_SCRIPT: {
			_quick_open_dialog->popup_dialog({ "Script" }, callable_mp(this, &EditorNode::_quick_opened), true);
		} break;
		case SCENE_OPEN_PREV: {
			if (!_prev_closed_scenes.is_empty()) {
				String path = _prev_closed_scenes[_prev_closed_scenes.size() - 1];
				path = ResourceUID::ensure_path_nocheck(path);
				if (!path.is_empty() && ResourceLoader::exists(path)) {
					open_scene(path);
				} else {
					_prev_closed_scenes.resize(_prev_closed_scenes.size() - 1);
				}
			}
		} break;
		case EditorSceneTabs::SCENE_CLOSE_OTHERS: {
			_tab_closing_menu_option = -1;
			int option_tab = _scene_tabs->get_option_tab();
			for (int i = 0; i < _editor_data.get_edited_scene_count(); i++) {
				if (i == option_tab) {
					continue;
				}
				_tabs_to_close.push_back(_editor_data.get_scene_path(i));
			}
			_proceed_closing_scene_tabs();
		} break;
		case EditorSceneTabs::SCENE_CLOSE_RIGHT: {
			_tab_closing_menu_option = -1;
			for (int i = _scene_tabs->get_option_tab() + 1; i < _editor_data.get_edited_scene_count(); i++) {
				_tabs_to_close.push_back(_editor_data.get_scene_path(i));
			}
			_proceed_closing_scene_tabs();
		} break;
		case SCENE_CLOSE_ALL: {
			_tab_closing_menu_option = -1;
			for (int i = 0; i < _editor_data.get_edited_scene_count(); i++) {
				_tabs_to_close.push_back(_editor_data.get_scene_path(i));
			}
			_proceed_closing_scene_tabs();
		} break;
		case SCENE_CLOSE: {
			_scene_tab_closed(_editor_data.get_edited_scene());
		} break;
		case EditorSceneTabs::SCENE_CLOSE: {
			_scene_tab_closed(_scene_tabs->get_option_tab());
		} break;
		case SCENE_TAB_CLOSE:
		case SCENE_SAVE_SCENE:
		case EditorSceneTabs::SCENE_SAVE_SCENE: {
			int scene_idx = -1;
			if (p_option == SCENE_TAB_CLOSE) {
				scene_idx = _tab_closing_idx;
			} else if (p_option == EditorSceneTabs::SCENE_SAVE_SCENE) {
				scene_idx = _scene_tabs->get_option_tab();
			}
			Node *scene = _editor_data.get_edited_scene_root(scene_idx);

			if (scene && !scene->get_scene_file_path().is_empty()) {
				if (DirAccess::exists(scene->get_scene_file_path().get_base_dir())) {
					if (scene_idx != _editor_data.get_edited_scene()) {
						_save_scene_with_preview(scene->get_scene_file_path(), scene_idx);
					} else {
						_save_scene_with_preview(scene->get_scene_file_path());
					}

					if (scene_idx != -1) {
						_discard_changes();
					}
					save_editor_layout_delayed();
				} else {
					show_save_accept(vformat(TTR("%s no longer exists! Please specify a new save location."), scene->get_scene_file_path().get_base_dir()));
				}
				break;
			}
			[[fallthrough]];
		}
		case SCENE_MULTI_SAVE_AS_SCENE:
		case SCENE_SAVE_AS_SCENE:
		case EditorSceneTabs::SCENE_SAVE_AS_SCENE: {
			int scene_idx = -1;
			if (p_option == SCENE_TAB_CLOSE) {
				scene_idx = _tab_closing_idx;
			} else if (p_option == EditorSceneTabs::SCENE_SAVE_SCENE || p_option == EditorSceneTabs::SCENE_SAVE_AS_SCENE) {
				scene_idx = _scene_tabs->get_option_tab();
				_current_menu_option = SCENE_TAB_SAVE_SCENE;
			}
			Node *scene = _editor_data.get_edited_scene_root(scene_idx);
			if (!scene) {
				if (p_option == SCENE_SAVE_SCENE) {
					// Pressing Ctrl + S saves the current script if a scene is currently open, but it won't if the scene has no root node.
					// Work around this by explicitly saving the script in this case (similar to pressing Ctrl + Alt + S).
					ScriptEditorDock::get_singleton()->save_current_script();
				}

				const int saved = _save_external_resources(true);
				if (saved > 0) {
					EditorToasterContainer::get_singleton()->popup_str(vformat(TTR("The current scene has no root node, but %d modified external resource(s) and/or plugin data were saved anyway."), saved), EditorToasterContainer::INFO);
				} else if (p_option == SCENE_SAVE_AS_SCENE) {
					// Don't show this dialog when pressing Ctrl + S to avoid interfering with script saving.
					show_warning(TTR("A root node is required to save the scene. You can add a root node using the Hierarchy dock."));
				}

				break;
			}

			_scene_resource_file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_SAVE_FILE);

			List<String> extensions;
			Ref<PackedScene> sd = memnew(PackedScene);
			ResourceSaver::get_recognized_extensions(sd, &extensions);
			_scene_resource_file_dialog->clear_filters();
			for (const String &extension : extensions) {
				_scene_resource_file_dialog->add_filter("*." + extension, extension.to_upper());
			}

			if (!scene->get_scene_file_path().is_empty()) {
				String path = scene->get_scene_file_path();
				String root_name = EditorNode::adjust_scene_name_casing(scene->get_name());
				String ext = path.get_extension().to_lower();
				path = path.get_base_dir().path_join(root_name + "." + ext);

				_scene_resource_file_dialog->set_current_path(path);
				if (extensions.size()) {
					if (extensions.find(ext) == nullptr) {
						_scene_resource_file_dialog->set_current_path(path.replacen("." + ext, "." + extensions.front()->get()));
					}
				}
			} else if (extensions.size()) {
				String root_name = scene->get_name();
				root_name = EditorNode::adjust_scene_name_casing(root_name);
				_scene_resource_file_dialog->set_current_path(root_name + "." + extensions.front()->get().to_lower());
			}
			_scene_resource_file_dialog->set_title(TTR("Save Scene As..."));
			_scene_resource_file_dialog->popup_file_dialog();

		} break;

		case EditorSceneTabs::SCENE_SET_AS_MAIN_SCENE: {
			const String scene_path = _editor_data.get_scene_path(_scene_tabs->get_option_tab());
			if (scene_path.is_empty()) {
				_menu_option_confirm(EditorSceneTabs::SCENE_SAVE_AS_SCENE, true);
				_current_menu_option = SCENE_TAB_SAVE_AS_MAIN_SCENE; // Overwrites SCENE_TAB_SAVE_SCENE.
				_scene_resource_file_dialog->set_title(TTR("Save new main scene..."));
			} else {
				ProjectSettings::get_singleton()->set("application/run/main_scene", ResourceUID::path_to_uid(scene_path));
				ProjectSettings::get_singleton()->save();
				FileSystemDock::get_singleton()->update_all();
			}
		} break;

		case SCENE_SAVE_ALL_SCENES: {
			_save_all_scenes();
		} break;

		case EditorSceneTabs::SCENE_RUN: {
			Node *scene = _editor_data.get_edited_scene_root(_scene_tabs->get_option_tab());
			ERR_FAIL_NULL(scene);

			const String &scene_path = scene->get_scene_file_path();
			if (scene_path.is_empty()) {
				_menu_option_confirm(EditorSceneTabs::SCENE_SAVE_AS_SCENE, true);
				_current_menu_option = SCENE_TAB_SAVE_AS_AND_RUN; // Overwrites SCENE_TAB_SAVE_SCENE.
				_scene_resource_file_dialog->set_title(TTR("Save scene before running..."));
			} else {
				_project_run_bar->play_custom_scene(scene_path);
			}
		} break;

		case PROJECT_EXPORT: {
			_project_export_dialog->popup_export();
		} break;

		case PROJECT_PACK_AS_ZIP: {
			String resource_path = ProjectSettings::get_singleton()->get_resource_path();
			const String base_path = resource_path.substr(0, resource_path.rfind_char('/')) + "/";

			_project_zip_export_dialog->set_current_path(base_path);
			_project_zip_export_dialog->set_current_file(ProjectZIPPacker::get_project_zip_safe_name());
			_project_zip_export_dialog->popup_file_dialog();
		} break;

		case SCENE_UNDO: {
			if ((int)Input::get_singleton()->get_mouse_button_mask() & BUTTON_MASK_FULL) {
				_log->add_message(TTR("Can't undo while mouse buttons are pressed."), EditorLog::MSG_TYPE_EDITOR);
			} else {
				EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
				String action = undo_redo->get_current_action_name();
				int id = undo_redo->get_current_action_history_id();
				if (!undo_redo->undo()) {
					_log->add_message(TTR("Nothing to undo."), EditorLog::MSG_TYPE_EDITOR);
				} else if (!action.is_empty()) {
					switch (id) {
						case EditorUndoRedoManager::GLOBAL_HISTORY:
							_log->add_message(vformat(TTR("Global Undo: %s"), action), EditorLog::MSG_TYPE_EDITOR);
							break;
						case EditorUndoRedoManager::REMOTE_HISTORY:
							_log->add_message(vformat(TTR("Remote Undo: %s"), action), EditorLog::MSG_TYPE_EDITOR);
							break;
						default:
							_log->add_message(vformat(TTR("Scene Undo: %s"), action), EditorLog::MSG_TYPE_EDITOR);
					}
				}
			}
			_update_unsaved_cache();
		} break;
		case SCENE_REDO: {
			EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
			if ((int)Input::get_singleton()->get_mouse_button_mask() & BUTTON_MASK_FULL) {
				_log->add_message(TTR("Can't redo while mouse buttons are pressed."), EditorLog::MSG_TYPE_EDITOR);
			} else {
				if (!undo_redo->redo()) {
					_log->add_message(TTR("Nothing to redo."), EditorLog::MSG_TYPE_EDITOR);
				} else {
					String action = undo_redo->get_current_action_name();
					if (action.is_empty()) {
						break;
					}

					switch (undo_redo->get_current_action_history_id()) {
						case EditorUndoRedoManager::GLOBAL_HISTORY:
							_log->add_message(vformat(TTR("Global Redo: %s"), action), EditorLog::MSG_TYPE_EDITOR);
							break;
						case EditorUndoRedoManager::REMOTE_HISTORY:
							_log->add_message(vformat(TTR("Remote Redo: %s"), action), EditorLog::MSG_TYPE_EDITOR);
							break;
						default:
							_log->add_message(vformat(TTR("Scene Redo: %s"), action), EditorLog::MSG_TYPE_EDITOR);
					}
				}
			}
			_update_unsaved_cache();
		} break;

		case SCENE_RELOAD_SAVED_SCENE: {
			const int p_idx = _editor_data.get_edited_scene();
			const uint64_t scene_time_opened = _editor_data.get_scene_time_opened(p_idx);
			const String scene_filename = _editor_data.get_scene_path(p_idx);

			if (scene_filename.is_empty()) {
				show_warning(TTR("Can't reload a scene that was never saved."));
				break;
			}

			if (_unsaved_cache) {
				if (!p_confirmed) {
					_confirmation_dialog->set_ok_button_text(TTRC("Save & Reload"));
					const String unsaved_message = _get_unsaved_scene_dialog_text(scene_filename, scene_time_opened);
					_confirmation_dialog->set_text(unsaved_message + "\n\n" + TTR("Save before reloading the scene?"));
					_confirmation_dialog->popup_centered();
					_discard_changes_btn->show();
					_discard_changes_btn->grab_focus();
					break;
				} else {
					_save_scene_with_preview(scene_filename);
				}
			}

			_discard_changes();
		} break;

		case EditorSceneTabs::SCENE_SHOW_IN_FILESYSTEM: {
			String path = _editor_data.get_scene_path(_scene_tabs->get_option_tab());
			if (!path.is_empty()) {
				FileSystemDock::get_singleton()->navigate_to_path(path);
			}
		} break;

		case PROJECT_OPEN_PLUGINS: {
			if (!_project_plugins_dialog) {
				_project_plugins_dialog = memnew(ProjectPluginsDialog);
				_editor_overlay->add_child(_project_plugins_dialog);
			}
			_project_plugins_dialog->popup_plugins();
		} break;

		case PROJECT_OPEN_AUTOLOADS: {
			if (!_project_autoloads_dialog) {
				_project_autoloads_dialog = memnew(ProjectAutoloadsDialog);
				_editor_overlay->add_child(_project_autoloads_dialog);
			}
			_project_autoloads_dialog->popup_autoloads();
		} break;

		case PROJECT_OPEN_SETTINGS: {
			_project_settings_editor->popup_project_settings();
		} break;

		case PROJECT_FIND_IN_FILES:
		case PROJECT_REPLACE_IN_FILES: {
			FindInFiles::get_singleton()->open_dock("", PROJECT_REPLACE_IN_FILES == p_option);
		} break;

		case PROJECT_SETUP_ANDROID_BUILD: {
#ifndef ANDROID_ENABLED
			if (!(AndroidSDKManager::is_android_sdk_setup() && AndroidSDKManager::is_java_sdk_setup())) {
				Callable setup_android_build_callable = callable_mp(this, &EditorNode::_setup_android_build).bind(p_confirmed);
				_android_sdk_manager->run_setup(setup_android_build_callable, setup_android_build_callable);
			} else {
#endif
				_setup_android_build(p_confirmed);
#ifndef ANDROID_ENABLED
			}
#endif
		} break;
		case PROJECT_OPEN_USER_DATA_FOLDER: {
			// Ensure_user_data_dir() to prevent the edge case: "Open User Data Folder" won't work after the project was renamed in ProjectSettingsEditor unless the project is saved.
			OS::get_singleton()->ensure_user_data_dir();
			OS::get_singleton()->shell_show_in_file_manager(OS::get_singleton()->get_user_data_dir(), true);
		} break;
		case SCENE_QUIT:
		case PROJECT_QUIT_TO_HUB:
		case TOOLS_CLEAR_PROJECT_CACHE:
		case PROJECT_RELOAD_CURRENT_PROJECT: {
			if (p_confirmed && _editor_save_callback.is_valid()) {
				_editor_save_callback.call();
				_editor_save_callback = Callable();
				p_confirmed = false;
			}

			if (p_confirmed && _stop_project_confirmation && _project_run_bar->is_playing()) {
				_project_run_bar->stop_playing();
				_stop_project_confirmation = false;
				p_confirmed = false;
			}

			bool is_restart = (p_option == PROJECT_RELOAD_CURRENT_PROJECT || p_option == TOOLS_CLEAR_PROJECT_CACHE);
			if (!p_confirmed) {
				if (!_stop_project_confirmation && _project_run_bar->is_playing()) {
					if (is_restart) {
						_confirmation_dialog->set_text(TTR("Stop running project before reloading the current project?"));
						_confirmation_dialog->set_ok_button_text(TTR("Stop & Reload"));
					} else {
						_confirmation_dialog->set_text(TTR("Stop running project before exiting the editor?"));
						_confirmation_dialog->set_ok_button_text(TTR("Stop & Quit"));
					}
					_discard_changes_btn->hide();
					_confirmation_dialog->reset_size();
					_confirmation_dialog->popup_centered();
					_stop_project_confirmation = true;
					break;
				}

				bool save_each = EDITOR_GET("interface/editor/behavior/save_each_scene_on_quit");
				if (_next_unsaved_scene(!save_each) == -1) {
					if (EditorUndoRedoManager::get_singleton()->is_history_unsaved(EditorUndoRedoManager::GLOBAL_HISTORY)) {
						if (is_restart) {
							_save_confirmation_dialog->set_ok_button_text(TTR("Save & Reload"));
							_save_confirmation_dialog->set_text(TTR("Save modified resources before reloading?"));
						} else {
							_save_confirmation_dialog->set_ok_button_text(TTR("Save & Quit"));
							_save_confirmation_dialog->set_text(TTR("Save modified resources before closing?"));
						}
						_save_confirmation_dialog->reset_size();
						_save_confirmation_dialog->popup_centered();
						break;
					}

					const String unsaved_status = _editor_data.get_unsaved_status("", &_editor_save_callback);
					if (!unsaved_status.is_empty()) {
						_save_confirmation_dialog->set_ok_button_text(is_restart ? TTR("Save & Reload") : TTR("Save & Quit"));
						_save_confirmation_dialog->set_text(unsaved_status);
						_save_confirmation_dialog->reset_size();
						_save_confirmation_dialog->popup_centered();
						break;
					}

					_discard_changes();
					break;
				}

				if (save_each) {
					_tab_closing_menu_option = _current_menu_option;
					for (int i = 0; i < _editor_data.get_edited_scene_count(); i++) {
						_tabs_to_close.push_back(_editor_data.get_scene_path(i));
					}
					_proceed_closing_scene_tabs();
				} else {
					String unsaved_scenes;
					int i = _next_unsaved_scene(true, 0);
					while (i != -1) {
						unsaved_scenes += "\n            " + _editor_data.get_edited_scene_root(i)->get_scene_file_path();
						i = _next_unsaved_scene(true, ++i);
					}
					if (is_restart) {
						_save_confirmation_dialog->set_ok_button_text(TTR("Save & Reload"));
						_save_confirmation_dialog->set_text(TTR("Save changes to the following scene(s) before reloading?") + unsaved_scenes);
					} else {
						_save_confirmation_dialog->set_ok_button_text(TTR("Save & Quit"));
						_save_confirmation_dialog->set_text((p_option == SCENE_QUIT ? TTR("Save changes to the following scene(s) before quitting?") : TTR("Save changes to the following scene(s) before opening HUB?")) + unsaved_scenes);
					}
					_save_confirmation_dialog->reset_size();
					_save_confirmation_dialog->popup_centered();
				}

				DisplayServer::get_singleton()->window_request_attention();
				break;
			}
			_save_external_resources();
			_discard_changes();
		} break;
		case EDITOR_OPEN_DATA_FOLDER: {
			OS::get_singleton()->shell_show_in_file_manager(EditorPaths::get_singleton()->get_data_dir(), true);
		} break;
		case EDITOR_OPEN_CONFIG_FOLDER: {
			OS::get_singleton()->shell_show_in_file_manager(EditorPaths::get_singleton()->get_config_dir(), true);
		} break;
		case EDITOR_MANAGE_EXPORT_TEMPLATES: {
			_export_template_manager->popup_manager();
		} break;
		case EDITOR_CONFIGURE_FBX_IMPORTER: {
#if !defined(ANDROID_ENABLED)
			_fbx_importer_manager->show_dialog();
#endif
		} break;
		case EDITOR_MANAGE_FEATURE_PROFILES: {
			_feature_profile_manager->popup_centered_clamped(Size2(900, 800) * EDSCALE, 0.8);
		} break;
		case EDITOR_TAKE_SCREENSHOT: {
			_screenshot_timer->start();
		} break;
		case SETTINGS_PICK_MAIN_SCENE: {
			_scene_resource_file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILE);
			List<String> extensions;
			ResourceLoader::get_recognized_extensions_for_type("PackedScene", &extensions);
			_scene_resource_file_dialog->clear_filters();
			for (const String &extension : extensions) {
				_scene_resource_file_dialog->add_filter("*." + extension, extension.to_upper());
			}

			Node *scene = _editor_data.get_edited_scene_root();
			if (scene) {
				_scene_resource_file_dialog->set_current_path(scene->get_scene_file_path());
			}
			_scene_resource_file_dialog->set_title(TTR("Pick a Main Scene"));
			_scene_resource_file_dialog->popup_file_dialog();

		} break;
	}
}

void EditorNode::_android_build_source_selected(const String &p_file) {
	_export_template_manager->install_android_template_from_file(p_file, _android_export_preset);
}

void EditorNode::_android_export_preset_selected(int p_index) {
	if (p_index >= 0) {
		_android_export_preset = EditorExport::get_singleton()->get_export_preset(_android_export_profile_btn->get_item_id(p_index));
	} else {
		_android_export_preset.unref();
	}
	_install_android_build_template_lbl->set_text(vformat(TTR(INSTALL_ANDROID_BUILD_TEMPLATE_MESSAGE), _export_template_manager->get_android_build_directory(_android_export_preset)));
}

void EditorNode::_android_install_build_template() {
	_missing_android_build_template_dialog->hide();
	_android_build_source_file_dialog->popup_centered_ratio();
}

void EditorNode::_android_explore_build_templates() {
	OS::get_singleton()->shell_show_in_file_manager(ProjectSettings::get_singleton()->globalize_path(_export_template_manager->get_android_build_directory(_android_export_preset).get_base_dir()), true);
}

void EditorNode::_android_remove_build_templates(bool p_prompt_for_removal) {
	String removal_text = ANDROID_BUILD_TEMPLATE_ALREADY_INSTALLED_MESSAGE;
	if (p_prompt_for_removal) {
		removal_text += "\n" + vformat(TTR(REMOVE_ANDROID_BUILD_TEMPLATE_MESSAGE), _export_template_manager->get_android_build_directory(_android_export_preset));
	}
	_remove_android_build_template_dialog->set_text(removal_text);
	_remove_android_build_template_dialog->popup_centered_clamped(Size2(600, 200) * EDSCALE, 0.7);
}

void EditorNode::_setup_android_build(bool p_confirmed) {
	if (p_confirmed) {
		setup_android_build_template(_android_export_preset, p_confirmed);
	} else {
		bool has_custom_gradle_build = false;
		_android_export_profile_btn->clear();
		for (int i = 0; i < EditorExport::get_singleton()->get_export_preset_count(); i++) {
			Ref<EditorExportPreset> export_preset = EditorExport::get_singleton()->get_export_preset(i);
			if (export_preset->get_platform()->get_class_name() == "EditorExportPlatformAndroid" && (bool)export_preset->get("gradle_build/use_gradle_build")) {
				_android_export_profile_btn->add_item(export_preset->get_name(), i);
				String gradle_build_directory = export_preset->get("gradle_build/gradle_build_directory");
				String android_source_template = export_preset->get("gradle_build/android_source_template");
				if (!android_source_template.is_empty() || (gradle_build_directory != "" && gradle_build_directory != "res://android")) {
					has_custom_gradle_build = true;
				}
			}
		}
		_android_export_preset_selected(_android_export_profile_btn->get_item_count() >= 1 ? 0 : -1);

		if (_android_export_profile_btn->get_item_count() > 1 && has_custom_gradle_build) {
			// If there's multiple options and at least one of them uses a custom gradle build then prompt the user to choose.
			_android_export_profile_btn->show();
			_install_android_build_template_dialog->popup_centered();
		} else {
			_android_export_profile_btn->hide();

			setup_android_build_template(_android_export_preset, p_confirmed);
		}
	}
}

void EditorNode::_request_screenshot() {
	_screenshot();
}

void EditorNode::_screenshot(bool p_use_utc) {
	String name = "editor_screenshot_" + Time::get_singleton()->get_datetime_string_from_system(p_use_utc).remove_char(':') + ".png";
	String path = String("user://") + name;

	if (!EditorRun::request_screenshot(callable_mp(this, &EditorNode::_save_screenshot_with_embedded_process).bind(path))) {
		_save_screenshot(path);
	}
}

void EditorNode::_save_screenshot(const String &p_path) {
	Viewport *viewport = get_viewport();
	ERR_FAIL_NULL_MSG(viewport, "Cannot get a viewport from the EditorNode.");
	Ref<ViewportTexture> texture = viewport->get_texture();
	ERR_FAIL_COND_MSG(texture.is_null(), "Cannot get a viewport texture from the editor window.");
	Ref<Image> img = texture->get_image();
	ERR_FAIL_COND_MSG(img.is_null(), "Cannot get an image from a viewport texture of the editor window.");
#ifdef RD_ENABLED
	RenderingDevice *rendering_device = RD::get_singleton();
	if (rendering_device && RenderingServer::get_singleton()->viewport_is_using_hdr_2d(viewport->get_viewport_rid())) {
		img->linear_to_srgb();
	}
#endif
	Error error = img->save_png(p_path);
	ERR_FAIL_COND_MSG(error != OK, "Cannot save screenshot to file '" + p_path + "'.");

	if (EDITOR_GET("interface/editor/behavior/automatically_open_screenshots")) {
		OS::get_singleton()->shell_show_in_file_manager(ProjectSettings::get_singleton()->globalize_path(p_path), true);
	}
}

void EditorNode::_save_screenshot_with_embedded_process(int64_t p_w, int64_t p_h, const String &p_emb_path, const Rect2i &p_rect, const String &p_path) {
	Viewport *viewport = get_viewport();
	ERR_FAIL_NULL_MSG(viewport, "Cannot get a viewport from the EditorNode.");
	Ref<ViewportTexture> texture = viewport->get_texture();
	ERR_FAIL_COND_MSG(texture.is_null(), "Cannot get a viewport texture from the editor window.");
	Ref<Image> img = texture->get_image();
	ERR_FAIL_COND_MSG(img.is_null(), "Cannot get an image from a viewport texture of the editor window.");
#ifdef RD_ENABLED
	RenderingDevice *rendering_device = RD::get_singleton();
	if (rendering_device && RenderingServer::get_singleton()->viewport_is_using_hdr_2d(viewport->get_viewport_rid())) {
		img->linear_to_srgb();
	}
#endif
	ERR_FAIL_COND(p_emb_path.is_empty());
	Ref<Image> overlay = Image::load_from_file(p_emb_path);
	DirAccess::remove_absolute(p_emb_path);
	ERR_FAIL_COND_MSG(overlay.is_null(), "Cannot get an image from a embedded process.");
	overlay->convert(img->get_format());
	overlay->resize(p_rect.size.x, p_rect.size.y);
	img->blend_rect(overlay, Rect2i(0, 0, p_w, p_h), p_rect.position);
	Error error = img->save_png(p_path);
	ERR_FAIL_COND_MSG(error != OK, "Cannot save screenshot to file '" + p_path + "'.");

	if (EDITOR_GET("interface/editor/behavior/automatically_open_screenshots")) {
		OS::get_singleton()->shell_show_in_file_manager(ProjectSettings::get_singleton()->globalize_path(p_path), true);
	}
}

void EditorNode::_check_system_theme_changed() {
	DisplayServer *display_server = DisplayServer::get_singleton();

	bool system_theme_changed = false;

	if (_follow_system_theme) {
		if (display_server->get_base_color() != _last_system_base_color) {
			system_theme_changed = true;
			_last_system_base_color = display_server->get_base_color();
		}

		if (display_server->is_dark_mode_supported() && display_server->is_dark_mode() != _last_dark_mode_state) {
			system_theme_changed = true;
			_last_dark_mode_state = display_server->is_dark_mode();
		}
	}

	if (_use_system_accent_color) {
		if (display_server->get_accent_color() != _last_system_accent_color) {
			system_theme_changed = true;
			_last_system_accent_color = display_server->get_accent_color();
		}
	}

	if (system_theme_changed) {
		EditorThemeManager::set_theme_outdated();

		SceneTree *sml = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
		if (!sml) {
			return;
		}
		Node *root = sml->get_root()->get_child(0);
		if (!root) {
			return;
		}
		// Some theme related settings are updated in editor settings change handlers.
		// Using "NOTIFICATION_EDITOR_SETTINGS_CHANGED" to simulate manual theme change from the editor settings and ensure all parts of the editor are updated correctly.
		root->propagate_notification(EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED);
	}
}

void EditorNode::_tool_menu_option(int p_idx) {
	switch (_tool_menu->get_item_id(p_idx)) {
		case TOOLS_ORPHAN_RESOURCES: {
			_orphan_resources_dialog->show();
		} break;
		case TOOLS_BUILD_PROFILE_MANAGER: {
			_build_profile_manager->popup_centered_clamped(Size2(700, 800) * EDSCALE, 0.8);
		} break;
		case TOOLS_CLEAR_PROJECT_CACHE: {
			_clear_cache_dialog->popup_centered();
		} break;
		case TOOLS_CUSTOM: {
			if (_tool_menu->get_item_submenu(p_idx) == "") {
				Callable callback = _tool_menu->get_item_metadata(p_idx);
				Callable::CallError ce;
				Variant result;
				callback.callp(nullptr, 0, result, ce);

				if (ce.error != Callable::CallError::CALL_OK) {
					String err = Variant::get_callable_error_text(callback, nullptr, 0, ce);
					ERR_PRINT("Error calling function from tool menu: " + err);
				}
			} // Else it's a submenu so don't do anything.
		} break;
	}
}

void EditorNode::_export_as_menu_option(int p_idx) {
	if (p_idx == 0) { // MeshLibrary
		_current_menu_option = FILE_EXPORT_MESH_LIBRARY;

		if (!_editor_data.get_edited_scene_root()) {
			show_warning(TTR("This operation can't be done without a scene."));
			return;
		}

		List<String> extensions;
		Ref<MeshLibrary> ml(memnew(MeshLibrary));
		ResourceSaver::get_recognized_extensions(ml, &extensions);
		_mesh_library_export_dialog->clear_filters();
		for (const String &extension : extensions) {
			_mesh_library_export_dialog->add_filter("*." + extension);
		}

		_mesh_library_export_dialog->set_title(TTR("Export Mesh Library"));
		_mesh_library_export_dialog->popup_file_dialog();
	} else { // Custom menu options added by plugins
		if (_export_as_menu->get_item_submenu(p_idx).is_empty()) { // If not a submenu
			Callable callback = _export_as_menu->get_item_metadata(p_idx);
			Callable::CallError ce;
			Variant result;
			callback.callp(nullptr, 0, result, ce);

			if (ce.error != Callable::CallError::CALL_OK) {
				String err = Variant::get_callable_error_text(callback, nullptr, 0, ce);
				ERR_PRINT("Error calling function from export_as menu: " + err);
			}
		}
	}
}

void EditorNode::_update_file_menu_opened() {
	bool has_unsaved = false;
	for (int i = 0; i < _editor_data.get_edited_scene_count(); i++) {
		if (is_scene_unsaved(i)) {
			has_unsaved = true;
			break;
		}
	}
	if (has_unsaved) {
		_file_menu->set_item_disabled(_file_menu->get_item_index(SCENE_SAVE_ALL_SCENES), false);
		_file_menu->set_item_tooltip(_file_menu->get_item_index(SCENE_SAVE_ALL_SCENES), String());
	} else {
		_file_menu->set_item_disabled(_file_menu->get_item_index(SCENE_SAVE_ALL_SCENES), true);
		_file_menu->set_item_tooltip(_file_menu->get_item_index(SCENE_SAVE_ALL_SCENES), TTR("All scenes are already saved."));
	}
}

void EditorNode::_palette_quick_open_dialog() {
	_quick_open_color_palette->popup_dialog({ "ColorPalette" }, _palette_file_selected_callback);
	_quick_open_color_palette->set_title(TTRC("Quick Open Color Palette..."));
}

void EditorNode::_clear_cache_confirmed() {
	_menu_option_confirm(TOOLS_CLEAR_PROJECT_CACHE, false);
}

void EditorNode::_plugin_over_edit(EditorPlugin *r_plugin, Object *p_object, bool p_set_current) {
	if (p_object) {
		_editor_plugins_over->add_plugin(r_plugin);
		r_plugin->edit(p_object);
		r_plugin->make_visible(true);
		if (p_set_current) {
			r_plugin->set_current();
		}
	} else {
		_editor_plugins_over->remove_plugin(r_plugin);
		r_plugin->edit(nullptr);
		r_plugin->make_visible(false);
	}
}

void EditorNode::_register_plugin_as_its_own_owner(EditorPlugin *p_plugin) {
	_active_plugins[p_plugin->get_instance_id()].insert(p_plugin);
}

void EditorNode::_filesystem_changed() {
	for (FileDialog *file_dialog : _file_dialogs) {
		file_dialog->invalidate();
	}

	_mark_unsaved_scenes();

	// FIXME: Move this to a cleaner location, it's hacky to do this in _filesystem_changed.
	String export_error;
	Error err = OK;
	// It's important to wait for the first scan to finish; otherwise, scripts or resources might not be imported.
	if (!_deferred_export.preset.is_empty() && !EditorFileSystem::get_singleton()->is_scanning()) {
		String preset_name = _deferred_export.preset;
		// Ensures export_project does not loop infinitely, because notifications may
		// come during the export.
		_deferred_export.preset = "";
		Ref<EditorExportPreset> export_preset;
		for (int i = 0; i < EditorExport::get_singleton()->get_export_preset_count(); ++i) {
			export_preset = EditorExport::get_singleton()->get_export_preset(i);
			if (export_preset->get_name() == preset_name) {
				break;
			}
			export_preset.unref();
		}

		if (export_preset.is_null()) {
			Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
			if (da->file_exists("res://export_presets.cfg")) {
				err = FAILED;
				export_error = vformat(
						"Invalid export preset name: %s.\nThe following presets were detected in this project's `export_presets.cfg`:\n\n",
						preset_name);
				for (int i = 0; i < EditorExport::get_singleton()->get_export_preset_count(); ++i) {
					// Write the preset name between double quotes since it needs to be written between quotes on the command line if it contains spaces.
					export_error += vformat("        \"%s\"\n", EditorExport::get_singleton()->get_export_preset(i)->get_name());
				}
			} else {
				err = FAILED;
				export_error = "This project doesn't have an `export_presets.cfg` file at its root.\nCreate an export preset from the \"Project > Export\" dialog and try again.";
			}
		} else {
			Ref<EditorExportPlatform> platform = export_preset->get_platform();
			const String export_path = _deferred_export.path.is_empty() ? export_preset->get_export_path() : _deferred_export.path;
			if (export_path.is_empty()) {
				err = FAILED;
				export_error = vformat("Export preset \"%s\" doesn't have a default export path, and none was specified.", preset_name);
			} else if (platform.is_null()) {
				err = FAILED;
				export_error = vformat("Export preset \"%s\" doesn't have a matching platform.", preset_name);
			} else {
				export_preset->update_value_overrides();
				if (_deferred_export.pack_only) { // Only export .pck or .zip data pack.
					if (export_path.ends_with(".zip")) {
						if (_deferred_export.patch) {
							err = platform->export_zip_patch(export_preset, _deferred_export.debug, export_path, _deferred_export.patches);
						} else {
							err = platform->export_zip(export_preset, _deferred_export.debug, export_path);
						}
					} else if (export_path.ends_with(".pck")) {
						if (_deferred_export.patch) {
							err = platform->export_pack_patch(export_preset, _deferred_export.debug, export_path, _deferred_export.patches);
						} else {
							err = platform->export_pack(export_preset, _deferred_export.debug, export_path);
						}
					} else {
						ERR_PRINT(vformat("Export path \"%s\" doesn't end with a supported extension.", export_path));
						err = FAILED;
					}
				} else { // Normal project export.
					String config_error;
					bool missing_templates;
					if (_deferred_export.android_build_template) {
						_export_template_manager->install_android_template(export_preset);
					}
					if (!platform->can_export(export_preset, config_error, missing_templates, _deferred_export.debug)) {
						ERR_PRINT(vformat("Cannot export project with preset \"%s\" due to configuration errors:\n%s", preset_name, config_error));
						err = missing_templates ? ERR_FILE_NOT_FOUND : ERR_UNCONFIGURED;
					} else {
						platform->clear_messages();
						err = platform->export_project(export_preset, _deferred_export.debug, export_path);
					}
				}
				if (err != OK) {
					export_error = vformat("Project export for preset \"%s\" failed.", preset_name);
				} else if (platform->get_worst_message_type() >= EditorExportPlatform::EXPORT_MESSAGE_WARNING) {
					export_error = vformat("Project export for preset \"%s\" completed with warnings.", preset_name);
				}
			}
		}

		if (err != OK) {
			ERR_PRINT(export_error);
			_exit_editor(EXIT_FAILURE);
			return;
		}
		if (!export_error.is_empty()) {
			WARN_PRINT(export_error);
		}
		_exit_editor(EXIT_SUCCESS);
	}
}

void EditorNode::_resources_reimporting(const Vector<String> &p_resources) {
	// This will copy all the modified properties of the nodes into 'scenes_modification_table'
	// before they are actually reimported. It's important to do this before the reimportation
	// because if a mesh is present in an inherited scene, the resource will be modified in
	// the inherited scene. Then, get_modified_properties_for_node will return the mesh property,
	// which will trigger a recopy of the previous mesh, preventing the reload.
	_scenes_modification_table.clear();
	_scenes_reimported.clear();
	_resources_pending_reimport.clear();
	EditorFileSystem *editor_file_system = EditorFileSystem::get_singleton();
	for (const String &res_path : p_resources) {
		// It's faster to use EditorFileSystem::get_file_type than fetching the resource type from disk.
		// This makes a big difference when reimporting many resources.
		String file_type = editor_file_system->get_file_type(res_path);
		if (file_type.is_empty()) {
			file_type = ResourceLoader::get_resource_type(res_path);
		}
		if (file_type == "PackedScene") {
			_scenes_reimported.push_back(res_path);
		} else {
			_resources_pending_reimport.push_back(res_path);
		}
	}

	if (_scenes_reimported.size() > 0) {
		preload_reimporting_with_path_in_edited_scenes(_scenes_reimported);
	}
}

void EditorNode::_resources_reimported(const Vector<String> &p_resources) {
	int current_tab = _scene_tabs->get_current_tab();

	for (const String &res_path : _resources_pending_reimport) {
		if (!ResourceCache::has(res_path)) {
			// Not loaded, no need to reload.
			continue;
		}
		// Reload normally.
		Ref<Resource> resource = ResourceCache::get_ref(res_path);
		if (resource.is_valid()) {
			resource->reload_from_file();
		}
	}

	// Editor may crash when related animation is playing while re-importing GLTF scene, stop it in advance.
	AnimationPlayer *ap = AnimationPlayerEditor::get_singleton()->get_player();
	if (ap && _scenes_reimported.size() > 0) {
		ap->stop(true);
	}

	// Only refresh the current scene tab if it's been reimported.
	// Otherwise the scene tab will try to grab focus unnecessarily.
	bool should_refresh_current_scene_tab = false;
	const String current_scene_tab = _editor_data.get_scene_path(current_tab);
	for (const String &reimported_scene_path : _scenes_reimported) {
		if (!should_refresh_current_scene_tab && reimported_scene_path == current_scene_tab) {
			should_refresh_current_scene_tab = true;
		}
		if (_editor_data.get_edited_scene_from_path(reimported_scene_path) != -1) {
			reload_scene(reimported_scene_path);
		}
	}

	reload_instances_with_path_in_edited_scenes();

	_scenes_modification_table.clear();
	_scenes_reimported.clear();
	_resources_pending_reimport.clear();

	if (should_refresh_current_scene_tab) {
		_apply_edited_scene(current_tab);
	}
}

void EditorNode::_sources_changed(bool p_exist) {
	if (_waiting_for_first_scan) {
		_waiting_for_first_scan = false;

		OS::get_singleton()->benchmark_end_measure("Editor", "First Scan");

		// Reload the global shader variables, but this time
		// loading textures, as they are now properly imported.
		RenderingServer::get_singleton()->global_shader_parameters_load_settings(true);

		_load_editor_layout();

		if (!_defer_load_scene.is_empty()) {
			OS::get_singleton()->benchmark_begin_measure("Editor", "Load Scene");

			open_scene(_defer_load_scene);
			_defer_load_scene = "";

			OS::get_singleton()->benchmark_end_measure("Editor", "Load Scene");
			OS::get_singleton()->benchmark_dump();
		}

		// Start preview thread now that it's safe.
		if (!_singleton->_cmdline_mode) {
			EditorResourcePreview::get_singleton()->start();
		}

		get_tree()->create_timer(1.0f)->connect("timeout", callable_mp(this, &EditorNode::_remove_lock_file));
	}
}

void EditorNode::_remove_lock_file() {
	OS::get_singleton()->remove_lock_file();
}

void EditorNode::_node_renamed() {
	if (InspectorDock::get_inspector_singleton()) {
		InspectorDock::get_inspector_singleton()->update_tree();
	}
}

void EditorNode::_save_editor_states(const String &p_file, int p_idx) {
	Node *scene = _editor_data.get_edited_scene_root(p_idx);
	bool saving_current_scene = p_idx < 0 || _editor_data.get_edited_scene() == p_idx;
	if (saving_current_scene && !scene) {
		return;
	}

	String path = EditorPaths::get_singleton()->get_project_settings_dir().path_join(p_file.get_file() + "-editstate-" + p_file.md5_text() + ".cfg");

	Ref<ConfigFile> cf;
	cf.instantiate();

	Dictionary md;
	if (saving_current_scene) {
		md = _editor_data.get_editor_plugin_states();

		// Save the currently selected nodes.

		List<Node *> selection = _editor_selection->get_full_selected_node_list();
		TypedArray<NodePath> selection_paths;
		for (Node *selected_node : selection) {
			selection_paths.push_back(scene->get_path_to(selected_node));
		}
		cf->set_value("editor_states", "$selected_nodes", selection_paths);
	} else {
		md = _editor_data.get_scene_editor_states_with_selection(p_idx);
	}

	for (const KeyValue<Variant, Variant> &kv : md) {
		cf->set_value("editor_states", kv.key, kv.value);
	}

	Error err = cf->save(path);
	ERR_FAIL_COND_MSG(err != OK, "Cannot save config file to '" + path + "'.");
}

void EditorNode::_update_title() {
	const String project_name = GLOBAL_GET("application/config/about/name");
	String title = project_name.is_empty() ? TTR("Unnamed Project") : project_name;
	if (_unsaved_cache) {
		// Display the "modified" mark before anything else so that it can always be seen in the OS task bar.
		title = vformat("(*) %s", title);
	}
	DisplayServer::get_singleton()->window_set_title(title + String(" - ") + GODOT_VERSION_NAME);
	if (_project_title_lbl) {
		_project_title_lbl->set_text(title);
	}
}

void EditorNode::_update_unsaved_cache() {
	bool is_unsaved = EditorUndoRedoManager::get_singleton()->is_history_unsaved(EditorUndoRedoManager::GLOBAL_HISTORY) ||
			EditorUndoRedoManager::get_singleton()->is_history_unsaved(_editor_data.get_current_edited_scene_history_id());

	if (_unsaved_cache != is_unsaved) {
		_unsaved_cache = is_unsaved;
		_update_title();
	}
}

void EditorNode::_version_control_menu_option(int p_idx) {
	switch (_vcs_actions_menu->get_item_id(p_idx)) {
		case VCS_METADATA: {
			VersionControlEditorPlugin::get_singleton()->popup_vcs_metadata_dialog();
		} break;
		case VCS_SETTINGS: {
			VersionControlEditorPlugin::get_singleton()->popup_vcs_set_up_dialog(_editor_overlay);
		} break;
	}
}

void EditorNode::_titlebar_resized() {
	DisplayServer::get_singleton()->window_set_window_buttons_offset(Vector2i(_editor_header->get_global_position().y + _editor_header->get_size().y / 2, _editor_header->get_global_position().y + _editor_header->get_size().y / 2), DisplayServerEnums::MAIN_WINDOW_ID);
	const Vector3i &margin = DisplayServer::get_singleton()->window_get_safe_title_margins(DisplayServerEnums::MAIN_WINDOW_ID);
	if (_left_menu_spacer) {
		int w = (margin.x);
		_left_menu_spacer->set_custom_minimum_size(Size2(w, 0));
		_left_menu_spacer->set_visible(w > 0);
	}
	if (_editor_header) {
		_editor_header->set_custom_minimum_size(Size2(0, margin.z - _editor_header->get_global_position().y));
	}
}

void EditorNode::_viewport_resized() {
	Window *w = get_window();
	if (w) {
		_was_window_windowed_last = w->get_mode() == Window::MODE_WINDOWED;
	}
}

int EditorNode::_save_external_resources(bool p_also_save_external_data) {
	// Save external resources and its subresources if any was modified.

	int flg = 0;
	if (EDITOR_GET("filesystem/on_save/compress_binary_resources")) {
		flg |= ResourceSaver::FLAG_COMPRESS;
	}
	flg |= ResourceSaver::FLAG_REPLACE_SUBRESOURCE_PATHS;

	HashSet<String> edited_resources;
	int saved = 0;
	List<Ref<Resource>> cached;
	ResourceCache::get_cached_resources(&cached);

	for (Ref<Resource> res : cached) {
		if (!res->is_edited()) {
			continue;
		}

		String path = res->get_path();
		if (path.begins_with("res://") && !FileAccessContent::is_content_path(path)) {
			int subres_pos = path.find("::");
			if (subres_pos == -1) {
				// Actual resource.
				edited_resources.insert(path);
			} else {
				edited_resources.insert(path.substr(0, subres_pos));
			}
		}

		res->set_edited(false);
	}

	bool script_was_saved = false;
	for (const String &edited_resource_path : edited_resources) {
		Ref<Resource> res = ResourceCache::get_ref(edited_resource_path);
		if (res.is_null()) {
			continue; // Maybe it was erased in a thread, who knows.
		}
		Ref<PackedScene> ps = res;
		if (ps.is_valid()) {
			continue; // Do not save PackedScenes, this will mess up the editor.
		}
		if (!script_was_saved) {
			Ref<Script> scr = res;
			script_was_saved = scr.is_valid();
		}
		ResourceSaver::save(res, res->get_path(), flg);
		saved++;
	}

	if (script_was_saved) {
		ScriptEditorDock::get_singleton()->update_script_times();
	}

	if (p_also_save_external_data) {
		ScriptEditorDock *script_editor = ScriptEditorDock::get_singleton();
		if (!script_editor->get_unsaved_status().is_empty()) {
			script_editor->save_external_data();
			saved++;
		}
		for (int i = 0; i < _editor_data.get_editor_plugin_count(); i++) {
			EditorPlugin *plugin = _editor_data.get_editor_plugin(i);
			if (!plugin->get_unsaved_status().is_empty()) {
				plugin->save_external_data();
				saved++;
			}
		}
	}

	EditorSettings::get_singleton()->save_project_metadata();
	EditorUndoRedoManager::get_singleton()->set_history_as_saved(EditorUndoRedoManager::GLOBAL_HISTORY);
	_update_unsaved_cache();

	return saved;
}

void EditorNode::_save_scene_silently() {
	// Save scene without displaying progress dialog. Used to work around
	// errors about parent node being busy setting up children
	// when Save on Focus Loss kicks in.
	Node *scene = _editor_data.get_edited_scene_root();
	if (scene && !scene->get_scene_file_path().is_empty() && FileAccess::exists(scene->get_scene_file_path())) {
		_save_scene(scene->get_scene_file_path());
		save_editor_layout_delayed();
	}
}

void EditorNode::_set_current_scene(int p_idx) {
	if (p_idx == _editor_data.get_edited_scene()) {
		return; // Pointless.
	}

	_apply_edited_scene(p_idx);
}

void EditorNode::_apply_edited_scene(int p_idx, bool p_ignore_state) {
	// Save the folding in case the scene gets reloaded.
	const String scene_path = _editor_data.get_scene_path(p_idx);
	if (scene_path.is_empty() && _editor_data.get_edited_scene_root(p_idx)) {
		_editor_folding.save_scene_folding(_editor_data.get_edited_scene_root(p_idx), _editor_data.get_scene_path(p_idx));
	}

	_changing_scene = true;
	if (!p_ignore_state) {
		_editor_data.save_edited_scene_state(_editor_selection, &_editor_history, _get_main_scene_state());
	}

	Node *old_scene = get_editor_data().get_edited_scene_root();

	_editor_selection->clear();
	HierarchyDock::get_singleton()->clear_previous_node_selection();
	_editor_data.set_edited_scene(p_idx);

	Node *new_scene = _editor_data.get_edited_scene_root();

	// Remove the scene only if it's a new scene, preventing performance issues when adding and removing scenes.
	if (old_scene && new_scene != old_scene && old_scene->get_parent() == _edited_scene_viewport) {
		_edited_scene_viewport->remove_child(old_scene);
	}

	if (Popup *p = Object::cast_to<Popup>(new_scene)) {
		p->show();
	}

	HierarchyDock::get_singleton()->set_edited_scene(new_scene);
	if (get_tree()) {
		get_tree()->set_edited_scene_root(new_scene);
	}

	if (new_scene && new_scene->get_parent() != _edited_scene_viewport) {
		_edited_scene_viewport->add_child(new_scene, true);
	}

	if (_editor_data.check_and_update_scene(p_idx)) {
		if (!_editor_data.get_scene_path(p_idx).is_empty()) {
			_editor_folding.load_scene_folding(_editor_data.get_edited_scene_root(p_idx), scene_path);
		}

		EditorUndoRedoManager::get_singleton()->clear_history(_editor_data.get_scene_history_id(p_idx), false);
	}
	_resource_node_references.clear();
	HierarchyDock::get_singleton()->get_hierarchy_editor()->update_tree();

	_update_title();

	const Dictionary state = _editor_data.restore_edited_scene_state(_editor_selection, &_editor_history);
	_set_main_scene_state(state);
	_update_unsaved_cache();

	_changing_scene = false;

	if (EDITOR_GET("interface/scene_tabs/auto_select_current_scene_file")) {
		FileSystemDock::get_singleton()->navigate_to_path(scene_path);
	}
}

bool EditorNode::_validate_scene_recursive(const String &p_filename, Node *p_node) {
	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node *child = p_node->get_child(i);
		if (child->get_scene_file_path() == p_filename) {
			return true;
		}

		if (_validate_scene_recursive(p_filename, child)) {
			return true;
		}
	}

	return false;
}

void EditorNode::_save_scene(String p_file, int p_idx) {
	ERR_FAIL_COND_MSG(!_saving_scene_path.is_empty() && _saving_scene_path == p_file, "Scene saved while already being saved!");

	Node *scene = _editor_data.get_edited_scene_root(p_idx);

	if (!scene) {
		show_warning(TTR("This operation can't be done without a tree root."));
		return;
	}

	if (!scene->get_scene_file_path().is_empty() && _validate_scene_recursive(scene->get_scene_file_path(), scene)) {
		show_warning(TTR("This scene can't be saved because there is a cyclic instance inclusion.\nPlease resolve it and then attempt to save again."));
		return;
	}

	scene->propagate_notification(NOTIFICATION_EDITOR_PRE_SAVE);

	_editor_data.apply_changes_in_editors();
	save_default_environment();
	List<Pair<AnimationMixer *, Ref<AnimatedValuesBackup>>> anim_backups;
	_reset_animation_mixers(scene, &anim_backups);
	_save_editor_states(p_file, p_idx);

	Ref<PackedScene> sdata;

	if (ResourceCache::has(p_file)) {
		// Something may be referencing this resource and we are good with that.
		// We must update it, but also let the previous scene state go, as
		// old version still work for referencing changes in instantiated or inherited scenes.

		sdata = ResourceCache::get_ref(p_file);
		if (sdata.is_valid()) {
			sdata->recreate_state();
		} else {
			sdata.instantiate();
		}
	} else {
		sdata.instantiate();
	}
	Error err = sdata->pack(scene);

	if (err != OK) {
		show_warning(TTR("Couldn't save scene. Likely dependencies (instances or inheritance) couldn't be satisfied."));
		return;
	}

	int flg = 0;
	if (EDITOR_GET("filesystem/on_save/compress_binary_resources")) {
		flg |= ResourceSaver::FLAG_COMPRESS;
	}
	flg |= ResourceSaver::FLAG_REPLACE_SUBRESOURCE_PATHS;

	err = ResourceSaver::save(sdata, p_file, flg);

	// This needs to be emitted before saving external resources.
	emit_signal(SNAME("scene_saved"), p_file);
	_editor_data.notify_scene_saved(p_file);

	_save_external_resources();
	_saving_scene_path = p_file; // Some editors may save scenes of built-in resources as external data, so avoid saving this scene again.
	_editor_data.save_editor_external_data();
	_saving_scene_path = "";

	for (Pair<AnimationMixer *, Ref<AnimatedValuesBackup>> &animation_backup : anim_backups) {
		animation_backup.first->restore(animation_backup.second);
	}

	if (err == OK) {
		scene->set_scene_file_path(ProjectSettings::get_singleton()->localize_path(p_file));
		_editor_data.set_scene_as_saved(p_idx);
		_editor_data.set_scene_modified_time(p_idx, FileAccess::get_modified_time(p_file));

		if (EDITOR_GET("filesystem/on_save/warn_on_saving_large_text_resources")) {
			if (p_file.ends_with(".tscn") || p_file.ends_with(".tres")) {
				const int64_t file_size = FileAccess::get_size(p_file);
				if (file_size >= LARGE_RESOURCE_WARNING_SIZE_THRESHOLD) {
					// File is larger than 500 KiB, likely because it contains binary data serialized as Base64.
					// This is slow to save and load, so warn the user.
					EditorToasterContainer::get_singleton()->popup_str(
							vformat(TTR("The text-based scene at path \"%s\" is large on disk (%s), likely because it has embedded binary data.\nThis slows down scene saving and loading.\nConsider saving its binary subresource(s) to a binary `.res` file or saving the scene as a binary `.scn` file.\nThis warning can be disabled in the Editor Settings (FileSystem > On Save > Warn on Saving Large Text Resources)."), p_file, String::humanize_size(file_size)), EditorToasterContainer::WARNING);
				}
			}
		}

		_editor_folding.save_scene_folding(scene, p_file);

		_update_title();
		_scene_tabs->update_scene_tabs();
	} else {
		_dialog_display_save_error(p_file, err);
	}

	scene->propagate_notification(NOTIFICATION_EDITOR_POST_SAVE);
	_update_unsaved_cache();
}

void EditorNode::_save_all_scenes() {
	_scenes_to_save_as.clear(); // In case saving was canceled before.
	for (int i = 0; i < _editor_data.get_edited_scene_count(); i++) {
		if (!is_scene_unsaved(i)) {
			continue;
		}

		const Node *scene = _editor_data.get_edited_scene_root(i);
		ERR_FAIL_NULL(scene);

		const String &scene_path = scene->get_scene_file_path();
		if (scene_path.is_empty() || !DirAccess::exists(scene_path.get_base_dir())) {
			_scenes_to_save_as.push_back(i);
			continue;
		}

		if (i == _editor_data.get_edited_scene()) {
			_save_scene_with_preview(scene_path);
		} else {
			_save_scene(scene_path, i);
		}
	}
	save_default_environment();

	if (!_scenes_to_save_as.is_empty()) {
		_save_next_scene_tab_as();
	}
}

int EditorNode::_next_unsaved_scene(bool p_valid_filename, int p_start) {
	for (int i = p_start; i < _editor_data.get_edited_scene_count(); i++) {
		if (!_editor_data.get_edited_scene_root(i)) {
			continue;
		}

		String scene_filename = _editor_data.get_edited_scene_root(i)->get_scene_file_path();
		if (p_valid_filename && scene_filename.is_empty()) {
			continue;
		}

		bool unsaved = EditorUndoRedoManager::get_singleton()->is_history_unsaved(_editor_data.get_scene_history_id(i));
		if (unsaved) {
			return i;
		} else if (!_editor_data.get_unsaved_status(scene_filename).is_empty()) {
			return i;
		}
	}
	return -1;
}

void EditorNode::_discard_changes(const String &p_str) {
	switch (_current_menu_option) {
		case SCENE_CLOSE:
		case SCENE_TAB_CLOSE: {
			const String path = _editor_data.get_scene_path(_tab_closing_idx);
			if (!path.is_empty()) {
				_update_prev_closed_scenes(path, true);
			}

			// Don't close tabs when exiting the editor (required for "restore_scenes_on_load" setting).
			if (!_is_closing_editor()) {
				_remove_scene(_tab_closing_idx);
				_scene_tabs->update_scene_tabs();
			}
			_proceed_closing_scene_tabs();
		} break;
		case SCENE_RELOAD_SAVED_SCENE: {
			int cur_idx = _editor_data.get_edited_scene();
			reload_scene(_editor_data.get_scene_path(cur_idx));
			_confirmation_dialog->hide();
		} break;
		case SCENE_QUIT: {
			_project_run_bar->stop_playing();
			_exit_editor(EXIT_SUCCESS);

		} break;
		case PROJECT_QUIT_TO_HUB: {
			_restart_editor(true);
		} break;
		case PROJECT_RELOAD_CURRENT_PROJECT: {
			_restart_editor();
		} break;
		case TOOLS_CLEAR_PROJECT_CACHE: {
			_files_to_delete_on_exit = LocalVector<String>{
				"res://.godot/imported",
				"res://.godot/exported",
				"res://.godot/global_script_class_cache.cfg",
				"res://.godot/scene_groups_cache.cfg",
				"res://.godot/uid_cache.bin",
				"res://.godot/editor/editor_script_doc_cache.res",
				"res://.godot/editor/" + EditorFileSystem::CACHE_FILE_NAME,
				"res://.godot/editor/filesystem_update4",
			};
			_restart_editor(false);
		} break;
	}
}

void EditorNode::_scene_tab_closed(int p_tab) {
	_current_menu_option = SCENE_TAB_CLOSE;
	_tab_closing_idx = p_tab;
	Node *scene = _editor_data.get_edited_scene_root(p_tab);
	if (!scene) {
		_discard_changes();
		return;
	}

	String scene_filename = scene->get_scene_file_path();
	String unsaved_message;

	if (EditorUndoRedoManager::get_singleton()->is_history_unsaved(_editor_data.get_scene_history_id(p_tab))) {
		if (scene_filename.is_empty()) {
			unsaved_message = TTR("This scene was never saved.");
		} else {
			uint32_t time_opened = _editor_data.get_scene_time_opened(p_tab);
			unsaved_message = _get_unsaved_scene_dialog_text(scene_filename, time_opened);
		}
	} else {
		unsaved_message = _editor_data.get_unsaved_status(scene_filename);
	}

	if (!unsaved_message.is_empty()) {
		_save_confirmation_dialog->set_ok_button_text(TTR("Save & Close"));
		_save_confirmation_dialog->set_text(unsaved_message + "\n\n" + TTR("Save before closing?"));
		_save_confirmation_dialog->reset_size();
		_save_confirmation_dialog->popup_centered();
	} else {
		_discard_changes();
	}

	save_editor_layout_delayed();
	_scene_tabs->update_scene_tabs();
}

void EditorNode::_cancel_close_scene_tab() {
	if (_is_closing_editor()) {
		_tab_closing_menu_option = -1;
	}
	_changing_scene = false;
	_tabs_to_close.clear();
}

void EditorNode::_cancel_confirmation() {
	_stop_project_confirmation = false;
}

void EditorNode::_prepare_save_confirmation_popup() {
	if (_save_confirmation_dialog->get_window() != get_last_exclusive_window()) {
		_save_confirmation_dialog->reparent(get_last_exclusive_window());
	}
}

void EditorNode::_inherit_request(String p_file) {
	_current_menu_option = SCENE_NEW_INHERITED_SCENE;
	_dialog_action(p_file);
}

void EditorNode::_instantiate_request(const Vector<String> &p_files) {
	request_instantiate_scenes(p_files);
}

void EditorNode::_quick_opened(const String &p_file_path) {
	load_scene_or_resource(p_file_path);
}

void EditorNode::_open_command_palette() {
	_command_palette->open_popup();
}

void EditorNode::_project_run_started() {
	if (bool(EDITOR_GET("run/output/always_clear_output_on_play"))) {
		_log->clear();
	}

	const ActionOnPlay action_on_play = static_cast<ActionOnPlay>(int(EDITOR_GET("run/docks/action_on_play")));
	if (action_on_play == ActionOnPlay::OPEN_OUTPUT) {
		_editor_dock_manager->open_dock(_log, true);
	} else if (action_on_play == ActionOnPlay::OPEN_DEBUGGER) {
		_editor_dock_manager->open_dock(EditorDebuggerNode::get_singleton(), true);
	}
}

void EditorNode::_update_prev_closed_scenes(const String &p_scene_path, bool p_add_scene) {
	if (!p_scene_path.is_empty()) {
		const String scene_uid = ResourceUID::path_to_uid(p_scene_path);
		if (p_add_scene) {
			_prev_closed_scenes.push_back(scene_uid);
		} else {
			_prev_closed_scenes.erase(scene_uid);
		}
		_file_menu->set_item_disabled(_file_menu->get_item_index(SCENE_OPEN_PREV), _prev_closed_scenes.is_empty());
	}
}

void EditorNode::_add_to_recent_scenes(const String &p_scene) {
	Array rc = EditorSettings::get_singleton()->get_project_metadata("recent_files", "scenes", Array());
	const String scene_uid = ResourceUID::path_to_uid(p_scene);

	if (rc.has(scene_uid)) {
		rc.erase(scene_uid);
	}
	rc.push_front(scene_uid);
	if (rc.size() > 10) {
		rc.resize(10);
	}

	EditorSettings::get_singleton()->set_project_metadata("recent_files", "scenes", rc);
}

void EditorNode::_update_recent_scenes() {
	Array rc = EditorSettings::get_singleton()->get_project_metadata("recent_files", "scenes", Array());
	_recent_scenes_menu->clear();

	LocalVector<int> missing_scenes;
	if (rc.size() == 0) {
		_recent_scenes_menu->add_item(TTRC("No Recent Scenes"), -1);
		_recent_scenes_menu->set_item_disabled(-1, true);
	} else {
		for (int i = 0; i < rc.size(); i++) {
			const String path = ResourceUID::ensure_path_nocheck(rc[i]);
			if (!path.is_empty() && ResourceLoader::exists(path)) {
				_recent_scenes_menu->add_item(path.trim_prefix("res://"), i);
			} else {
				missing_scenes.push_back(i);
			}
		}

		_recent_scenes_menu->add_separator();
		_recent_scenes_menu->add_shortcut(ED_SHORTCUT("editor/clear_recent", TTRC("Clear Recent Scenes")));

		if (!missing_scenes.is_empty()) {
			// Some scenes are missing, so update the stored list.
			Array new_scenes;
			for (int i = 0; i < rc.size(); i++) {
				if (!missing_scenes.has(i)) {
					new_scenes.push_back(rc[i]);
				}
			}
			EditorSettings::get_singleton()->set_project_metadata("recent_files", "scenes", new_scenes);
		}
	}
	_recent_scenes_menu->set_item_auto_translate_mode(-1, AUTO_TRANSLATE_MODE_ALWAYS);
	_recent_scenes_menu->reset_size();
}

void EditorNode::_open_recent_scene(int p_idx) {
	if (p_idx == _recent_scenes_menu->get_item_count() - 1) {
		EditorSettings::get_singleton()->set_project_metadata("recent_files", "scenes", Array());
	} else {
		Array rc = EditorSettings::get_singleton()->get_project_metadata("recent_files", "scenes", Array());
		ERR_FAIL_INDEX(p_idx, rc.size());

		if (open_scene(rc[p_idx]) != OK) {
			rc.remove_at(p_idx);
			EditorSettings::get_singleton()->set_project_metadata("recent_files", "scenes", rc);
		}
	}
}

void EditorNode::_add_dropped_files_recursive(const Vector<String> &p_files, String p_to_path) {
	Ref<DirAccess> dir = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	ERR_FAIL_COND(dir.is_null());

	for (int i = 0; i < p_files.size(); i++) {
		const String &from = p_files[i];
		String to = p_to_path.path_join(from.get_file());

		if (dir->dir_exists(from)) {
			Vector<String> sub_files;

			Ref<DirAccess> sub_dir = DirAccess::open(from);
			ERR_FAIL_COND(sub_dir.is_null());

			sub_dir->list_dir_begin();

			String next_file = sub_dir->get_next();
			while (!next_file.is_empty()) {
				if (next_file == "." || next_file == "..") {
					next_file = sub_dir->get_next();
					continue;
				}

				sub_files.push_back(from.path_join(next_file));
				next_file = sub_dir->get_next();
			}

			if (!sub_files.is_empty()) {
				dir->make_dir(to);
				_add_dropped_files_recursive(sub_files, to);
			}

			continue;
		}

		dir->copy(from, to);
	}
}

void EditorNode::_update_vsync_mode() {
	const DisplayServerEnums::VSyncMode window_vsync_mode = DisplayServerEnums::VSyncMode(int(EDITOR_GET("interface/editor/display/vsync_mode")));
	DisplayServer::get_singleton()->window_set_vsync_mode(window_vsync_mode);
}

void EditorNode::_update_from_settings() {
	if (!is_inside_tree()) {
		return;
	}
	_update_title();

	int current_filter = GLOBAL_GET("rendering/textures/canvas_textures/default_texture_filter");
	if (current_filter != _edited_scene_viewport->get_default_canvas_item_texture_filter()) {
		Viewport::DefaultCanvasItemTextureFilter tf = (Viewport::DefaultCanvasItemTextureFilter)current_filter;
		_edited_scene_viewport->set_default_canvas_item_texture_filter(tf);
	}
	int current_repeat = GLOBAL_GET("rendering/textures/canvas_textures/default_texture_repeat");
	if (current_repeat != _edited_scene_viewport->get_default_canvas_item_texture_repeat()) {
		Viewport::DefaultCanvasItemTextureRepeat tr = (Viewport::DefaultCanvasItemTextureRepeat)current_repeat;
		_edited_scene_viewport->set_default_canvas_item_texture_repeat(tr);
	}
	String current_default_context = GLOBAL_GET("internationalization/locale/default_context");
	if (current_default_context != TranslationServer::get_singleton()->get_default_context()) {
		TranslationServer::get_singleton()->set_default_context(current_default_context);
		Ref<TranslationDomain> domain = TranslationServer::get_singleton()->get_main_domain();
		if (!domain->is_enabled()) {
			domain->set_locale_override(TranslationServer::get_singleton()->get_default_context());
		}
	}

	RSE::DOFBokehShape dof_shape = RSE::DOFBokehShape(int(GLOBAL_GET("rendering/camera/depth_of_field/depth_of_field_bokeh_shape")));
	RS::get_singleton()->camera_attributes_set_dof_blur_bokeh_shape(dof_shape);
	RSE::DOFBlurQuality dof_quality = RSE::DOFBlurQuality(int(GLOBAL_GET("rendering/camera/depth_of_field/depth_of_field_bokeh_quality")));
	bool dof_jitter = GLOBAL_GET("rendering/camera/depth_of_field/depth_of_field_use_jitter");
	RS::get_singleton()->camera_attributes_set_dof_blur_quality(dof_quality, dof_jitter);
	RS::get_singleton()->environment_set_ssao_quality(RSE::EnvironmentSSAOQuality(int(GLOBAL_GET("rendering/environment/ssao/quality"))), GLOBAL_GET("rendering/environment/ssao/half_size"), GLOBAL_GET("rendering/environment/ssao/adaptive_target"), GLOBAL_GET("rendering/environment/ssao/blur_passes"), GLOBAL_GET("rendering/environment/ssao/fadeout_from"), GLOBAL_GET("rendering/environment/ssao/fadeout_to"));
	RS::get_singleton()->screen_space_roughness_limiter_set_active(GLOBAL_GET("rendering/anti_aliasing/screen_space_roughness_limiter/enabled"), GLOBAL_GET("rendering/anti_aliasing/screen_space_roughness_limiter/amount"), GLOBAL_GET("rendering/anti_aliasing/screen_space_roughness_limiter/limit"));
	bool glow_bicubic = int(GLOBAL_GET("rendering/environment/glow/upscale_mode")) > 0;
	RS::get_singleton()->environment_set_ssil_quality(RSE::EnvironmentSSILQuality(int(GLOBAL_GET("rendering/environment/ssil/quality"))), GLOBAL_GET("rendering/environment/ssil/half_size"), GLOBAL_GET("rendering/environment/ssil/adaptive_target"), GLOBAL_GET("rendering/environment/ssil/blur_passes"), GLOBAL_GET("rendering/environment/ssil/fadeout_from"), GLOBAL_GET("rendering/environment/ssil/fadeout_to"));
	RS::get_singleton()->environment_glow_set_use_bicubic_upscale(glow_bicubic);
	RS::get_singleton()->environment_set_ssr_half_size(GLOBAL_GET("rendering/environment/screen_space_reflection/half_size"));
	RSE::SubSurfaceScatteringQuality sss_quality = RSE::SubSurfaceScatteringQuality(int(GLOBAL_GET("rendering/environment/subsurface_scattering/subsurface_scattering_quality")));
	RS::get_singleton()->sub_surface_scattering_set_quality(sss_quality);
	float sss_scale = GLOBAL_GET("rendering/environment/subsurface_scattering/subsurface_scattering_scale");
	float sss_depth_scale = GLOBAL_GET("rendering/environment/subsurface_scattering/subsurface_scattering_depth_scale");
	RS::get_singleton()->sub_surface_scattering_set_scale(sss_scale, sss_depth_scale);

	uint32_t directional_shadow_size = GLOBAL_GET("rendering/lights_and_shadows/directional_shadow/size");
	uint32_t directional_shadow_16_bits = GLOBAL_GET("rendering/lights_and_shadows/directional_shadow/16_bits");
	RS::get_singleton()->directional_shadow_atlas_set_size(directional_shadow_size, directional_shadow_16_bits);

	RSE::ShadowQuality shadows_quality = RSE::ShadowQuality(int(GLOBAL_GET("rendering/lights_and_shadows/positional_shadow/soft_shadow_filter_quality")));
	RS::get_singleton()->positional_soft_shadow_filter_set_quality(shadows_quality);
	RSE::ShadowQuality directional_shadow_quality = RSE::ShadowQuality(int(GLOBAL_GET("rendering/lights_and_shadows/directional_shadow/soft_shadow_filter_quality")));
	RS::get_singleton()->directional_soft_shadow_filter_set_quality(directional_shadow_quality);
	float probe_update_speed = GLOBAL_GET("rendering/lightmapping/probe_capture/update_speed");
	RS::get_singleton()->lightmap_set_probe_capture_update_speed(probe_update_speed);
	RSE::EnvironmentSDFGIFramesToConverge frames_to_converge = RSE::EnvironmentSDFGIFramesToConverge(int(GLOBAL_GET("rendering/global_illumination/sdfgi/frames_to_converge")));
	RS::get_singleton()->environment_set_sdfgi_frames_to_converge(frames_to_converge);
	RSE::EnvironmentSDFGIRayCount ray_count = RSE::EnvironmentSDFGIRayCount(int(GLOBAL_GET("rendering/global_illumination/sdfgi/probe_ray_count")));
	RS::get_singleton()->environment_set_sdfgi_ray_count(ray_count);
	RSE::VoxelGIQuality voxel_gi_quality = RSE::VoxelGIQuality(int(GLOBAL_GET("rendering/global_illumination/voxel_gi/quality")));
	RS::get_singleton()->voxel_gi_set_quality(voxel_gi_quality);
	RS::get_singleton()->environment_set_volumetric_fog_volume_size(GLOBAL_GET("rendering/environment/volumetric_fog/volume_size"), GLOBAL_GET("rendering/environment/volumetric_fog/volume_depth"));
	RS::get_singleton()->environment_set_volumetric_fog_filter_active(bool(GLOBAL_GET("rendering/environment/volumetric_fog/use_filter")));
	RS::get_singleton()->canvas_set_shadow_texture_size(GLOBAL_GET("rendering/2d/shadow_atlas/size"));

	bool use_half_res_gi = GLOBAL_GET("rendering/global_illumination/gi/use_half_resolution");
	RS::get_singleton()->gi_set_use_half_resolution(use_half_res_gi);

	bool snap_2d_transforms = GLOBAL_GET("rendering/2d/snap/snap_2d_transforms_to_pixel");
	_edited_scene_viewport->set_snap_2d_transforms_to_pixel(snap_2d_transforms);
	bool snap_2d_vertices = GLOBAL_GET("rendering/2d/snap/snap_2d_vertices_to_pixel");
	_edited_scene_viewport->set_snap_2d_vertices_to_pixel(snap_2d_vertices);

	Viewport::SDFOversize sdf_oversize = Viewport::SDFOversize(int(GLOBAL_GET("rendering/2d/sdf/oversize")));
	_edited_scene_viewport->set_sdf_oversize(sdf_oversize);
	Viewport::SDFScale sdf_scale = Viewport::SDFScale(int(GLOBAL_GET("rendering/2d/sdf/scale")));
	_edited_scene_viewport->set_sdf_scale(sdf_scale);

	Viewport::MSAA msaa = Viewport::MSAA(int(GLOBAL_GET("rendering/anti_aliasing/quality/msaa_2d")));
	_edited_scene_viewport->set_msaa_2d(msaa);

	// 2D doesn't use a dedicated SubViewport like 3D does, so we apply it on the root viewport instead.
	bool use_debanding = GLOBAL_GET("rendering/anti_aliasing/quality/use_debanding");
	_edited_scene_viewport->set_use_debanding(use_debanding);
	get_viewport()->set_use_debanding(use_debanding);

	// Enable HDR if requested.
	const bool hdr_requested = GLOBAL_GET("display/window/hdr/request_hdr_output");
	DisplayServer::get_singleton()->window_request_hdr_output(hdr_requested);

	const bool use_hdr_2d = GLOBAL_GET("rendering/viewport/hdr_2d");
	_edited_scene_viewport->set_use_hdr_2d(use_hdr_2d || hdr_requested);
	get_viewport()->set_use_hdr_2d(use_hdr_2d || hdr_requested);

	if (hdr_requested && !use_hdr_2d) {
		WARN_PRINT_ED("HDR 2D was automatically enabled because HDR output was requested in project settings. To avoid this warning, enable rendering/viewport/hdr_2d in the Project Settings.");
	}

	float mesh_lod_threshold = GLOBAL_GET("rendering/mesh_lod/lod_change/threshold_pixels");
	_edited_scene_viewport->set_mesh_lod_threshold(mesh_lod_threshold);

	RS::get_singleton()->decals_set_filter(RSE::DecalFilter(int(GLOBAL_GET("rendering/textures/decals/filter"))));
	RS::get_singleton()->light_projectors_set_filter(RSE::LightProjectorFilter(int(GLOBAL_GET("rendering/textures/light_projectors/filter"))));
	RS::get_singleton()->lightmaps_set_bicubic_filter(GLOBAL_GET("rendering/lightmapping/lightmap_gi/use_bicubic_filter"));
	RS::get_singleton()->material_set_use_debanding(GLOBAL_GET("rendering/anti_aliasing/quality/use_debanding"));

	SceneTree *tree = get_tree();
	tree->set_debug_collisions_color(GLOBAL_GET("debug/shapes/collision/shape_color"));
	tree->set_debug_collision_contact_color(GLOBAL_GET("debug/shapes/collision/contact_color"));

	if (ProjectSettings::get_singleton()->check_changed_settings_in_group("display/window/hdr/request_hdr_output") || ProjectSettings::get_singleton()->check_changed_settings_in_group("rendering/viewport/hdr_2d")) {
		const bool use_hdr = GLOBAL_GET("display/window/hdr/request_hdr_output").operator bool() || GLOBAL_GET("rendering/viewport/hdr_2d").operator bool();

		LocalVector<ObjectID> invalid_viewports;
		for (const ObjectID &id : _hdr_viewports) {
			Viewport *vp = ObjectDB::get_instance<Viewport>(id);
			if (vp) {
				vp->set_use_hdr_2d(use_hdr);
			} else {
				invalid_viewports.push_back(id);
			}
		}

		for (const ObjectID &id : invalid_viewports) {
			_hdr_viewports.erase(id);
		}
	}

	ResourceImporterTexture::get_singleton()->update_imports();
	ResourceImporterStreamedTexture::get_singleton()->update_imports();

	_update_translations();

#ifdef DEBUG_ENABLED
	NavigationServer2D::get_singleton()->set_debug_navigation_edge_connection_color(GLOBAL_GET("debug/shapes/navigation/2d/edge_connection_color"));
	NavigationServer2D::get_singleton()->set_debug_navigation_geometry_edge_color(GLOBAL_GET("debug/shapes/navigation/2d/geometry_edge_color"));
	NavigationServer2D::get_singleton()->set_debug_navigation_geometry_face_color(GLOBAL_GET("debug/shapes/navigation/2d/geometry_face_color"));
	NavigationServer2D::get_singleton()->set_debug_navigation_geometry_edge_disabled_color(GLOBAL_GET("debug/shapes/navigation/2d/geometry_edge_disabled_color"));
	NavigationServer2D::get_singleton()->set_debug_navigation_geometry_face_disabled_color(GLOBAL_GET("debug/shapes/navigation/2d/geometry_face_disabled_color"));
	NavigationServer2D::get_singleton()->set_debug_navigation_enable_edge_connections(GLOBAL_GET("debug/shapes/navigation/2d/enable_edge_connections"));
	NavigationServer2D::get_singleton()->set_debug_navigation_enable_edge_lines(GLOBAL_GET("debug/shapes/navigation/2d/enable_edge_lines"));
	NavigationServer2D::get_singleton()->set_debug_navigation_enable_geometry_face_random_color(GLOBAL_GET("debug/shapes/navigation/2d/enable_geometry_face_random_color"));

	NavigationServer3D::get_singleton()->set_debug_navigation_edge_connection_color(GLOBAL_GET("debug/shapes/navigation/3d/edge_connection_color"));
	NavigationServer3D::get_singleton()->set_debug_navigation_geometry_edge_color(GLOBAL_GET("debug/shapes/navigation/3d/geometry_edge_color"));
	NavigationServer3D::get_singleton()->set_debug_navigation_geometry_face_color(GLOBAL_GET("debug/shapes/navigation/3d/geometry_face_color"));
	NavigationServer3D::get_singleton()->set_debug_navigation_geometry_edge_disabled_color(GLOBAL_GET("debug/shapes/navigation/3d/geometry_edge_disabled_color"));
	NavigationServer3D::get_singleton()->set_debug_navigation_geometry_face_disabled_color(GLOBAL_GET("debug/shapes/navigation/3d/geometry_face_disabled_color"));
	NavigationServer3D::get_singleton()->set_debug_navigation_enable_edge_connections(GLOBAL_GET("debug/shapes/navigation/3d/enable_edge_connections"));
	NavigationServer3D::get_singleton()->set_debug_navigation_enable_edge_connections_xray(GLOBAL_GET("debug/shapes/navigation/3d/enable_edge_connections_xray"));
	NavigationServer3D::get_singleton()->set_debug_navigation_enable_edge_lines(GLOBAL_GET("debug/shapes/navigation/3d/enable_edge_lines"));
	NavigationServer3D::get_singleton()->set_debug_navigation_enable_edge_lines_xray(GLOBAL_GET("debug/shapes/navigation/3d/enable_edge_lines_xray"));
	NavigationServer3D::get_singleton()->set_debug_navigation_enable_geometry_face_random_color(GLOBAL_GET("debug/shapes/navigation/3d/enable_geometry_face_random_color"));
#endif // DEBUG_ENABLED
}

void EditorNode::_update_translations() {
	Ref<TranslationDomain> main = TranslationServer::get_singleton()->get_main_domain();

	TranslationServer::get_singleton()->load_project_translations(main);

	if (main->is_enabled()) {
		// Check for the exact locale.
		if (main->has_translation_for_locale(main->get_locale_override(), true)) {
			// The set of translation resources for the current locale changed.
			const HashSet<Ref<Translation>> translations = main->find_translations(main->get_locale_override(), false);
			if (translations != _tracked_translations) {
				_translation_resources_changed();
			}
		} else {
			// Translations for the current preview locale is removed.
			main->set_enabled(false);
			main->set_locale_override(String());
			_translation_resources_changed();
		}
	}
}

void EditorNode::_translation_resources_changed() {
	for (const Ref<Translation> &translation : _tracked_translations) {
		translation->disconnect_changed(callable_mp(this, &EditorNode::_queue_translation_notification));
	}
	_tracked_translations.clear();

	const Ref<TranslationDomain> main = TranslationServer::get_singleton()->get_main_domain();
	if (main->is_enabled()) {
		const HashSet<Ref<Translation>> translations = main->find_translations(main->get_locale_override(), false);
		_tracked_translations.reserve(translations.size());
		for (const Ref<Translation> &translation : translations) {
			translation->connect_changed(callable_mp(this, &EditorNode::_queue_translation_notification));
			_tracked_translations.insert(translation);
		}
	}

	_queue_translation_notification();
	emit_signal(SNAME("preview_locale_changed"));
}

void EditorNode::_queue_translation_notification() {
	if (_pending_translation_notification) {
		return;
	}
	_pending_translation_notification = true;
	callable_mp(this, &EditorNode::_propagate_translation_notification).call_deferred();
}

void EditorNode::_propagate_translation_notification() {
	_pending_translation_notification = false;
	_edited_scene_viewport->propagate_notification(NOTIFICATION_TRANSLATION_CHANGED);
}

void EditorNode::_queue_resource_counter_changed() {
	if (_resource_counter_queued) {
		return;
	}
	callable_mp(this, &EditorNode::_emit_resource_counter_changed).call_deferred();
	_resource_counter_queued = true;
}

void EditorNode::_emit_resource_counter_changed() {
	_resource_counter_queued = false;
	emit_signal(SNAME("resource_counter_changed"));
}

void EditorNode::_exit_editor(int p_exit_code) {
	_exiting = true;
	_waiting_for_first_scan = false;
	_resource_preview->stop(); // Stop early to avoid crashes.
	_save_editor_layout();

	// Dim the editor window while it's quitting to make it clearer that it's busy.
	dim_editor(true);

	// Unload addons before quitting to allow cleanup.
	EditorManagers::get_singleton()->get_project_plugin_manager()->unload_plugins();

	if (!_files_to_delete_on_exit.is_empty()) {
		Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
		for (const String &path : _files_to_delete_on_exit) {
			if (da->file_exists(path) || da->dir_exists(path)) {
				OS::get_singleton()->move_to_trash(ProjectSettings::get_singleton()->globalize_path(path));
			}
		}
	}
	get_tree()->quit(p_exit_code);
}

void EditorNode::_remove_edited_scene(bool p_change_tab) {
	// When scene gets closed no node is edited anymore, so make sure the editors are notified before nodes are freed.
	hide_unused_editors(HierarchyDock::get_singleton());
	_editor_selection->clear();
	_editor_selection->update(false);

	int new_index = _editor_data.get_edited_scene();
	int old_index = new_index;

	if (new_index > 0) {
		new_index = new_index - 1;
	} else if (_editor_data.get_edited_scene_count() > 1) {
		new_index = 1;
	} else {
		_editor_data.add_edited_scene(-1);
		new_index = 1;
	}

	if (p_change_tab) {
		_set_current_scene(new_index);
	}
	_editor_data.remove_scene(old_index);
	_update_title();
	_scene_tabs->update_scene_tabs();
}

void EditorNode::_remove_scene(int p_idx, bool p_change_tab) {
	// Clear icon cache in case some scripts are no longer needed or class icons are outdated.
	// FIXME: Ideally the cache should never be cleared and only updated on per-script basis, when an icon changes.
	_editor_data.clear_script_icon_cache();
	_class_icon_cache.clear();

	_save_editor_states(_editor_data.get_scene_path(p_idx), p_idx);
	if (_editor_data.get_edited_scene() == p_idx) {
		// Scene to remove is current scene.
		_remove_edited_scene(p_change_tab);
	} else {
		// Scene to remove is not active scene.
		_editor_data.remove_scene(p_idx);
	}
}

bool EditorNode::_find_and_save_resource(Ref<Resource> r_res, HashMap<Ref<Resource>, bool> &r_processed, int32_t p_flags) {
	if (r_res.is_null()) {
		return false;
	}

	if (r_processed.has(r_res)) {
		return r_processed[r_res];
	}
	if (FileAccessContent::is_content_path(r_res->get_path())) {
		r_res->set_edited(false);
		r_processed[r_res] = false;
		return false;
	}

	bool changed = r_res->is_edited();
	r_res->set_edited(false);

	bool subchanged = _find_and_save_edited_subresources(r_res.ptr(), r_processed, p_flags);

	if (r_res->get_path().is_resource_file()) {
		if (changed || subchanged) {
			ResourceSaver::save(r_res, r_res->get_path(), p_flags);
		}
		r_processed[r_res] = false; // Because it's a file.
		return false;
	} else {
		r_processed[r_res] = changed;
		return changed;
	}
}

bool EditorNode::_find_and_save_edited_subresources(Object *p_object, HashMap<Ref<Resource>, bool> &r_processed, int32_t p_flags) {
	bool ret_changed = false;
	List<PropertyInfo> pi;
	p_object->get_property_list(&pi);
	for (const PropertyInfo &property_info : pi) {
		if (!(property_info.usage & PROPERTY_USAGE_STORAGE)) {
			continue;
		}

		switch (property_info.type) {
			case Variant::OBJECT: {
				Ref<Resource> res = p_object->get(property_info.name);

				if (_find_and_save_resource(res, r_processed, p_flags)) {
					ret_changed = true;
				}

			} break;
			case Variant::ARRAY: {
				Array varray = p_object->get(property_info.name);
				int len = varray.size();
				for (int i = 0; i < len; i++) {
					const Variant &v = varray.get(i);
					Ref<Resource> res = v;
					if (_find_and_save_resource(res, r_processed, p_flags)) {
						ret_changed = true;
					}
				}

			} break;
			case Variant::DICTIONARY: {
				Dictionary d = p_object->get(property_info.name);
				for (const KeyValue<Variant, Variant> &kv : d) {
					Ref<Resource> res = kv.value;
					if (_find_and_save_resource(res, r_processed, p_flags)) {
						ret_changed = true;
					}
				}
			} break;
			default: {
			}
		}
	}

	return ret_changed;
}

void EditorNode::_save_edited_subresources(Node *p_scene, HashMap<Ref<Resource>, bool> &r_processed, int32_t p_flags) {
	_find_and_save_edited_subresources(p_scene, r_processed, p_flags);

	for (int i = 0; i < p_scene->get_child_count(); i++) {
		Node *n = p_scene->get_child(i);
		if (n->get_owner() != _editor_data.get_edited_scene_root()) {
			continue;
		}
		_save_edited_subresources(n, r_processed, p_flags);
	}
}

void EditorNode::_mark_unsaved_scenes() {
	for (int i = 0; i < _editor_data.get_edited_scene_count(); i++) {
		Node *node = _editor_data.get_edited_scene_root(i);
		if (!node) {
			continue;
		}

		String path = node->get_scene_file_path();
		if (!path.is_empty() && !FileAccess::exists(path)) {
			// Mark scene tab as unsaved if the file is gone.
			EditorUndoRedoManager::get_singleton()->set_history_as_unsaved(_editor_data.get_scene_history_id(i));
		}
	}

	_update_title();
	_scene_tabs->update_scene_tabs();
}

void EditorNode::_find_node_types(Node *p_node, int &r_count_2d, int &r_count_3d) {
	if (p_node->is_class("Viewport") || (p_node != _editor_data.get_edited_scene_root() && p_node->get_owner() != _editor_data.get_edited_scene_root())) {
		return;
	}

	if (p_node->is_class("CanvasItem")) {
		r_count_2d++;
	} else if (p_node->is_class("Node3D")) {
		r_count_3d++;
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		_find_node_types(p_node->get_child(i), r_count_2d, r_count_3d);
	}
}

void EditorNode::_save_scene_with_preview(String p_file, int p_idx) {
	_save_scene_progress = memnew(EditorProgress("save", TTR("Saving Scene"), 4));

	if (_editor_data.get_edited_scene_root() != nullptr) {
		_save_scene_progress->step(TTR("Analyzing"), 0);

		int c2d = 0;
		int c3d = 0;

		_find_node_types(_editor_data.get_edited_scene_root(), c2d, c3d);

		_save_scene_progress->step(TTR("Creating Thumbnail"), 1);
		// Current view?

		Ref<Image> img;
		// If neither 3D or 2D nodes are present, make a 1x1 black texture.
		// We cannot fallback on the 2D editor, because it may not have been used yet,
		// which would result in an invalid texture.
		if (c3d == 0 && c2d == 0) {
			img.instantiate();
			img->initialize_data(1, 1, false, Image::FORMAT_RGB8);
		} else if (c3d < c2d) {
			Ref<ViewportTexture> viewport_texture = _edited_scene_viewport->get_texture();
			if (viewport_texture->get_width() > 0 && viewport_texture->get_height() > 0) {
				img = viewport_texture->get_image();
			}
		} else {
			// The 3D editor may be disabled as a feature, but scenes can still be opened.
			// This check prevents the preview from regenerating in case those scenes are then saved.
			// The preview will be generated if no feature profile is set (as the 3D editor is enabled by default).
			Ref<EditorFeatureProfile> profile = _feature_profile_manager->get_current_profile();
			if (profile.is_null() || !profile->is_feature_disabled(EditorFeatureProfile::FEATURE_3D)) {
				img = Node3DEditor::get_singleton()->get_editor_viewport(0)->get_viewport_node()->get_texture()->get_image();
			}
		}

		if (img.is_valid() && img->get_width() > 0 && img->get_height() > 0) {
			img = img->duplicate();

			_save_scene_progress->step(TTR("Creating Thumbnail"), 3);

			int preview_size = EDITOR_GET("filesystem/file_dialog/thumbnail_size");
			preview_size *= EDSCALE;

			// Consider a square region.
			int vp_size = MIN(img->get_width(), img->get_height());
			int x = (img->get_width() - vp_size) / 2;
			int y = (img->get_height() - vp_size) / 2;

			if (vp_size < preview_size) {
				// Just square it.
				img->crop_from_point(x, y, vp_size, vp_size);
			} else {
				int ratio = vp_size / preview_size;
				int size = preview_size * MAX(1, ratio / 2);

				x = (img->get_width() - size) / 2;
				y = (img->get_height() - size) / 2;

				img->crop_from_point(x, y, size, size);
				img->resize(preview_size, preview_size, Image::INTERPOLATE_LANCZOS);
			}
			img->convert(Image::FORMAT_RGB8);

			// Save thumbnail directly, as thumbnailer may not update due to actual scene not changing md5.
			String temp_path = EditorPaths::get_singleton()->get_cache_dir();
			String cache_base = ProjectSettings::get_singleton()->globalize_path(p_file).md5_text();
			cache_base = temp_path.path_join("resthumb-" + cache_base);

			// Does not have it, try to load a cached thumbnail.
			post_process_preview(img);
			img->save_png(cache_base + ".png");
		}
	}

	_save_scene_progress->step(TTR("Saving Scene"), 4);
	_save_scene(p_file, p_idx);

	if (!_singleton->_cmdline_mode) {
		EditorResourcePreview::get_singleton()->check_for_invalidation(p_file);
	}

	_close_save_scene_progress();
}

void EditorNode::_close_save_scene_progress() {
	memdelete(_save_scene_progress);
	_save_scene_progress = nullptr;
}

void EditorNode::_proceed_closing_scene_tabs() {
	List<String>::Element *next_tab_to_close = _tabs_to_close.front();
	if (!next_tab_to_close) {
		if (_is_closing_editor()) {
			_current_menu_option = _tab_closing_menu_option;
			_menu_option_confirm(_tab_closing_menu_option, true);
		} else {
			_current_menu_option = -1;
			_save_confirmation_dialog->hide();
		}
		return;
	}
	String scene_to_close = next_tab_to_close->get();
	_tabs_to_close.pop_front();

	int tab_idx = -1;
	for (int i = 0; i < _editor_data.get_edited_scene_count(); i++) {
		if (_editor_data.get_scene_path(i) == scene_to_close) {
			tab_idx = i;
			break;
		}
	}
	ERR_FAIL_COND(tab_idx < 0);

	_scene_tab_closed(tab_idx);
}

void EditorNode::_save_next_scene_tab_as() {
	if (_scenes_to_save_as.is_empty()) {
		return;
	}
	int scene_idx = _scenes_to_save_as.front()->get();
	_scenes_to_save_as.pop_front();
	_set_current_scene(scene_idx);
	_menu_option_confirm(SCENE_MULTI_SAVE_AS_SCENE, false);
}

bool EditorNode::_is_closing_editor() const {
	return _tab_closing_menu_option == SCENE_QUIT || _tab_closing_menu_option == PROJECT_QUIT_TO_HUB || _tab_closing_menu_option == PROJECT_RELOAD_CURRENT_PROJECT;
}

void EditorNode::_restart_editor(bool p_goto_hub) {
	_exiting = true;

	if (_project_run_bar->is_playing()) {
		_project_run_bar->stop_playing();
	}

	String to_reopen;
	if (!p_goto_hub && get_tree()->get_edited_scene_root()) {
		to_reopen = get_tree()->get_edited_scene_root()->get_scene_file_path();
	}

	_exit_editor(EXIT_SUCCESS);

	List<String> args;
	for (const String &a : Main::get_forwardable_cli_arguments(Main::CLI_SCOPE_TOOL)) {
		args.push_back(a);
	}

	if (p_goto_hub) {
		args.push_back("--hub");

		// Setup working directory.
		const String exec_dir = OS::get_singleton()->get_executable_path().get_base_dir();
		if (!exec_dir.is_empty()) {
			args.push_back("--path");
			args.push_back(exec_dir);
		}

		List<String>::Element *vbf = args.find("--verbose");
		if (vbf) {
			args.erase(vbf);
		}
	} else {
		args.push_back("--path");
		args.push_back(ProjectSettings::get_singleton()->get_resource_path());

		args.push_back("-e");
	}
	if (!_files_to_delete_on_exit.is_empty()) {
		args.push_back("--clear-shader-cache");
	}

	if (!to_reopen.is_empty()) {
		args.push_back(to_reopen);
	}

	OS::get_singleton()->set_restart_on_exit(true, args);
}

Dictionary EditorNode::_get_main_scene_state() {
	Dictionary state;
	state["hierarchy_offset"] = HierarchyDock::get_singleton()->get_hierarchy_editor()->get_tree_control()->get_vscroll_bar()->get_value();
	state["property_edit_offset"] = InspectorDock::get_inspector_singleton()->get_scroll_offset();
	state["node_filter"] = HierarchyDock::get_singleton()->get_filter();
	return state;
}

void EditorNode::_set_main_scene_state(const Dictionary &p_state) {
	if (p_state.has("hierarchy_offset")) {
		HierarchyDock::get_singleton()->get_hierarchy_editor()->get_tree_control()->get_vscroll_bar()->set_value(p_state["hierarchy_offset"]);
	}
	if (p_state.has("property_edit_offset")) {
		InspectorDock::get_inspector_singleton()->set_scroll_offset(p_state["property_edit_offset"]);
	}

	if (p_state.has("node_filter")) {
		HierarchyDock::get_singleton()->set_filter(p_state["node_filter"]);
	}

	// This should only happen at the very end.

	EditorDebuggerNode::get_singleton()->update_live_edit_root();
	ScriptEditorDock::get_singleton()->set_scene_root_script(_editor_data.get_scene_root_script(_editor_data.get_edited_scene()));
	_editor_data.notify_edited_scene_changed();
	emit_signal(SNAME("scene_changed"));

	// Reset SDFGI after everything else so that any last-second scene modifications will be processed.
	RenderingServer::get_singleton()->sdfgi_reset();
}

Ref<ConfigFile> EditorNode::_load_scene_config(const String &p_scene_path) {
	const String config_file_path = EditorPaths::get_singleton()->get_project_settings_dir().path_join(p_scene_path.get_file() + "-editstate-" + p_scene_path.md5_text() + ".cfg");

	Ref<ConfigFile> editor_state_cf;
	editor_state_cf.instantiate();
	editor_state_cf->load(config_file_path);
	return editor_state_cf;
}

void EditorNode::_save_editor_layout() {
	if (!_load_editor_layout_done) {
		return;
	}
	Ref<ConfigFile> config;
	config.instantiate();
	// Load and amend existing config if it exists.
	config->load(EditorPaths::get_singleton()->get_project_settings_dir().path_join("editor_layout.cfg"));

	_editor_dock_manager->save_docks_to_config(config, "docks");
	_save_open_scenes_to_config(config);
	_save_central_editor_layout_to_config(config);
	_save_window_settings_to_config(config, "EditorWindow");
	_editor_data.get_editor_window_layout(config);

	config->save(EditorPaths::get_singleton()->get_project_settings_dir().path_join("editor_layout.cfg"));
}

void EditorNode::_load_editor_layout() {
	EditorProgress ep("loading_editor_layout", TTR("Loading editor"), 5);
	ep.step(TTR("Loading editor layout..."), 0, true);
	Ref<ConfigFile> config;
	config.instantiate();
	Error err = config->load(EditorPaths::get_singleton()->get_project_settings_dir().path_join("editor_layout.cfg"));
	if (err != OK) { // No config.
		// If config is not found, expand the res:// folder and favorites by default.
		TreeItem *root = FileSystemDock::get_singleton()->get_tree_control()->get_item_with_metadata("res://", 0);
		if (root) {
			root->set_collapsed(false);
		}

		TreeItem *favorites = FileSystemDock::get_singleton()->get_tree_control()->get_item_with_metadata("Favorites", 0);
		if (favorites) {
			favorites->set_collapsed(false);
		}

		if (_overridden_default_layout) {
			_layout_menu_option(LAYOUT_DEFAULT);
		} else {
			ep.step(TTR("Loading docks..."), 1, true);
			_editor_dock_manager->load_docks_from_config(_default_layout, "docks", true);
		}
	} else {
		ep.step(TTR("Loading docks..."), 1, true);
		_editor_dock_manager->load_docks_from_config(config, "docks", true);

		ep.step(TTR("Reopening scenes..."), 2, true);
		_load_open_scenes_from_config(config);

		ep.step(TTR("Loading central editor layout..."), 3, true);
		_load_central_editor_layout_from_config(config);

		ep.step(TTR("Loading editor window layout..."), 4, true);
		_editor_data.set_editor_window_layout(config);

		ep.step(TTR("Editor layout ready."), 5, true);
	}
	_load_editor_layout_done = true;
}

void EditorNode::_save_central_editor_layout_to_config(Ref<ConfigFile> r_config_file) {
	// Debugger tab.

	int selected_default_debugger_tab_idx = EditorDebuggerNode::get_singleton()->get_default_debugger()->get_current_debugger_tab();
	r_config_file->set_value(EDITOR_NODE_CONFIG_SECTION, "selected_default_debugger_tab_idx", selected_default_debugger_tab_idx);
}

void EditorNode::_load_central_editor_layout_from_config(Ref<ConfigFile> p_config_file) {
	// Debugger tab.

	if (p_config_file->has_section_key(EDITOR_NODE_CONFIG_SECTION, "selected_default_debugger_tab_idx")) {
		int selected_default_debugger_tab_idx = p_config_file->get_value(EDITOR_NODE_CONFIG_SECTION, "selected_default_debugger_tab_idx");
		EditorDebuggerNode::get_singleton()->get_default_debugger()->switch_to_debugger(selected_default_debugger_tab_idx);
	}
}

void EditorNode::_save_window_settings_to_config(Ref<ConfigFile> r_layout, const String &p_section) {
	Window *w = get_window();
	if (w) {
		r_layout->set_value(p_section, "screen", w->get_current_screen());

		Size2i win_size = w->get_size();

		if (DisplayServer::get_singleton()->has_feature(DisplayServerEnums::FEATURE_SELF_FITTING_WINDOWS)) {
			// Work around logical size issues with HiDPI on Wayland. See GH-110643.
			float win_scale = DisplayServer::get_singleton()->window_get_scale(w->get_window_id());

			win_size.width /= win_scale;
			win_size.height /= win_scale;
		}

		Window::Mode mode = w->get_mode();
		switch (mode) {
			case Window::MODE_WINDOWED:
				r_layout->set_value(p_section, "mode", "windowed");
				r_layout->set_value(p_section, "size", win_size);
				break;
			case Window::MODE_FULLSCREEN:
			case Window::MODE_EXCLUSIVE_FULLSCREEN:
				r_layout->set_value(p_section, "mode", "fullscreen");
				break;
			case Window::MODE_MINIMIZED:
				if (_was_window_windowed_last) {
					r_layout->set_value(p_section, "mode", "windowed");
					r_layout->set_value(p_section, "size", w->get_size());
				} else {
					r_layout->set_value(p_section, "mode", "maximized");
				}
				break;
			default:
				r_layout->set_value(p_section, "mode", "maximized");
				break;
		}

		r_layout->set_value(p_section, "position", w->get_position());
	}
}

void EditorNode::_save_open_scenes_to_config(Ref<ConfigFile> r_layout) {
	PackedStringArray scenes;
	for (int i = 0; i < _editor_data.get_edited_scene_count(); i++) {
		String path = _editor_data.get_scene_path(i);
		if (path.is_empty()) {
			continue;
		}
		scenes.push_back(path);
	}
	r_layout->set_value(EDITOR_NODE_CONFIG_SECTION, "open_scenes", scenes);

	String currently_edited_scene_path = _editor_data.get_scene_path(_editor_data.get_edited_scene());
	r_layout->set_value(EDITOR_NODE_CONFIG_SECTION, "current_scene", currently_edited_scene_path);
}

void EditorNode::_load_open_scenes_from_config(Ref<ConfigFile> p_layout) {
	if (Engine::get_singleton()->is_recovery_mode_hint()) {
		return;
	}

	if (!bool(EDITOR_GET("interface/scene_tabs/restore_scenes_on_load"))) {
		return;
	}

	if (!p_layout->has_section_key(EDITOR_NODE_CONFIG_SECTION, "open_scenes")) {
		return;
	}

	_restoring_scenes = true;

	const PackedStringArray scenes = p_layout->get_value(EDITOR_NODE_CONFIG_SECTION, "open_scenes");
	for (const String &scene_path : scenes) {
		if (FileAccess::exists(scene_path)) {
			load_scene(scene_path);
		}
	}

	bool current_scene_found = false;
	const String current_scene = p_layout->get_value(EDITOR_NODE_CONFIG_SECTION, "current_scene", String());
	for (int i = 0; i < _editor_data.get_edited_scene_count(); i++) {
		if (_editor_data.get_scene_path(i) == current_scene) {
			_apply_edited_scene(i, true);
			current_scene_found = true;
			break;
		}
	}
	if (!current_scene_found && _editor_data.get_edited_scene_count() > 0) {
		_apply_edited_scene(0);
	}

	save_editor_layout_delayed();

	_restoring_scenes = false;
}

void EditorNode::_update_layouts_menu() {
	_editor_layouts_menu->clear();
	_overridden_default_layout = false;

	_editor_layouts_menu->reset_size();
	_editor_layouts_menu->add_shortcut(ED_SHORTCUT("layout/save", TTRC("Save Layout...")), LAYOUT_SAVE);
	_editor_layouts_menu->add_shortcut(ED_SHORTCUT("layout/delete", TTRC("Delete Layout...")), LAYOUT_DELETE);
	_editor_layouts_menu->add_separator();

	Ref<ConfigFile> config;
	config.instantiate();
	Error err = config->load(EditorSettings::get_singleton()->get_editor_layouts_config());
	if (err == OK && config->has_section("Default")) {
		_overridden_default_layout = true;
	}

	_editor_layouts_menu->add_shortcut(ED_SHORTCUT("layout/default", _overridden_default_layout ? TTRC("Default (Overridden)") : TTRC("Default")), LAYOUT_DEFAULT);

	if (err != OK) {
		return; // No config.
	}

	Vector<String> layouts = config->get_sections();
	for (const String &layout : layouts) {
		if (layout != "Default" && !layout.contains_char('/')) {
			_editor_layouts_menu->add_item(layout);
			_editor_layouts_menu->set_item_auto_translate_mode(-1, AUTO_TRANSLATE_MODE_DISABLED);
		}
	}
}

void EditorNode::_layout_menu_option(int p_id) {
	switch (p_id) {
		case LAYOUT_SAVE: {
			_current_menu_option = p_id;
			_editor_layout_dialog->set_save_mode_enabled(true);
			_editor_layout_dialog->popup_centered();
		} break;

		case LAYOUT_DELETE: {
			_current_menu_option = p_id;
			_editor_layout_dialog->set_save_mode_enabled(false);
			_editor_layout_dialog->popup_centered();
		} break;

		case LAYOUT_DEFAULT: {
			// Check if the default layout was overridden, and if so, select that instead.
			Ref<ConfigFile> config;
			config.instantiate();
			Error err = config->load(EditorSettings::get_singleton()->get_editor_layouts_config());
			if (err == OK && config->has_section("Default")) {
				_editor_dock_manager->load_docks_from_config(config, "Default");
				_save_editor_layout();

				return;
			}

			_editor_dock_manager->load_docks_from_config(_default_layout, "docks");
			_save_editor_layout();
		} break;

		default: {
			Ref<ConfigFile> config;
			config.instantiate();
			Error err = config->load(EditorSettings::get_singleton()->get_editor_layouts_config());
			if (err == OK) {
				_editor_dock_manager->load_docks_from_config(config, _editor_layouts_menu->get_item_text(p_id));
				_save_editor_layout();
			}
		}
	}
}

void EditorNode::_inherit_imported(const String &p_action) {
	_open_imported_dialog->hide();
	open_scene(_open_import_request, true, true);
}

void EditorNode::_open_imported() {
	open_scene(_open_import_request, true, false, true);
}

void EditorNode::_resources_changed(const Vector<String> &p_resources) {
	List<Ref<Resource>> changed;

	int rc = p_resources.size();
	for (int i = 0; i < rc; i++) {
		Ref<Resource> res = ResourceCache::get_ref(p_resources.get(i));
		if (res.is_null()) {
			continue;
		}

		if (!res->editor_can_reload_from_file()) {
			continue;
		}
		if (!res->get_path().is_resource_file() && !res->get_path().is_absolute_path()) {
			continue;
		}
		if (!FileAccess::exists(res->get_path())) {
			continue;
		}

		if (!res->get_import_path().is_empty()) {
			// This is an imported resource, will be reloaded if reimported via the _resources_reimported() callback.
			continue;
		}

		changed.push_back(res);
	}

	if (changed.size()) {
		for (Ref<Resource> &res : changed) {
			res->reload_from_file();
		}
	}
}

void EditorNode::_scan_external_changes() {
	_external_changes_tree->clear();
	TreeItem *r = _external_changes_tree->create_item();
	_external_changes_tree->set_hide_root(true);
	bool need_reload = false;

	_externally_changed_scene_paths.clear();
	_project_settings_changed_on_disk = false;

	// Check if any edited scene has changed.
	for (int i = 0; i < _editor_data.get_edited_scene_count(); i++) {
		Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);

		const String scene_path = _editor_data.get_scene_path(i);

		if (scene_path == "" || !da->file_exists(scene_path)) {
			continue;
		}

		uint64_t last_date = _editor_data.get_scene_modified_time(i);
		uint64_t date = FileAccess::get_modified_time(scene_path);

		if (date > last_date) {
			TreeItem *ti = _external_changes_tree->create_item(r);
			ti->set_text(0, scene_path.get_file());
			need_reload = true;
			_externally_changed_scene_paths.insert(scene_path);
		}
	}

	String project_settings_path = ProjectSettings::get_singleton()->get_resource_path().path_join("project.godot");
	if (FileAccess::get_modified_time(project_settings_path) > ProjectSettings::get_singleton()->get_last_saved_time()) {
		TreeItem *ti = _external_changes_tree->create_item(r);
		ti->set_text(0, "project.godot");
		need_reload = true;
		_project_settings_changed_on_disk = true;
	}

	if (need_reload) {
		callable_mp((Window *)_external_changes_dialog, &Window::popup_centered_ratio).call_deferred(0.3);
	}
}

void EditorNode::_reload_modified_scenes() {
	int current_idx = _editor_data.get_edited_scene();

	for (int i = 0; i < _editor_data.get_edited_scene_count(); i++) {
		if (_editor_data.get_scene_path(i) == "") {
			continue;
		}

		uint64_t last_date = _editor_data.get_scene_modified_time(i);
		uint64_t date = FileAccess::get_modified_time(_editor_data.get_scene_path(i));

		if (date > last_date) {
			String filename = _editor_data.get_scene_path(i);
			_editor_data.set_edited_scene(i);
			_remove_edited_scene(false);

			Error err = open_scene(filename);
			if (err != OK) {
				ERR_PRINT(vformat("Failed to load scene: %s", filename));
			}
			_editor_data.move_edited_scene_to_index(i);
		}
	}

	_set_current_scene(current_idx);
	_scene_tabs->update_scene_tabs();
	_external_changes_dialog->hide();
}

void EditorNode::_reload_project_settings() {
	const Error error = ProjectSettings::get_singleton()->setup(ProjectSettings::get_singleton()->get_resource_path(), String(), true, true);
	ERR_FAIL_COND_MSG(error != OK, "Failed to reload project settings.");
	EditorManagers::get_singleton()->get_project_autoload_manager()->refresh_autoloads();
}

void EditorNode::_resave_externally_modified_scenes(String p_str) {
	save_scene_list(_externally_changed_scene_paths);

	if (_project_settings_changed_on_disk) {
		ProjectSettings::get_singleton()->save();
	}

	_external_changes_dialog->hide();
}

void EditorNode::_feature_profile_changed() {
	Ref<EditorFeatureProfile> profile = _feature_profile_manager->get_current_profile();
	if (profile.is_valid()) {
		_editor_dock_manager->set_dock_enabled(SignalsDock::get_singleton(), !profile->is_feature_disabled(EditorFeatureProfile::FEATURE_SIGNALS_DOCK));
		_editor_dock_manager->set_dock_enabled(GroupsDock::get_singleton(), !profile->is_feature_disabled(EditorFeatureProfile::FEATURE_GROUPS_DOCK));
		// The Import dock is useless without the FileSystem dock. Ensure the configuration is valid.
		bool fs_dock_disabled = profile->is_feature_disabled(EditorFeatureProfile::FEATURE_FILESYSTEM_DOCK);
		_editor_dock_manager->set_dock_enabled(FileSystemDock::get_singleton(), !fs_dock_disabled);
		_editor_dock_manager->set_dock_enabled(ImportDock::get_singleton(), !fs_dock_disabled && !profile->is_feature_disabled(EditorFeatureProfile::FEATURE_IMPORT_DOCK));
		_editor_dock_manager->set_dock_enabled(_history_dock, !profile->is_feature_disabled(EditorFeatureProfile::FEATURE_HISTORY_DOCK));

		_scene_editor_dock->set_3d_enabled(!profile->is_feature_disabled(EditorFeatureProfile::FEATURE_3D));
		_editor_dock_manager->set_dock_enabled(ScriptEditorDock::get_singleton(), !profile->is_feature_disabled(EditorFeatureProfile::FEATURE_SCRIPT));
		if (!Engine::get_singleton()->is_recovery_mode_hint()) {
			_editor_dock_manager->set_dock_enabled(GameView::get_dock(), !profile->is_feature_disabled(EditorFeatureProfile::FEATURE_GAME));
		}
	} else {
		_editor_dock_manager->set_dock_enabled(ImportDock::get_singleton(), true);
		_editor_dock_manager->set_dock_enabled(SignalsDock::get_singleton(), true);
		_editor_dock_manager->set_dock_enabled(GroupsDock::get_singleton(), true);
		_editor_dock_manager->set_dock_enabled(FileSystemDock::get_singleton(), true);
		_editor_dock_manager->set_dock_enabled(_history_dock, true);
		_scene_editor_dock->set_3d_enabled(true);
		_editor_dock_manager->set_dock_enabled(ScriptEditorDock::get_singleton(), true);
		if (!Engine::get_singleton()->is_recovery_mode_hint() && GameView::get_dock()) {
			_editor_dock_manager->set_dock_enabled(GameView::get_dock(), true);
		}
	}

	_editor_dock_manager->update_docks_menu();
}

bool EditorNode::_is_class_editor_disabled_by_feature_profile(const StringName &p_class) {
	Ref<EditorFeatureProfile> profile = EditorFeatureProfileManager::get_singleton()->get_current_profile();
	if (profile.is_null()) {
		return false;
	}

	StringName class_name = p_class;

	while (class_name != StringName()) {
		if (profile->is_class_disabled(class_name)) {
			return true;
		}
		if (profile->is_class_editor_disabled(class_name)) {
			return true;
		}
		class_name = ClassDB::get_parent_class(class_name);
	}

	return false;
}

Ref<Texture2D> EditorNode::_get_class_or_script_icon(const String &p_class, const String &p_script_path, const String &p_fallback, bool p_fallback_script_to_theme, bool p_skip_fallback_virtual) {
	ERR_FAIL_COND_V_MSG(p_class.is_empty(), nullptr, "Class name cannot be empty.");
	EditorData &ed = EditorNode::get_editor_data();

	// Check for a script icon first.
	if (!p_script_path.is_empty()) {
		Ref<Texture2D> script_icon = ed.get_script_icon(p_script_path);
		if (script_icon.is_valid()) {
			return script_icon;
		}

		if (p_fallback_script_to_theme) {
			// Look for the native base type in the editor theme. This is relevant for
			// scripts extending other scripts and for built-in classes.
			String base_type;
			if (ScriptServer::is_global_class(p_class)) {
				base_type = ScriptServer::get_global_class_native_base(p_class);
			} else {
				Ref<Script> scr = ResourceLoader::load(p_script_path, "Script");
				if (scr.is_valid()) {
					base_type = scr->get_instance_base_type();
				}
			}
			if (_theme.is_valid()) {
				bool instantiable = false;

				// If the class doesn't exist or isn't global, then it's not instantiable
				if (ClassDB::class_exists(p_class)) {
					instantiable = !ClassDB::is_virtual(p_class) && ClassDB::can_instantiate(p_class);
				} else if (ScriptServer::is_global_class(p_class)) {
					instantiable = !ScriptServer::is_global_class_abstract(p_class);
				}

				return _get_class_or_script_icon(base_type, "", "", false, p_skip_fallback_virtual || instantiable);
			}
		}
	}

	// Script was not valid or didn't yield any useful values, try the class name
	// directly.

	// Check if the class name is a custom type.
	// TODO: Should probably be deprecated in 4.x
	const EditorData::CustomType *ctype = ed.get_custom_type_by_name(p_class);
	if (ctype && ctype->icon.is_valid()) {
		return ctype->icon;
	}

	// Look up the class name or the fallback name in the editor theme.
	// This is only relevant for built-in classes.
	if (_theme.is_valid()) {
		if (_theme->has_icon(editor_get_type_icon_name(p_class), EditorStringName(Editor))) {
			return _theme->get_icon(editor_get_type_icon_name(p_class), EditorStringName(Editor));
		}

		if (!p_fallback.is_empty() && _theme->has_icon(p_fallback, EditorStringName(Editor))) {
			return _theme->get_icon(p_fallback, EditorStringName(Editor));
		}

		// If the fallback is empty or wasn't found, use the default fallback.
		if (ClassDB::class_exists(p_class)) {
			if (!p_skip_fallback_virtual) {
				bool instantiable = !ClassDB::is_virtual(p_class) && ClassDB::can_instantiate(p_class);
				if (!instantiable) {
					if (ClassDB::is_parent_class(p_class, SNAME("Node"))) {
						return _theme->get_icon("status_node_disabled", EditorStringName(Editor));
					} else {
						return _theme->get_icon("status_object_disabled", EditorStringName(Editor));
					}
				}
			}
			StringName parent = ClassDB::get_parent_class_nocheck(p_class);
			if (parent) {
				// Skip virtual class if `p_skip_fallback_virtual` is true or `p_class` is instantiable.
				return _get_class_or_script_icon(parent, "", "", false, true);
			}
		}
	}

	return nullptr;
}

void EditorNode::_pick_main_scene_custom_action(const String &p_custom_action_name) {
	if (p_custom_action_name == "select_current") {
		Node *scene = _editor_data.get_edited_scene_root();

		if (!scene) {
			show_warning(TTR("There is no defined scene to run."));
			return;
		}

		_pick_main_scene_dialog->hide();

		if (!FileAccess::exists(scene->get_scene_file_path())) {
			_current_menu_option = SAVE_AND_RUN_MAIN_SCENE;
			_menu_option_confirm(SCENE_SAVE_AS_SCENE, true);
			_scene_resource_file_dialog->set_title(TTR("Save scene before running..."));
		} else {
			_current_menu_option = SETTINGS_PICK_MAIN_SCENE;
			_dialog_action(scene->get_scene_file_path());
		}
	}
}

void EditorNode::_immediate_dialog_confirmed() {
	_immediate_dialog_was_confirmed = true;
}

void EditorNode::_begin_first_scan() {
	if (!_waiting_for_first_scan) {
		return;
	}
	_requested_first_scan = true;
}

// Recursive function to inform nodes that an array of nodes have had their scene reimported.
// It will attempt to call a method named '_nodes_scene_reimported' on every node in the
// tree so that editor scripts which create transient nodes will have the opportunity
// to recreate them.
void EditorNode::_notify_nodes_scene_reimported(Node *p_node, Array p_reimported_nodes) {
	Skeleton3D *skel_3d = Object::cast_to<Skeleton3D>(p_node);
	if (skel_3d) {
		skel_3d->reset_bone_poses();
	} else {
		BoneAttachment3D *attachment = Object::cast_to<BoneAttachment3D>(p_node);
		if (attachment) {
			attachment->notify_rebind_required();
		}
	}

	p_node->call(SNAME("_nodes_scene_reimported"), p_reimported_nodes);

	for (int i = 0; i < p_node->get_child_count(); i++) {
		_notify_nodes_scene_reimported(p_node->get_child(i), p_reimported_nodes);
	}
}

void EditorNode::_remove_all_not_owned_children(Node *p_node, Node *p_owner) {
	Vector<Node *> nodes_to_remove;
	if (p_node != p_owner && p_node->get_owner() != p_owner) {
		nodes_to_remove.push_back(p_node);
	}
	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node *child_node = p_node->get_child(i);
		_remove_all_not_owned_children(child_node, p_owner);
	}

	for (Node *node : nodes_to_remove) {
		node->get_parent()->remove_child(node);
		node->queue_free();
	}
}

void EditorNode::_progress_dialog_visibility_changed() {
	// Open the io errors after the progress dialog is closed.
	if (_load_errors_queued_to_display && !_progress_dialog->is_visible()) {
		EditorInterface::get_singleton()->popup_dialog_centered_ratio(_singleton->_load_error_dialog, 0.5);
		_load_errors_queued_to_display = false;
	}
}

void EditorNode::_load_error_dialog_visibility_changed() {
	if (!_load_error_dialog->is_visible()) {
		_load_error_log->clear();
	}
}

bool EditorNode::_is_project_data_missing() {
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	const String project_data_dir = EditorPaths::get_singleton()->get_project_data_dir();
	if (!da->dir_exists(project_data_dir)) {
		return true;
	}

	String project_data_gdignore_file_path = project_data_dir.path_join(".gdignore");
	if (!FileAccess::exists(project_data_gdignore_file_path)) {
		Ref<FileAccess> f = FileAccess::open(project_data_gdignore_file_path, FileAccess::WRITE);
		if (f.is_valid()) {
			f->store_line("");
		} else {
			ERR_PRINT("Failed to create file " + project_data_gdignore_file_path.quote() + ".");
		}
	}

	String uid_cache = ResourceUID::get_singleton()->get_cache_file();
	if (!da->file_exists(uid_cache)) {
		Error err = ResourceUID::get_singleton()->save_to_cache();
		if (err != OK) {
			ERR_PRINT("Failed to create file " + uid_cache.quote() + ".");
		}
	}

	const String dirs[] = {
		EditorPaths::get_singleton()->get_project_settings_dir(),
		ProjectSettings::get_singleton()->get_imported_files_path()
	};
	for (const String &dir : dirs) {
		if (!da->dir_exists(dir)) {
			return true;
		}
	}
	return false;
}

void EditorNode::_build_file_menu() {
	if (!_file_menu) {
		return;
	}
	_file_menu->clear(false);

	_file_menu->add_icon_shortcut(_theme->get_icon(SNAME("action_create_new_scene_from"), EditorStringName(Editor)), ED_GET_SHORTCUT("editor/new_scene"), SCENE_NEW_SCENE);
	_file_menu->add_shortcut(ED_GET_SHORTCUT("editor/new_inherited_scene"), SCENE_NEW_INHERITED_SCENE);
	_file_menu->add_icon_shortcut(_theme->get_icon(SNAME("resource_packed_scene"), EditorStringName(Editor)), ED_GET_SHORTCUT("editor/open_scene"), SCENE_OPEN_SCENE);
	_file_menu->add_shortcut(ED_GET_SHORTCUT("editor/reopen_closed_scene"), SCENE_OPEN_PREV);
	if (!_recent_scenes_menu) {
		_recent_scenes_menu = memnew(PopupMenu);
		_recent_scenes_menu->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
		_recent_scenes_menu->connect("about_to_popup", callable_mp(this, &EditorNode::_update_recent_scenes));
		_recent_scenes_menu->connect(SceneStringName(id_pressed), callable_mp(this, &EditorNode::_open_recent_scene));
	}
	_file_menu->add_submenu_node_item(TTRC("Open Recent"), _recent_scenes_menu, SCENE_OPEN_RECENT);
	_file_menu->add_separator();

	_file_menu->add_icon_shortcut(_theme->get_icon(SNAME("action_save"), EditorStringName(Editor)), ED_GET_SHORTCUT("editor/save_scene"), SCENE_SAVE_SCENE);
	_file_menu->add_shortcut(ED_GET_SHORTCUT("editor/save_scene_as"), SCENE_SAVE_AS_SCENE);
	_file_menu->add_shortcut(ED_GET_SHORTCUT("editor/save_all_scenes"), SCENE_SAVE_ALL_SCENES);
	_file_menu->add_separator();

	_file_menu->add_icon_shortcut(_theme->get_icon(SNAME("action_load"), EditorStringName(Editor)), ED_GET_SHORTCUT("editor/quick_open"), SCENE_QUICK_OPEN);
	_file_menu->add_shortcut(ED_GET_SHORTCUT("editor/quick_open_scene"), SCENE_QUICK_OPEN_SCENE);
	_file_menu->add_shortcut(ED_GET_SHORTCUT("editor/quick_open_script"), SCENE_QUICK_OPEN_SCRIPT);
	_file_menu->add_separator();

	if (!_export_as_menu) {
		_export_as_menu = memnew(PopupMenu);
		_export_as_menu->add_shortcut(ED_GET_SHORTCUT("editor/export_as_mesh_library"), FILE_EXPORT_MESH_LIBRARY);
		_export_as_menu->connect("index_pressed", callable_mp(this, &EditorNode::_export_as_menu_option));
	}
	_file_menu->add_submenu_node_item(TTRC("Export As..."), _export_as_menu, SCENE_EXPORT_AS);
	_file_menu->add_separator();

	_file_menu->add_shortcut(ED_GET_SHORTCUT("editor/reload_saved_scene"), SCENE_RELOAD_SAVED_SCENE);
	_file_menu->add_icon_shortcut(_theme->get_icon(SNAME("action_close_scene"), EditorStringName(Editor)), ED_GET_SHORTCUT("editor/close_scene"), SCENE_CLOSE);
	_file_menu->add_shortcut(ED_GET_SHORTCUT("editor/close_all_scenes"), SCENE_CLOSE_ALL);
	_file_menu->add_separator();
	_file_menu->add_icon_shortcut(_theme->get_icon(SNAME("action_close"), EditorStringName(Editor)), ED_GET_SHORTCUT("editor/file_quit"), SCENE_QUIT, true);
}

void EditorNode::_build_project_menu() {
	if (!_project_menu) {
		return;
	}
	_project_menu->clear(false);

	_project_menu->add_icon_item(_theme->get_icon(SNAME("gui_autoload"), EditorStringName(Editor)), TTRC("Autoloads"), PROJECT_OPEN_AUTOLOADS);
	_project_menu->add_shortcut(ED_GET_SHORTCUT("editor/find_in_files"), PROJECT_FIND_IN_FILES);
	_project_menu->add_shortcut(ED_GET_SHORTCUT("editor/replace_in_files"), PROJECT_REPLACE_IN_FILES);
	_project_menu->add_separator();

	_project_menu->add_item(TTRC("Version Control"), PROJECT_VERSION_CONTROL);
	if (!_vcs_actions_menu) {
		_vcs_actions_menu = VersionControlEditorPlugin::get_singleton()->get_version_control_actions_panel();
		_vcs_actions_menu->connect("index_pressed", callable_mp(this, &EditorNode::_version_control_menu_option));
		_vcs_actions_menu->add_item(TTRC("Create/Override Version Control Metadata..."), VCS_METADATA);
		_vcs_actions_menu->add_item(TTRC("Version Control Settings..."), VCS_SETTINGS);
	}
	_project_menu->set_item_submenu_node(_project_menu->get_item_index(PROJECT_VERSION_CONTROL), _vcs_actions_menu);

	_project_menu->add_separator();
	_project_menu->add_icon_shortcut(_theme->get_icon(SNAME("node_resource_preloader"), EditorStringName(Editor)), ED_GET_SHORTCUT("editor/export"), PROJECT_EXPORT);
	_project_menu->add_item(TTRC("Pack Project as ZIP..."), PROJECT_PACK_AS_ZIP);
	_project_menu->add_item(TTRC("Setup Android Build..."), PROJECT_SETUP_ANDROID_BUILD);
#ifndef ANDROID_ENABLED
	_project_menu->add_item(TTRC("Open User Data Folder"), PROJECT_OPEN_USER_DATA_FOLDER);
#endif
	_project_menu->add_separator();

	_project_menu->add_icon_item(_theme->get_icon(SNAME("action_plugin"), EditorStringName(Editor)), TTRC("Plugins"), PROJECT_OPEN_PLUGINS);

	_project_menu->add_separator();
	_project_menu->add_shortcut(ED_GET_SHORTCUT("editor/reload_current_project"), PROJECT_RELOAD_CURRENT_PROJECT);
	_project_menu->add_icon_shortcut(_theme->get_icon(SNAME("action_close"), EditorStringName(Editor)), ED_GET_SHORTCUT("editor/quit_to_project_list"), PROJECT_QUIT_TO_HUB, true);
}

void EditorNode::_build_settings_menu() {
	if (!_settings_menu) {
		return;
	}
	_settings_menu->clear(false);

	_settings_menu->add_submenu_node_item(TTRC("Editor Docks"), _editor_dock_manager->get_docks_menu());

	if (!_editor_layouts_menu) {
		_editor_layouts_menu = memnew(PopupMenu);
		_editor_layouts_menu->connect(SceneStringName(id_pressed), callable_mp(this, &EditorNode::_layout_menu_option));
	}
	_settings_menu->add_submenu_node_item(TTRC("Editor Layout"), _editor_layouts_menu);
	_settings_menu->add_separator();

	_settings_menu->add_shortcut(ED_GET_SHORTCUT("editor/take_screenshot"), EDITOR_TAKE_SCREENSHOT);
	_settings_menu->set_item_tooltip(-1, TTRC("Screenshots are stored in the user data folder (\"user://\")."));

	_settings_menu->add_separator();

#ifndef ANDROID_ENABLED
	if (EditorPaths::get_singleton()->get_data_dir() == EditorPaths::get_singleton()->get_config_dir()) {
		// Configuration and data folders are located in the same place.
		_settings_menu->add_item(TTRC("Open Editor Data/Settings Folder"), EDITOR_OPEN_DATA_FOLDER);
	} else {
		// Separate configuration and data folders.
		_settings_menu->add_item(TTRC("Open Editor Data Folder"), EDITOR_OPEN_DATA_FOLDER);
		_settings_menu->add_item(TTRC("Open Editor Settings Folder"), EDITOR_OPEN_CONFIG_FOLDER);
	}
	_settings_menu->add_separator();
#endif

	_settings_menu->add_item(TTRC("Manage Editor Features..."), EDITOR_MANAGE_FEATURE_PROFILES);
	_settings_menu->add_item(TTRC("Manage Export Templates..."), EDITOR_MANAGE_EXPORT_TEMPLATES);
#if !defined(ANDROID_ENABLED)
	_settings_menu->add_item(TTRC("Configure FBX Importer..."), EDITOR_CONFIGURE_FBX_IMPORTER);
#endif
}

void EditorNode::_update_main_menu_type() {
	const bool use_menu_button = EDITOR_GET("interface/editor/appearance/collapse_main_menu");
	const MenuType new_menu_type = use_menu_button ? MenuType::COMPACT : MenuType::FULL;

	if (new_menu_type == _menu_type) {
		return; // Nothing to do.
	}
	_menu_type = new_menu_type;

	// Update menu items.
	_build_file_menu();
	_build_project_menu();
	_build_settings_menu();

	// Delete all menu.
	if (_main_menu_bar) {
		for (PopupMenu *menu : _main_menu_items) {
			if (menu->get_parent() == _main_menu_bar) {
				_main_menu_bar->remove_child(menu);
			}
		}
		memdelete(_main_menu_bar);
		_main_menu_bar = nullptr;
	}
	if (_main_menu_btn) {
		PopupMenu *popup = _main_menu_btn->get_popup();
		popup->clear(false);
		for (PopupMenu *menu : _main_menu_items) {
			if (menu->get_parent() == popup) {
				popup->remove_child(menu);
			}
		}
		memdelete(_main_menu_btn);
		_main_menu_btn = nullptr;
	}
	HBoxContainer *left_section = _editor_header->get_left_section();
	const int menu_index = _left_menu_spacer ? _left_menu_spacer->get_index() + 2 : 1;

	if (new_menu_type == MenuType::COMPACT) {
		_main_menu_btn = memnew(MenuButton);
		if (is_inside_tree()) {
			_main_menu_btn->set_button_icon(_theme->get_icon(SNAME("gui_triple_bar"), EditorStringName(Editor)));
		}
		_main_menu_btn->set_focus_mode(Control::FOCUS_NONE);
		_main_menu_btn->set_switch_on_hover(true);
		_main_menu_btn->set_text(TTRC("Main Menu"));
		_main_menu_btn->set_theme_type_variation("MainMenuButton");
		left_section->add_child(_main_menu_btn);
		left_section->move_child(_main_menu_btn, menu_index);

		{
			PopupMenu *popup = _main_menu_btn->get_popup();
			for (PopupMenu *menu : _main_menu_items) {
				popup->add_submenu_node_item(menu->get_name(), menu);
			}
		}
	} else {
		_main_menu_bar = memnew(MenuBar);
		_main_menu_bar->set_mouse_filter(Control::MOUSE_FILTER_STOP);
		_main_menu_bar->set_prefer_global_menu(false);
		_main_menu_bar->set_switch_on_hover(true);
		_main_menu_bar->set_theme_type_variation("MainMenuBar");
		_main_menu_bar->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
		left_section->add_child(_main_menu_bar);
		left_section->move_child(_main_menu_bar, menu_index);

		{
			for (PopupMenu *menu : _main_menu_items) {
				_main_menu_bar->add_child(menu);
			}
		}
	}
}

void EditorNode::_add_to_main_menu(const String &p_name, PopupMenu *r_menu) {
	r_menu->set_name(p_name);
	_main_menu_items.push_back(r_menu);
}

void EditorNode::_copy_warning(const String &p_str) {
	DisplayServer::get_singleton()->clipboard_set(_warning_dialog->get_text());
}

#ifdef ANDROID_ENABLED
void EditorNode::_screen_orientation_changed(int p_new_orientation) {
	_portrait_mode = p_new_orientation == DisplayServerEnums::SENSOR_ORIENTATION_PORTRAIT;
}
#endif
