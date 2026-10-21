/**************************************************************************/
/*  project_list.cpp                                                      */
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

#include "project_list.h"

#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/io/dir_access.h"
#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "core/os/time.h"
#include "core/version.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_paths.h"
#include "editor/project_manager/project_manager.h"
#include "editor/project_manager/project_tag.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/dialog.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/progress_bar.h"
#include "scene/gui/texture_button.h"
#include "scene/gui/texture_rect.h"
#include "scene/resources/image_texture.h"
#include "servers/display/display_server.h"

void ProjectListItemControl::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			if (_thumbnail_needs_reload) {
				// The project thumbnail may not be loaded by the time the control is displayed,
				// so use a loading placeholder.
				_project_thumbnail->set_texture(get_editor_theme_icon(SNAME("gui_project_thumbnail_loading")));
			}

			project_title->begin_bulk_theme_override();
			project_title->add_theme_font_override(SceneStringName(font), get_theme_font(SNAME("title"), EditorStringName(EditorFonts)));
			project_title->add_theme_font_size_override(SceneStringName(font_size), get_theme_font_size(SNAME("title_size"), EditorStringName(EditorFonts)));
			project_title->add_theme_color_override(SceneStringName(font_color), get_theme_color(SceneStringName(font_color), SNAME("ProjectList")));
			project_title->end_bulk_theme_override();

			project_path->add_theme_color_override(SceneStringName(font_color), get_theme_color(SceneStringName(font_color), SNAME("ProjectList")));

			_update_version_icon();

			project_unsupported_features->set_texture(get_editor_theme_icon(SNAME("status_node_warning")));

			favorite_focus_color = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
			_update_favorite_button_focus_color();
			if (is_favorite) {
				favorite_button->set_texture_normal(get_editor_theme_icon(SNAME("action_unfavorite")));
			} else {
				favorite_button->set_texture_normal(get_editor_theme_icon(SNAME("action_favorite")));
			}

			if (project_is_missing) {
				explore_button->set_button_icon(get_editor_theme_icon(SNAME("file_broken")));
#if !defined(ANDROID_ENABLED)
			} else {
				explore_button->set_button_icon(get_editor_theme_icon(SNAME("action_load")));
#endif
			}
			if (touch_menu_button) {
				touch_menu_button->set_button_icon(get_editor_theme_icon(SNAME("gui_tab_menu_hl")));
			}
		} break;

		case NOTIFICATION_MOUSE_ENTER: {
			is_hovering = true;
			queue_redraw();
		} break;

		case NOTIFICATION_MOUSE_EXIT: {
			is_hovering = false;
			queue_redraw();
		} break;

		case NOTIFICATION_FOCUS_ENTER: {
			ProjectList *pl = get_list();
			if (pl) {
				int idx = pl->get_index(this);
				if (idx >= 0) {
					// has_focus(true) is false on mouse-initiated focus, true on keyboard navigation.
					pl->select_project(idx, !has_focus(true));

					pl->emit_signal(SNAME(ProjectList::SIGNAL_SELECTION_CHANGED));
				}
			}
		} break;

		case NOTIFICATION_DRAW: {
			if (is_selected && is_hovering) {
				draw_style_box(get_theme_stylebox(SNAME("hover_pressed"), SNAME("ProjectList")), Rect2(Point2(), get_size()));
			} else if (is_selected) {
				draw_style_box(get_theme_stylebox(SNAME("selected"), SNAME("ProjectList")), Rect2(Point2(), get_size()));
			} else if (is_hovering) {
				draw_style_box(get_theme_stylebox(SNAME("hovered"), SNAME("ProjectList")), Rect2(Point2(), get_size()));
			}
			// Due to how this control works, we can't rely on the built-in way of checking for focus visibility.
			if (has_focus() && !is_focus_hidden) {
				draw_style_box(get_theme_stylebox(SNAME("focus"), SNAME("ProjectList")), Rect2(Point2(), get_size()));
			}

			draw_line(Point2(0, get_size().y + 1), Point2(get_size().x, get_size().y + 1), get_theme_color(SNAME("guide_color"), SNAME("ProjectList")));
		} break;

		case NOTIFICATION_READY: {
			set_project_title_autowrap();
		} break;
	}
}

ProjectList *ProjectListItemControl::get_list() const {
	if (!is_inside_tree()) {
		return nullptr;
	}
	ProjectList *pl = Object::cast_to<ProjectList>(get_parent()->get_parent());
	return pl;
}

void ProjectListItemControl::_update_favorite_button_focus_color() {
	if (favorite_button->has_focus()) {
		favorite_button->set_self_modulate(favorite_focus_color);
	} else {
		favorite_button->set_self_modulate(Color(1.0, 1.0, 1.0, 1.0));
	}
}

void ProjectListItemControl::_favorite_button_pressed() {
	emit_signal(SNAME("favorite_pressed"));
}

void ProjectListItemControl::_explore_button_pressed() {
	emit_signal(SNAME("explore_pressed"));
}

void ProjectListItemControl::_request_menu() {
	emit_signal(SNAME("request_menu"), Vector2(touch_menu_button->get_position()));
}

void ProjectListItemControl::set_project_title(const String &p_title) {
	project_title->set_text(p_title);
}

void ProjectListItemControl::set_project_path(const String &p_path) {
	project_path->set_text(p_path);
}

void ProjectListItemControl::set_tags(const PackedStringArray &p_tags, ProjectList *p_parent_list) {
	for (const String &tag : p_tags) {
		ProjectTag *tag_control = memnew(ProjectTag(tag));
		tag_container->add_child(tag_control);
		tag_control->connect_button_to(callable_mp(p_parent_list, &ProjectList::add_search_tag).bind(tag));
	}
}

void ProjectListItemControl::set_project_thumbnail(const Ref<Texture2D> &p_thumbnail) {
	_thumbnail_needs_reload = false;

	// The default project thumbnail is 128×128 to look crisp on hiDPI displays,
	// but we want the actual displayed size to be 64×64 on loDPI displays.
	_project_thumbnail->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	_project_thumbnail->set_custom_minimum_size(Size2(64, 64) * EDSCALE);
	_project_thumbnail->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);

	_project_thumbnail->set_texture(p_thumbnail);
}

void ProjectListItemControl::set_last_edited_info(const String &p_info) {
	last_edited_info->set_text(p_info);
}

void ProjectListItemControl::_update_version_icon() {
	switch (version_match_type) {
		case VersionMatchType::PROJECT_USES_OLDER_MAJOR:
			project_different_version->set_texture(get_editor_theme_icon(SNAME("gui_project_upgrade_major")));
			break;
		case VersionMatchType::PROJECT_USES_OLDER_MINOR:
			project_different_version->set_texture(get_editor_theme_icon(SNAME("gui_project_upgrade")));
			break;
		case VersionMatchType::PROJECT_USES_NEWER_MAJOR:
			project_different_version->set_texture(get_editor_theme_icon(SNAME("gui_project_downgrade_major")));
			break;
		case VersionMatchType::PROJECT_USES_NEWER_MINOR:
			project_different_version->set_texture(get_editor_theme_icon(SNAME("gui_project_downgrade")));
			break;
		default:
			project_different_version->set_texture(get_editor_theme_icon(SNAME("status_warning")));
			break;
	}
}

void ProjectListItemControl::set_project_version(const String &p_info) {
	project_version->set_text(p_info.is_empty() ? TTR("Unknown") : p_info);
	version_match_type = VersionMatchType::PROJECT_USES_SAME;
	project_different_version->hide();
	if (p_info.is_empty()) {
		project_version->set_tooltip_text(TTR("The project does not record an editor version."));
		return;
	}
	const int major = p_info.get_slicec('.', 0).to_int();
	const int minor = p_info.get_slicec('.', 1).to_int();
	if (major < GODOT_VERSION_MAJOR) {
		version_match_type = VersionMatchType::PROJECT_USES_OLDER_MAJOR;
	} else if (major > GODOT_VERSION_MAJOR) {
		version_match_type = VersionMatchType::PROJECT_USES_NEWER_MAJOR;
	} else if (minor < GODOT_VERSION_MINOR) {
		version_match_type = VersionMatchType::PROJECT_USES_OLDER_MINOR;
	} else if (minor > GODOT_VERSION_MINOR) {
		version_match_type = VersionMatchType::PROJECT_USES_NEWER_MINOR;
	}
	_update_version_icon();
	project_version->set_tooltip_text(String());
	if (p_info != GODOT_VERSION_NUMBER) {
		project_different_version->set_focus_mode(FOCUS_NONE);
		project_different_version->set_tooltip_text(vformat(TTR("This project was last saved with editor %s. This editor is %s. Open the project with the appropriate editor version."), p_info, GODOT_VERSION_NUMBER));
		project_different_version->show();
	}
}

void ProjectListItemControl::set_unsupported_features(PackedStringArray p_features) {
	project_unsupported_features->hide();
	if (p_features.is_empty()) {
		return;
	}
	const String tooltip_text = TTR("This project uses features unsupported by the current build:") + "\n" + String(", ").join(p_features);
	project_unsupported_features->set_focus_mode(FOCUS_NONE);
	project_unsupported_features->set_tooltip_text(tooltip_text);
	project_unsupported_features->show();
}

bool ProjectListItemControl::should_load_project_thumbnail() const {
	return _thumbnail_needs_reload;
}

void ProjectListItemControl::set_selected(bool p_selected, bool p_hide_focus) {
	is_selected = p_selected;
	is_focus_hidden = is_selected && p_hide_focus;
	queue_redraw();
}

void ProjectListItemControl::set_is_favorite(bool p_favorite) {
	is_favorite = p_favorite;
	if (p_favorite) {
		favorite_button->set_texture_normal(get_editor_theme_icon(SNAME("action_unfavorite")));
	} else {
		favorite_button->set_texture_normal(get_editor_theme_icon(SNAME("action_favorite")));
	}
}

void ProjectListItemControl::set_is_missing(bool p_missing) {
	project_is_missing = p_missing;

	if (project_is_missing) {
		explore_button->set_button_icon(get_editor_theme_icon(SNAME("file_broken")));
		explore_button->set_tooltip_text(TTRC("Error: Project is missing on the filesystem."));
	} else {
#if defined(ANDROID_ENABLED)
		// Opening the system file manager is not supported on the Android editor.
		explore_button->hide();
#else
		explore_button->set_button_icon(get_editor_theme_icon(SNAME("action_load")));
		explore_button->set_tooltip_text(OS::get_singleton()->get_platform_string(OS::PLATFORM_STRING_FILE_MANAGER_OPEN));
#endif
	}
}

void ProjectListItemControl::set_is_grayed(bool p_grayed) {
	if (p_grayed) {
		main_vbox->set_modulate(Color(1, 1, 1, 0.5));
		// Don't make the thumbnail less prominent if the parent is already grayed out.
		explore_button->set_modulate(Color(1, 1, 1, 1.0));
	} else {
		main_vbox->set_modulate(Color(1, 1, 1, 1.0));
		explore_button->set_modulate(Color(1, 1, 1, 0.5));
	}
}

void ProjectListItemControl::set_project_title_autowrap() {
	ProjectList *pl = get_list();

	title_fullsize_cache = project_title->get_size().x;
	pl->window_size_cache = get_window()->get_size().x;

	int tag_maxsize = 0;
	int tag_size = 0;
	for (Node *child : tag_container->iterate_children()) {
		ProjectTag *tag = Object::cast_to<ProjectTag>(child);

		tag_size += tag->get_size().x;

		if (tag_maxsize == 0) {
			tag_maxsize = tag->get_custom_maximum_size().x;
		}
	}
	tag_size_cache = MIN(tag_size, tag_maxsize);

	int &title_size = pl->title_size_cache;
	const int size_check = 800 * EDSCALE;
	if (title_size == 0) {
		title_size = size_check;
	}

	if (title_fullsize_cache > size_check - tag_size_cache) {
		resize_project_title(title_size);
	}
}

void ProjectListItemControl::resize_project_title(const int p_title_size) {
	bool autowrap = project_title->get_autowrap_mode() != TextServer::AUTOWRAP_OFF;
	if (autowrap && p_title_size > title_fullsize_cache + tag_size_cache) {
		project_title->set_custom_maximum_size(Vector2(-1, -1));
		project_title->set_custom_minimum_size(Vector2(0, 0));
		project_title->set_autowrap_mode(TextServer::AUTOWRAP_OFF);

		return;
	}

	int abs_minsize = (200 * EDSCALE);
	if (title_fullsize_cache > abs_minsize && p_title_size < title_fullsize_cache + tag_size_cache) {
		ProjectTag *tag = memnew(ProjectTag("dummy"));
		int tag_maxsize = tag->get_custom_maximum_size().x;
		memdelete(tag);
		int title_maxsize = p_title_size - tag_size_cache;
		int title_minsize = abs_minsize + tag_maxsize - tag_size_cache;

		project_title->set_custom_maximum_size(Vector2(MAX(title_maxsize, title_minsize), -1));
		project_title->set_custom_minimum_size(Vector2(MAX(title_maxsize, title_minsize), 0));
		project_title->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	}
}

void ProjectListItemControl::_bind_methods() {
	ADD_SIGNAL(MethodInfo("favorite_pressed"));
	ADD_SIGNAL(MethodInfo("explore_pressed"));
	ADD_SIGNAL(MethodInfo("request_menu"));
}

ProjectListItemControl::ProjectListItemControl() {
	set_focus_mode(FocusMode::FOCUS_ALL);
	set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);

	// Left spacer.
	add_child(memnew(Control));

	VBoxContainer *favorite_box = memnew(VBoxContainer);
	favorite_box->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	add_child(favorite_box);

	favorite_button = memnew(TextureButton);
	favorite_button->set_name("FavoriteButton");
	favorite_button->set_tooltip_text(TTRC("Toggle Favorite"));
	favorite_button->set_auto_translate_mode(AUTO_TRANSLATE_MODE_ALWAYS);
	// This makes the project's "hover" style display correctly when hovering the favorite icon.
	favorite_button->set_mouse_filter(MOUSE_FILTER_PASS);
	favorite_box->add_child(favorite_button);
	favorite_button->connect(SceneStringName(pressed), callable_mp(this, &ProjectListItemControl::_favorite_button_pressed));
	favorite_button->connect(SceneStringName(focus_entered), callable_mp(this, &ProjectListItemControl::_update_favorite_button_focus_color));
	favorite_button->connect(SceneStringName(focus_exited), callable_mp(this, &ProjectListItemControl::_update_favorite_button_focus_color));

	_project_thumbnail = memnew(TextureRect);
	_project_thumbnail->set_name("ProjectThumbnail");
	_project_thumbnail->set_v_size_flags(SIZE_SHRINK_CENTER);
	add_child(_project_thumbnail);

	main_vbox = memnew(VBoxContainer);
	main_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(main_vbox);

	Control *ec = memnew(Control);
	ec->set_custom_minimum_size(Size2(0, 1));
	ec->set_mouse_filter(MOUSE_FILTER_PASS);
	main_vbox->add_child(ec);

	// Top half, title, tags and unsupported features labels.
	{
		HBoxContainer *title_hb = memnew(HBoxContainer);
		main_vbox->add_child(title_hb);

		project_title = memnew(Label);
		project_title->set_name("ProjectName");
		project_title->set_h_size_flags(Control::SIZE_FILL);
		title_hb->add_child(project_title);

		tag_container = memnew(HFlowContainer);
		tag_container->set_alignment(FlowContainer::ALIGNMENT_END);
		tag_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		title_hb->add_child(tag_container);
	}

	// Bottom half, containing the path and view folder button.
	{
		HBoxContainer *path_hb = memnew(HBoxContainer);
		path_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		main_vbox->add_child(path_hb);

		explore_button = memnew(Button);
		explore_button->set_name("ExploreButton");
		explore_button->set_tooltip_auto_translate_mode(AUTO_TRANSLATE_MODE_ALWAYS);
		explore_button->set_mouse_filter(MOUSE_FILTER_PASS);
		explore_button->set_tooltip_text(TTRC("Open in file manager"));
		explore_button->set_flat(true);
		path_hb->add_child(explore_button);
		explore_button->connect(SceneStringName(pressed), callable_mp(this, &ProjectListItemControl::_explore_button_pressed));

		project_path = memnew(Label);
		project_path->set_name("ProjectPath");
		project_path->set_structured_text_bidi_override(TextServer::STRUCTURED_TEXT_FILE);
		project_path->set_clip_text(true);
		project_path->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		project_path->set_modulate(Color(1, 1, 1, 0.5));
		path_hb->add_child(project_path);

		project_unsupported_features = memnew(TextureRect);
		project_unsupported_features->set_name("ProjectUnsupportedFeatures");
		project_unsupported_features->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
		path_hb->add_child(project_unsupported_features);
		project_unsupported_features->hide();

		project_different_version = memnew(TextureRect);
		project_different_version->set_name("ProjectDifferentVersion");
		project_different_version->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
		path_hb->add_child(project_different_version);
		project_different_version->hide();

		project_version = memnew(Label);
		project_version->set_name("ProjectVersion");
		project_version->set_mouse_filter(Control::MOUSE_FILTER_PASS);
		path_hb->add_child(project_version);

		last_edited_info = memnew(Label);
		last_edited_info->set_name("LastEditedInfo");
		last_edited_info->set_mouse_filter(Control::MOUSE_FILTER_PASS);
		last_edited_info->set_tooltip_auto_translate_mode(AUTO_TRANSLATE_MODE_ALWAYS);
		last_edited_info->set_tooltip_text(TTRC("Last edited timestamp"));
		last_edited_info->set_modulate(Color(1, 1, 1, 0.5));
		path_hb->add_child(last_edited_info);
	}

	if (DisplayServer::get_singleton()->is_touchscreen_available()) {
		touch_menu_button = memnew(Button);
		touch_menu_button->set_theme_type_variation(SceneStringName(FlatButton));
		touch_menu_button->set_v_size_flags(SIZE_SHRINK_CENTER);
		add_child(touch_menu_button);
		touch_menu_button->set_mouse_filter(MOUSE_FILTER_PASS);
		touch_menu_button->connect(SceneStringName(pressed), callable_mp(this, &ProjectListItemControl::_request_menu));
	}

	// Right spacer.
	add_child(memnew(Control));
}

struct ProjectListComparator {
	ProjectList::FilterOption order_option = ProjectList::FilterOption::EDIT_DATE;

	// operator<
	_FORCE_INLINE_ bool operator()(const ProjectList::Item &a, const ProjectList::Item &b) const {
		if (a.favorite && !b.favorite) {
			return true;
		}
		if (b.favorite && !a.favorite) {
			return false;
		}
		switch (order_option) {
			case ProjectList::PATH:
				return a.path < b.path;
			case ProjectList::EDIT_DATE:
				return a.last_edited > b.last_edited;
			case ProjectList::TAGS:
				return a.tag_sort_string < b.tag_sort_string;
			default:
				return a.project_name < b.project_name;
		}
	}
};

String ProjectList::Item::get_last_edited_string() const {
	if (missing) {
		return TTR("Missing Date");
	}

	OS::TimeZoneInfo tz = OS::get_singleton()->get_time_zone_info();
	return Time::get_singleton()->get_datetime_string_from_unix_time(last_edited + tz.bias * 60, true);
}

// Notifications.

void ProjectList::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_TRANSLATION_CHANGED: {
			if (is_ready()) {
				for (const Item &item : _projects) {
					_update_project_control_translatable_fields(item);
				}
				update_dock_menu();
			}
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			if (project_context_menu) {
				_update_menu_icons();
			}
		} break;

		case NOTIFICATION_PROCESS: {
			// Load thumbnails as a coroutine to speed up launch when you have hundreds of projects.
			if (_thumbnail_load_index < _projects.size()) {
				Item &item = _projects.write[_thumbnail_load_index];
				if (item.control->should_load_project_thumbnail()) {
					_load_project_thumbnail(_thumbnail_load_index);
				}
				_thumbnail_load_index++;

				// Scan directories in thread to avoid blocking the window.
			} else if (scan_data && scan_data->scan_in_progress.is_set()) {
				// Wait for the thread.
			} else {
				set_process(false);
				if (scan_data) {
					_scan_finished();
				}
			}
		} break;
	}
}

// Projects scan.

void ProjectList::_scan_thread(void *p_scan_data) {
	ScanData *scan_data = static_cast<ScanData *>(p_scan_data);

	for (const String &base_path : scan_data->paths_to_scan) {
		print_verbose(vformat("Scanning for projects in \"%s\".", base_path));
		_scan_folder_recursive(base_path, &scan_data->found_projects, scan_data->scan_in_progress);

		if (!scan_data->scan_in_progress.is_set()) {
			print_verbose("Scan aborted.");
			break;
		}
	}
	print_verbose(vformat("Found %d project(s).", scan_data->found_projects.size()));
	scan_data->scan_in_progress.clear();
}

void ProjectList::_scan_finished() {
	if (scan_data->scan_in_progress.is_set()) {
		// Abort scanning.
		scan_data->scan_in_progress.clear();
	}

	scan_data->thread->wait_to_finish();
	memdelete(scan_data->thread);
	if (scan_progress) {
		scan_progress->hide();
	}

	for (const String &E : scan_data->found_projects) {
		add_project(E, false);
	}
	memdelete(scan_data);
	scan_data = nullptr;

	save_config();

	if (ProjectManager::get_singleton()->is_initialized()) {
		update_project_list();
	}
}

// Initialization & loading.

void ProjectList::save_config() {
	_config.save(_config_path);
}

// Load project data from p_property_key and return it in a ProjectList::Item.
// p_favorite is passed directly into the Item.
ProjectList::Item ProjectList::load_project_data(const String &p_path, bool p_favorite) {
	String conf = p_path.path_join("project.godot");
	bool grayed = false;
	bool missing = false;
	bool recovery_mode = false;

	Ref<ConfigFile> cf = memnew(ConfigFile);
	Error cf_err = cf->load(conf);

	int config_version = 0;
	String cf_project_name;
	String project_name = TTR("Unnamed Project");
	if (cf_err == OK) {
		cf_project_name = cf->get_value("application", "config/about/name", "");
		if (!cf_project_name.is_empty()) {
			project_name = cf_project_name.xml_unescape();
		}
		config_version = (int)cf->get_value("", "config_version", 0);
	}

	if (config_version > ProjectSettings::CONFIG_VERSION) {
		// Comes from an incompatible (more recent) Godot version, gray it out.
		grayed = true;
	}

	const String description = cf->get_value("application", "config/about/description", "");
	const PackedStringArray tags = cf->get_value("application", "config/tags", PackedStringArray());
	const String main_scene = cf->get_value("application", "run/main_scene", "");

	String thumbnail = cf->get_value("application", "config/about/thumbnail", "");
	if (thumbnail.begins_with("uid://")) {
		Error err;
		Ref<FileAccess> file = FileAccess::open(p_path.path_join(".godot/uid_cache.bin"), FileAccess::READ, &err);
		if (err == OK) {
			thumbnail = ResourceUID::get_path_from_cache(file, thumbnail);
			if (thumbnail.is_empty()) {
				WARN_PRINT(vformat("Could not load thumbnail from UID for project at path \"%s\". Make sure UID cache exists.", p_path));
			}
		} else {
			// Cache does not exist yet, so ignore and fallback to default thumbnail.
			thumbnail = "";
		}
	}

	const PackedStringArray project_features = cf->get_value("application", "config/features", PackedStringArray());
	const String rendering_method = cf->get_value("rendering", "renderer/rendering_method", GLOBAL_GET("rendering/renderer/rendering_method"));
	const PackedStringArray unsupported_features = ProjectSettings::get_unsupported_features(project_features, rendering_method);
	const String project_version = cf->get_value("application", "config/editor_version", "");

	uint64_t last_edited = 0;
	if (cf_err == OK) {
		int *cached_time = modified_time_cache.getptr(p_path);
		if (cached_time) {
			// Modified time may change as a result of Project Manager actions, which will affect sorting.
			// For that reason, the time is read only once.
			last_edited = *cached_time;
		} else {
			// The modification date marks the date the project was last edited.
			// This is because the `project.godot` file will always be modified
			// when editing a project (but not when running it).
			last_edited = FileAccess::get_modified_time(conf);
			modified_time_cache[p_path] = last_edited;
		}
	} else {
		grayed = true;
		missing = true;
	}

	for (const String &tag : tags) {
		ProjectManager::get_singleton()->add_new_tag(tag);
	}

	// We can't use OS::get_user_data_dir() because it attempts to load paths from the current loaded project through ProjectSettings,
	// while here we're parsing project files externally. Therefore, we have to replicate its behavior.
	String user_dir = OS::get_singleton()->get_safe_dir_name(cf->get_value("application", "config/settings/custom_user_directory", ""), true);
	if (user_dir.is_empty()) {
		String appname = OS::get_singleton()->get_safe_dir_name(cf_project_name);
		if (appname.is_empty()) {
			appname = "[Unnamed Project]";
		}
		user_dir = OS::get_singleton()->get_godot_dir_name().path_join("app_userdata").path_join(appname);
	}

	String recovery_mode_lock_file = OS::get_singleton()->get_user_data_dir(user_dir).path_join(".recovery_mode_lock");
	recovery_mode = FileAccess::exists(recovery_mode_lock_file);

	return Item(project_name, description, project_version, tags, p_path, thumbnail, main_scene, unsupported_features, last_edited, p_favorite, grayed, missing, recovery_mode, config_version);
}

void ProjectList::_update_thumbnails_async() {
	_thumbnail_load_index = 0;
	set_process(true);
}

void ProjectList::_load_project_thumbnail(int p_index) {
	Item &item = _projects.write[p_index];

	Ref<Texture2D> default_thumbnail = get_editor_theme_icon(SNAME("gui_project_thumbnail_default"));
	Ref<Texture2D> thumbnail;
	if (!item.missing && !item.thumbnail.is_empty()) {
		Ref<Image> img;
		img.instantiate();
		Error err = img->load(item.thumbnail.replace_first("res://", item.path + "/"));
		if (err == OK) {
			img->resize(default_thumbnail->get_width(), default_thumbnail->get_height(), Image::INTERPOLATE_LANCZOS);
			thumbnail = ImageTexture::create_from_image(img);
		}
	}
	if (thumbnail.is_null()) {
		thumbnail = item.missing ? get_editor_theme_icon(SNAME("gui_project_thumbnail_missing")) : default_thumbnail;
	}

	item.control->set_project_thumbnail(thumbnail);
}

// Project list updates.

void ProjectList::update_project_list() {
	// This is a full, hard reload of the list. Don't call this unless really required, it's expensive.
	// If you have 150 projects, it may read through 150 files on your disk at once + load 150 thumbnails.
	// FIXME: Does it really have to be a full, hard reload? Runtime updates should be made much cheaper.

	if (ProjectManager::get_singleton()->is_initialized()) {
		// Clear whole list
		for (int i = 0; i < _projects.size(); ++i) {
			Item &project = _projects.write[i];
			CRASH_COND(project.control == nullptr);

			memdelete(project.control); // Why not queue_free()?
		}

		_projects.clear();
		_last_clicked = "";
		_selected_project_paths.clear();

		load_project_list();
	}

	// Create controls
	for (int i = 0; i < _projects.size(); ++i) {
		_create_project_item_control(i);
	}

	sort_projects();
	_update_thumbnails_async();
	update_dock_menu();

	set_v_scroll(0);
	emit_signal(SNAME(SIGNAL_LIST_CHANGED));
}

void ProjectList::sort_projects() {
	SortArray<Item, ProjectListComparator> sorter;
	sorter.compare.order_option = _order_option;
	sorter.sort(_projects.ptrw(), _projects.size());

	String search_term;
	PackedStringArray tags;

	if (!_search_term.is_empty()) {
		PackedStringArray search_parts = _search_term.split(" ");
		if (search_parts.size() > 1 || search_parts[0].begins_with("tag:")) {
			PackedStringArray remaining;
			for (const String &part : search_parts) {
				if (part.begins_with("tag:")) {
					tags.push_back(part.get_slicec(':', 1));
				} else {
					remaining.append(part);
				}
			}
			search_term = String(" ").join(remaining); // Search term without tags.
		} else {
			search_term = _search_term;
		}
	}

	for (int i = 0; i < _projects.size(); ++i) {
		Item &item = _projects.write[i];

		bool item_visible = true;
		if (!_search_term.is_empty()) {
			String search_path;
			if (search_term.contains_char('/')) {
				// Search path will match the whole path
				search_path = item.path;
			} else {
				// Search path will only match the last path component to make searching more strict
				search_path = item.path.get_file();
			}

			bool missing_tags = false;
			for (const String &tag : tags) {
				if (!item.tags.has(tag)) {
					missing_tags = true;
					break;
				}
			}

			// When searching, display projects whose name or path contain the search term and whose tags match the searched tags.
			item_visible = !missing_tags && (search_term.is_empty() || item.project_name.containsn(search_term) || search_path.containsn(search_term));
		}

		item.control->set_visible(item_visible);
	}

	for (int i = 0; i < _projects.size(); ++i) {
		Item &item = _projects.write[i];
		item.control->get_parent()->move_child(item.control, i);
	}

	// Rewind the coroutine because order of projects changed
	_update_thumbnails_async();
	update_dock_menu();
}

int ProjectList::get_project_count() const {
	return _projects.size();
}

void ProjectList::find_projects(const String &p_path) {
	PackedStringArray paths = { p_path };
	find_projects_multiple(paths);
}

void ProjectList::find_projects_multiple(const PackedStringArray &p_paths) {
	if (!scan_progress && is_inside_tree()) {
		scan_progress = memnew(Dialog);
		scan_progress->set_title(TTRC("Scanning"));
		scan_progress->set_ok_button_text(TTRC("Cancel"));

		VBoxContainer *vb = memnew(VBoxContainer);
		scan_progress->add_child(vb);

		Label *label = memnew(Label);
		label->set_text(TTRC("Scanning for projects..."));
		vb->add_child(label);

		ProgressBar *progress = memnew(ProgressBar);
		progress->set_indeterminate(true);
		vb->add_child(progress);

		add_child(scan_progress);
		scan_progress->connect(SceneStringName(confirmed), callable_mp(this, &ProjectList::_scan_finished));
		scan_progress->connect("canceled", callable_mp(this, &ProjectList::_scan_finished));
	}

	scan_data = memnew(ScanData);
	scan_data->paths_to_scan = p_paths;
	scan_data->scan_in_progress.set();

	scan_data->thread = memnew(Thread);
	scan_data->thread->start(_scan_thread, scan_data);

	if (scan_progress) {
		scan_progress->reset_size();
		scan_progress->popup_centered();
	}
	set_process(true);
}

void ProjectList::load_project_list() {
	_config.load(_config_path);
	Vector<String> sections = _config.get_sections();

	for (const String &path : sections) {
		bool favorite = _config.get_value(path, "favorite", false);
		_projects.push_back(load_project_data(path, favorite));
	}
}

void ProjectList::_scan_folder_recursive(const String &p_path, List<String> *r_projects, const SafeFlag &p_scan_active) {
	if (!p_scan_active.is_set()) {
		return;
	}

	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	Error error = da->change_dir(p_path);
	ERR_FAIL_COND_MSG(error != OK, vformat("Failed to open the path \"%s\" for scanning (code %d).", p_path, error));

	da->list_dir_begin();
	String n = da->get_next();
	while (!n.is_empty()) {
		if (!p_scan_active.is_set()) {
			return;
		}

		if (da->current_is_dir() && n[0] != '.') {
			_scan_folder_recursive(da->get_current_dir().path_join(n), r_projects, p_scan_active);
		} else if (n == "project.godot") {
			r_projects->push_back(da->get_current_dir());
		}
		n = da->get_next();
	}
	da->list_dir_end();
}

// Project list items.

void ProjectList::add_project(const String &dir_path, bool favorite) {
	if (!_config.has_section(dir_path)) {
		_config.set_value(dir_path, "favorite", favorite);
	}
}

int ProjectList::refresh_project(const String &dir_path) {
	// Reloads information about a specific project.
	// If it wasn't loaded and should be in the list, it is added (i.e new project).
	// If it isn't in the list anymore, it is removed.
	// If it is in the list but doesn't exist anymore, it is marked as missing.

	bool should_be_in_list = _config.has_section(dir_path);
	bool is_favorite = _config.get_value(dir_path, "favorite", false);

	bool was_selected = _selected_project_paths.has(dir_path);

	int temp_title_index = -1;

	// Remove item in any case
	for (int i = 0; i < _projects.size(); ++i) {
		const Item &existing_item = _projects[i];
		if (existing_item.path == dir_path) {
			temp_title_index = existing_item.project_title_index;

			_remove_project(i, false);
			break;
		}
	}

	int index = -1;
	if (should_be_in_list) {
		// Recreate it with updated info

		Item item = load_project_data(dir_path, is_favorite);

		item.project_title_index = temp_title_index;
		_projects.push_back(item);
		_create_project_item_control(_projects.size() - 1);

		sort_projects();

		for (int i = 0; i < _projects.size(); ++i) {
			if (_projects[i].path == dir_path) {
				if (was_selected) {
					ensure_project_visible(i);
				}
				_load_project_thumbnail(i);

				index = i;
				break;
			}
		}
	}

	return index;
}

int ProjectList::get_index(const ProjectListItemControl *p_control) const {
	for (int i = 0; i < _projects.size(); ++i) {
		if (_projects[i].control == p_control) {
			return i;
		}
	}
	return -1;
}

void ProjectList::ensure_project_visible(int p_index) {
	const Item &item = _projects[p_index];
	// Since follow focus is enabled.
	item.control->grab_focus(true);
}

void ProjectList::_create_project_item_control(int p_index) {
	// Will be added last in the list, so make sure indexes match
	ERR_FAIL_COND(p_index != project_list_vbox->get_child_count());

	Item &item = _projects.write[p_index];
	ERR_FAIL_COND(item.control != nullptr); // Already created

	ProjectListItemControl *hb = memnew(ProjectListItemControl);
	item.control = hb;

	hb->add_theme_constant_override("separation", 10 * EDSCALE);

	_update_project_control_translatable_fields(item);

	hb->set_project_path(item.path);
	hb->set_tooltip_text(item.description);
	hb->set_tags(item.tags, this);

	hb->set_is_favorite(item.favorite);
	hb->set_is_missing(item.missing);
	hb->set_is_grayed(item.grayed);

	hb->connect(SceneStringName(gui_input), callable_mp(this, &ProjectList::_list_item_input).bind(hb));
	hb->connect("favorite_pressed", callable_mp(this, &ProjectList::_on_favorite_pressed).bind(hb));

#if !defined(ANDROID_ENABLED)
	hb->connect("explore_pressed", callable_mp(this, &ProjectList::_on_explore_pressed).bind(item.path));
#endif
	hb->connect("request_menu", callable_mp(this, &ProjectList::_open_menu).bind(hb));

	project_list_vbox->add_child(hb);
}

void ProjectList::_update_project_control_translatable_fields(const Item &item) {
	ProjectListItemControl *control = item.control;

	control->set_project_title(!item.missing ? item.project_name : TTR("Missing Project"));
	control->set_last_edited_info(item.get_last_edited_string());
	control->set_project_version(item.project_version);
	control->set_unsupported_features(item.unsupported_features.duplicate());
}

void ProjectList::_toggle_project(int p_index) {
	// This methods adds to the selection or removes from the
	// selection.
	Item &item = _projects.write[p_index];

	if (_selected_project_paths.has(item.path)) {
		_deselect_project_nocheck(p_index);
	} else {
		_select_project_nocheck(p_index);
	}
}

void ProjectList::_remove_project(int p_index, bool p_update_config) {
	const Item item = _projects[p_index]; // Take a copy

	_selected_project_paths.erase(item.path);

	if (_last_clicked == item.path) {
		_last_clicked = "";
	}

	memdelete(item.control);
	_projects.remove_at(p_index);

	if (p_update_config) {
		_config.erase_section(item.path);
		// Not actually saving the file, in case you are doing more changes to settings
	}

	update_dock_menu();
}

void ProjectList::_list_item_input(const Ref<InputEvent> &p_ev, Control *p_hb) {
	Ref<InputEventMouseButton> mb = p_ev;
	int clicked_index = p_hb->get_index();
	const Item &clicked_project = _projects[clicked_index];

	if (mb.is_valid() && mb->is_pressed()) {
		if (mb->get_button_index() == MouseButton::LEFT) {
			if (mb->is_shift_pressed() && _selected_project_paths.size() > 0 && !_last_clicked.is_empty() && clicked_project.path != _last_clicked) {
				int anchor_index = -1;
				for (int i = 0; i < _projects.size(); ++i) {
					const Item &p = _projects[i];
					if (p.path == _last_clicked) {
						anchor_index = p.control->get_index();
						break;
					}
				}
				CRASH_COND(anchor_index == -1);
				_select_project_range(anchor_index, clicked_index);

			} else if (mb->is_command_or_control_pressed()) {
				_toggle_project(clicked_index);

			} else {
				_last_clicked = clicked_project.path;
				select_project(clicked_index, true);
			}

			emit_signal(SNAME(SIGNAL_SELECTION_CHANGED));

			// Do not allow opening a project more than once using a single project manager instance.
			// Opening the same project in several editor instances at once can lead to various issues.
			if (!mb->is_command_or_control_pressed() && mb->is_double_click() && !project_opening_initiated) {
				emit_signal(SNAME(SIGNAL_PROJECT_ASK_OPEN));
			}
		} else if (mb->get_button_index() == MouseButton::RIGHT) {
			_open_menu(mb->get_position(), p_hb);
		}
	}

	Ref<InputEventKey> kev = p_ev;

	if (kev.is_valid() && kev->is_pressed()) {
		switch (kev->get_keycode()) {
			case Key::ENTER: {
				if (!project_opening_initiated) {
					emit_signal(SNAME(SIGNAL_PROJECT_ASK_OPEN));
				}
				accept_event();
			} break;
			case Key::E: {
				_on_explore_pressed(clicked_project.path);
				accept_event();
			} break;
			case Key::F: {
				if (kev->is_command_or_control_pressed()) {
					return; // Focus the search box by the ProjectManager.
				}
				_on_favorite_pressed(p_hb);
				accept_event();
			} break;
			default: {
			} break;
		}
	}
}

void ProjectList::_on_favorite_pressed(Node *p_hb) {
	ProjectListItemControl *control = Object::cast_to<ProjectListItemControl>(p_hb);

	int index = control->get_index();
	Item item = _projects.write[index]; // Take copy

	item.favorite = !item.favorite;

	_config.set_value(item.path, "favorite", item.favorite);
	save_config();

	_projects.write[index] = item;

	control->set_is_favorite(item.favorite);

	sort_projects();

	// As controls are sorted, the calls are delayed in case follow focus does not take effect.
	if (Input::get_singleton()->is_key_pressed(Key::ALT)) {
		callable_mp((ScrollContainer *)this, &ScrollContainer::ensure_control_visible).call_deferred(control);
	} else {
		// Do not follow the control when toggling.
		callable_mp(this, &ProjectList::ensure_project_visible).call_deferred(index);
	}

	update_dock_menu();
}

void ProjectList::_on_explore_pressed(const String &p_path) {
	OS::get_singleton()->shell_show_in_file_manager(p_path, true);
}

void ProjectList::_open_menu(const Vector2 &p_at, Control *p_hb) {
	int clicked_index = p_hb->get_index();
	const Item &clicked_project = _projects[clicked_index];

	if (!project_context_menu) {
		project_context_menu = memnew(PopupMenu);
		project_context_menu->add_item(TTRC("Open in Editor"), MENU_EDIT);
		project_context_menu->add_item(TTRC("Open in Editor (Verbose Mode)"), MENU_EDIT_VERBOSE);
		project_context_menu->add_item(TTRC("Open in Editor (Recovery Mode)"), MENU_EDIT_RECOVERY);
		project_context_menu->add_item(TTRC("Run Project"), MENU_RUN);
		project_context_menu->add_separator();
#if !defined(ANDROID_ENABLED)
		project_context_menu->add_item(OS::get_singleton()->get_platform_string(OS::PLATFORM_STRING_FILE_MANAGER_OPEN), MENU_SHOW_IN_FILE_MANAGER);
#endif
		project_context_menu->add_item(TTRC("Copy Path"), MENU_COPY_PATH);
		project_context_menu->add_separator();
		project_context_menu->add_item(TTRC("Rename"), MENU_RENAME);
		project_context_menu->add_item(TTRC("Manage Tags"), MENU_MANAGE_TAGS);
		project_context_menu->add_item(TTRC("Duplicate"), MENU_DUPLICATE);
		project_context_menu->add_item(TTRC("Remove from Project List"), MENU_REMOVE);
		add_child(project_context_menu);
		project_context_menu->connect(SceneStringName(id_pressed), callable_mp(this, &ProjectList::_menu_option));
		_update_menu_icons();
	}
	clicked_project.control->grab_focus(true);

	for (int id : Vector<int>{
				 MENU_EDIT,
				 MENU_EDIT_VERBOSE,
				 MENU_EDIT_RECOVERY,
				 MENU_RUN,
#if !defined(ANDROID_ENABLED)
				 MENU_SHOW_IN_FILE_MANAGER,
#endif
				 MENU_RENAME,
				 MENU_MANAGE_TAGS,
				 MENU_DUPLICATE }) {
		project_context_menu->set_item_disabled(project_context_menu->get_item_index(id), clicked_project.missing);
	}

	project_context_menu->set_position(p_hb->get_screen_position() + p_at);
	project_context_menu->reset_size();
	project_context_menu->popup();
}

void ProjectList::_menu_option(int p_option) {
	emit_signal(SIGNAL_MENU_OPTION_SELECTED, p_option);
}

void ProjectList::_update_menu_icons() {
	project_context_menu->set_item_icon(project_context_menu->get_item_index(MENU_EDIT), get_editor_theme_icon("action_edit"));
	project_context_menu->set_item_icon(project_context_menu->get_item_index(MENU_EDIT_VERBOSE), get_editor_theme_icon("status_notification"));
	project_context_menu->set_item_icon(project_context_menu->get_item_index(MENU_EDIT_RECOVERY), get_editor_theme_icon("status_node_warning"));
	project_context_menu->set_item_icon(project_context_menu->get_item_index(MENU_RUN), get_editor_theme_icon("playback_play"));
#if !defined(ANDROID_ENABLED)
	project_context_menu->set_item_icon(project_context_menu->get_item_index(MENU_SHOW_IN_FILE_MANAGER), get_editor_theme_icon("action_load"));
#endif
	project_context_menu->set_item_icon(project_context_menu->get_item_index(MENU_COPY_PATH), get_editor_theme_icon("action_copy"));
	project_context_menu->set_item_icon(project_context_menu->get_item_index(MENU_RENAME), get_editor_theme_icon("action_rename"));
	project_context_menu->set_item_icon(project_context_menu->get_item_index(MENU_MANAGE_TAGS), get_editor_theme_icon("resource_script"));
	project_context_menu->set_item_icon(project_context_menu->get_item_index(MENU_DUPLICATE), get_editor_theme_icon("action_duplicate"));
	project_context_menu->set_item_icon(project_context_menu->get_item_index(MENU_REMOVE), get_editor_theme_icon("action_remove"));
}

// Project list selection.

void ProjectList::_clear_project_selection() {
	Vector<Item> previous_selected_items = get_selected_projects();
	_selected_project_paths.clear();

	for (int i = 0; i < previous_selected_items.size(); ++i) {
		previous_selected_items[i].control->set_selected(false);
	}
}

void ProjectList::_select_project_nocheck(int p_index, bool p_hide_focus) {
	Item &item = _projects.write[p_index];
	_selected_project_paths.insert(item.path);
	item.control->set_selected(true, p_hide_focus);
}

void ProjectList::_deselect_project_nocheck(int p_index) {
	Item &item = _projects.write[p_index];
	_selected_project_paths.erase(item.path);
	item.control->set_selected(false);
}

inline void _sort_project_range(int &a, int &b) {
	if (a > b) {
		int temp = a;
		a = b;
		b = temp;
	}
}

void ProjectList::_select_project_range(int p_begin, int p_end) {
	_clear_project_selection();

	_sort_project_range(p_begin, p_end);
	for (int i = p_begin; i <= p_end; ++i) {
		_select_project_nocheck(i);
	}
}

void ProjectList::select_project(int p_index, bool p_hide_focus) {
	// This method keeps only one project selected.
	_clear_project_selection();
	_select_project_nocheck(p_index, p_hide_focus);
}

void ProjectList::select_first_visible_project() {
	_clear_project_selection();

	for (int i = 0; i < _projects.size(); i++) {
		if (_projects[i].control->is_visible()) {
			_select_project_nocheck(i);
			break;
		}
	}
}

void ProjectList::deselect_all_visible_projects() {
	for (int i = 0; i < _projects.size(); i++) {
		if (_projects[i].control->is_visible()) {
			_deselect_project_nocheck(i);
		}
	}
}

void ProjectList::select_all_visible_projects() {
	for (int i = 0; i < _projects.size(); i++) {
		if (_projects[i].control->is_visible()) {
			_select_project_nocheck(i);
		}
	}
}

Vector<ProjectList::Item> ProjectList::get_selected_projects() const {
	Vector<Item> items;
	if (_selected_project_paths.is_empty()) {
		return items;
	}
	items.resize(_selected_project_paths.size());
	int j = 0;
	for (int i = 0; i < _projects.size(); ++i) {
		const Item &item = _projects[i];
		if (_selected_project_paths.has(item.path)) {
			items.write[j++] = item;
		}
	}
	ERR_FAIL_COND_V(j != items.size(), items);
	return items;
}

const HashSet<String> &ProjectList::get_selected_project_keys() const {
	// Faster if that's all you need
	return _selected_project_paths;
}

int ProjectList::get_single_selected_index() const {
	if (_selected_project_paths.is_empty()) {
		// Default selection
		return 0;
	}
	String key;
	if (_selected_project_paths.size() == 1) {
		// Only one selected
		key = *_selected_project_paths.begin();
	} else {
		// Multiple selected, consider the last clicked one as "main"
		key = _last_clicked;
	}
	for (int i = 0; i < _projects.size(); ++i) {
		if (_projects[i].path == key) {
			return i;
		}
	}
	return 0;
}

void ProjectList::erase_selected_projects(bool p_delete_project_contents) {
	if (_selected_project_paths.is_empty()) {
		return;
	}

	for (int i = 0; i < _projects.size(); ++i) {
		Item &item = _projects.write[i];
		if (_selected_project_paths.has(item.path) && item.control->is_visible()) {
			_config.erase_section(item.path);

			// Comment out for now until we have a better warning system to
			// ensure users delete their project only.
			//if (p_delete_project_contents) {
			//	OS::get_singleton()->move_to_trash(item.path);
			//}

			memdelete(item.control);
			_projects.remove_at(i);
			--i;
		}
	}

	save_config();
	_selected_project_paths.clear();
	_last_clicked = "";

	update_dock_menu();
}

// Resize project titles.

void ProjectList::resize_project_titles() {
	if (window_size_cache == 0) {
		return;
	}
	const int window_size = get_window()->get_size().x;
	const int difference = window_size - window_size_cache;
	window_size_cache = window_size;

	title_size_cache += difference;
	for (Item &item : _projects) {
		item.control->resize_project_title(title_size_cache);
	}
}

// Missing projects.

bool ProjectList::is_any_project_missing() const {
	for (int i = 0; i < _projects.size(); ++i) {
		if (_projects[i].missing) {
			return true;
		}
	}
	return false;
}

void ProjectList::erase_missing_projects() {
	if (_projects.is_empty()) {
		return;
	}

	int deleted_count = 0;
	int remaining_count = 0;

	for (int i = 0; i < _projects.size(); ++i) {
		const Item &item = _projects[i];

		if (item.missing) {
			_remove_project(i, true);
			--i;
			++deleted_count;

		} else {
			++remaining_count;
		}
	}

	print_line("Removed " + itos(deleted_count) + " projects from the list, remaining " + itos(remaining_count) + " projects");
	save_config();
}

// Project list sorting and filtering.

void ProjectList::set_search_term(String p_search_term) {
	_search_term = p_search_term;
}

void ProjectList::add_search_tag(const String &p_tag) {
	const String tag_string = "tag:" + p_tag;

	int exists = _search_term.find(tag_string);
	if (exists > -1) {
		_search_term = _search_term.erase(exists, tag_string.length() + 1);
	} else if (_search_term.is_empty() || _search_term.ends_with(" ")) {
		_search_term += tag_string;
	} else {
		_search_term += " " + tag_string;
	}
	ProjectManager::get_singleton()->get_search_box()->set_text(_search_term);

	sort_projects();
}

void ProjectList::set_order_option(int p_option, bool p_save) {
	FilterOption selected = (FilterOption)p_option;
	if (p_save) {
		EditorSettings::get_singleton()->set("project_manager/sorting_order", p_option);
		EditorSettings::get_singleton()->save();
	}
	_order_option = selected;

	sort_projects();
}

// Global menu integration.

void ProjectList::update_dock_menu() {
	if (!NativeMenu::get_singleton()->has_feature(NativeMenu::FEATURE_GLOBAL_MENU)) {
		return;
	}
	RID dock_rid = NativeMenu::get_singleton()->get_system_menu(NativeMenu::DOCK_MENU_ID);
	NativeMenu::get_singleton()->clear(dock_rid);

	int favs_added = 0;
	int total_added = 0;
	for (int i = 0; i < _projects.size(); ++i) {
		if (!_projects[i].grayed && !_projects[i].missing) {
			if (_projects[i].favorite) {
				favs_added++;
			} else {
				if (favs_added != 0) {
					NativeMenu::get_singleton()->add_separator(dock_rid);
				}
				favs_added = 0;
			}
			NativeMenu::get_singleton()->add_item(dock_rid, _projects[i].project_name + " ( " + _projects[i].path + " )", callable_mp(this, &ProjectList::_global_menu_open_project), Callable(), i);
			total_added++;
		}
	}
	if (total_added != 0) {
		NativeMenu::get_singleton()->add_separator(dock_rid);
	}
	NativeMenu::get_singleton()->add_item(dock_rid, TTR("New Window"), callable_mp(this, &ProjectList::_global_menu_new_window));
}

void ProjectList::_global_menu_new_window(const Variant &p_tag) {
	List<String> args;
	args.push_back("-p");
	OS::get_singleton()->create_instance(args);
}

void ProjectList::_global_menu_open_project(const Variant &p_tag) {
	int idx = (int)p_tag;

	if (idx >= 0 && idx < _projects.size()) {
		String conf = _projects[idx].path.path_join("project.godot");
		List<String> args;
		args.push_back(conf);
		OS::get_singleton()->create_instance(args);
	}
}

// Object methods.

void ProjectList::_bind_methods() {
	ADD_SIGNAL(MethodInfo(SIGNAL_LIST_CHANGED));
	ADD_SIGNAL(MethodInfo(SIGNAL_SELECTION_CHANGED));
	ADD_SIGNAL(MethodInfo(SIGNAL_PROJECT_ASK_OPEN));
	ADD_SIGNAL(MethodInfo(SIGNAL_MENU_OPTION_SELECTED));
}

ProjectList::ProjectList() {
	set_follow_focus(true);

	project_list_vbox = memnew(VBoxContainer);
	project_list_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(project_list_vbox);

	_config_path = EditorPaths::get_singleton()->get_data_dir().path_join("projects.cfg");
}
