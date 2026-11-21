/**************************************************************************/
/*  editor_theme_manager.cpp                                              */
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

#include "editor_theme_manager.h"

#include "core/error/error_macros.h"
#include "core/io/config_file.h"
#include "core/io/resource_loader.h"
#include "core/math/color.h"
#include "core/math/math_funcs.h"
#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_paths.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_color_map.h"
#include "editor/themes/editor_fonts.h"
#include "editor/themes/editor_icons.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_sounds.h"
#include "scene/resources/style_box.h"
#include "scene/theme/theme_db.h"
#include "servers/display/display_server.h"

bool EditorThemeManager::_definition_refresh_pending = false;
Ref<Theme> EditorThemeManager::_theme_definition;
std::atomic<bool> EditorThemeManager::_active_dark_theme{ true };
std::atomic<float> EditorThemeManager::_active_icon_saturation{ 1.0f };
int EditorThemeManager::_benchmark_run = 0;


Ref<EditorTheme> EditorThemeManager::generate_theme(const Ref<EditorTheme> &p_old_theme) {
	OS::get_singleton()->benchmark_begin_measure(_get_benchmark_key(), "Generate Theme");

	Ref<EditorTheme> theme = _create_base_theme(p_old_theme);

	ERR_FAIL_COND_V(theme.is_null(), p_old_theme);

	OS::get_singleton()->benchmark_end_measure(_get_benchmark_key(), "Generate Theme");
	_benchmark_run++;

	return theme;
}

bool EditorThemeManager::is_generated_theme_outdated() {
	// This list includes settings used by files in the editor/themes folder.
	// Note that the editor scale is purposefully omitted because it cannot be changed
	// without a restart, so there is no point regenerating the theme.

	if (_outdated_cache_dirty) {
		// TODO: We can use this information more intelligently to do partial theme updates and speed things up.
		_outdated_cache = EditorSettings::get_singleton()->check_changed_settings_in_group("interface/theme") ||
				EditorSettings::get_singleton()->check_changed_settings_in_group("interface/editor/fonts") ||
				EditorSettings::get_singleton()->check_changed_settings_in_group("interface/editor/appearance/max_sticky_tree_items") ||
				EditorSettings::get_singleton()->check_changed_settings_in_group("interface/editor/timers/dragging_hover_wait_seconds") ||
				EditorSettings::get_singleton()->check_changed_settings_in_group("interface/touchscreen/enable_touch_optimizations") ||
				EditorSettings::get_singleton()->check_changed_settings_in_group("interface/touchscreen/scale_gizmo_handles") ||
				EditorSettings::get_singleton()->check_changed_settings_in_group("editors/visual_editors") ||
				EditorSettings::get_singleton()->check_changed_settings_in_group("text_editor/theme") ||
				EditorSettings::get_singleton()->check_changed_settings_in_group("text_editor/help/help") ||
				EditorSettings::get_singleton()->check_changed_settings_in_group("docks/property_editor/subresource_hue_tint") ||
				EditorSettings::get_singleton()->check_changed_settings_in_group("filesystem/file_dialog/thumbnail_size") ||
				EditorSettings::get_singleton()->check_changed_settings_in_group("run/output/font_size");

		// The outdated flag is relevant at the moment of changing editor settings.
		callable_mp_static(&EditorThemeManager::_reset_dirty_flag).call_deferred();
		_outdated_cache_dirty = false;
	}

	return _outdated_cache;
}

void EditorThemeManager::set_theme_outdated() {
	callable_mp_static(&EditorThemeManager::_reset_dirty_flag).call_deferred();
	_outdated_cache_dirty = false;
	_outdated_cache = true;
}

Ref<Theme> EditorThemeManager::get_default_theme() {
	return ResourceLoader::load(ThemeDB::DEFAULT_THEME_PATH, "Theme");
}

Vector<Ref<Resource>> EditorThemeManager::get_builtin_themes() {
	Vector<Ref<Resource>> themes;
	const char *const PATHS[] = { ThemeDB::DEFAULT_THEME_PATH, "res://.content/resources/themes/dark.tres", "res://.content/resources/themes/light.tres" };
	for (const char *path : PATHS) {
		const Ref<Theme> definition = ResourceLoader::load(path, "Theme");
		ERR_CONTINUE(definition.is_null());
		themes.push_back(definition);
	}
	return themes;
}

float EditorThemeManager::get_icon_saturation() {
	return _active_icon_saturation.load(std::memory_order_relaxed);
}

bool EditorThemeManager::is_dark_theme() {
	return _active_dark_theme.load(std::memory_order_relaxed);
}

bool EditorThemeManager::is_dark_icon_and_font() {
	const int icon_font_color_setting = EDITOR_GET("interface/theme/icon_and_font_color");
	return icon_font_color_setting == AUTO_COLOR ? is_dark_theme() : icon_font_color_setting == LIGHT_COLOR;
}

void EditorThemeManager::initialize() {
	EditorColorMap::create();
}

void EditorThemeManager::finalize() {
	if (_theme_definition.is_valid()) {
		_theme_definition->disconnect_changed(callable_mp_static(&EditorThemeManager::_theme_definition_changed));
		_theme_definition.unref();
	}
	_definition_refresh_pending = false;
	EditorColorMap::finish();
}

String EditorThemeManager::_get_benchmark_key() {
	if (_benchmark_run == 0) {
		return "EditorTheme (Startup)";
	}

	return vformat("EditorTheme (Run %d)", _benchmark_run);
}

Ref<EditorTheme> EditorThemeManager::_create_base_theme(const Ref<EditorTheme> &p_old_theme) {
	const Ref<Theme> definition = _get_theme_definition();
	ERR_FAIL_COND_V(definition.is_null(), Ref<EditorTheme>());
	Ref<EditorTheme> theme = memnew(EditorTheme);
	theme->merge_with(definition);
	theme->set_default_base_scale(EDSCALE);
	if (EDITOR_GET("interface/theme/use_system_accent_color")) {
		const Color accent = DisplayServer::get_singleton()->get_accent_color();
		if (accent != Color(0, 0, 0, 0)) {
			theme->set_style_parameter("default_accent_color", accent);
		}
	}
	// Resolution scales dimensions; keep focus outlines at least their authored pixel width.
	const double focus_width = definition->get_style_parameter("default_focus_border_width");
	theme->set_style_parameter("default_focus_border_width", focus_width * MAX(1.0f, EDSCALE) / EDSCALE);
	const Color base_color = theme->get_style_parameter("default_base_color");
	const bool dark = base_color.get_luminance() < 0.5;
	const int icon_mode = EDITOR_GET("interface/theme/icon_and_font_color");
	const bool dark_icons = icon_mode == AUTO_COLOR ? dark : icon_mode == LIGHT_COLOR;
	if (icon_mode != AUTO_COLOR) {
		theme->set_color("mono_color_font", "Editor", dark_icons ? Color(1, 1, 1) : Color(0, 0, 0));
	}
	const float saturation = theme->get_style_parameter("default_icon_saturation");
	const int thumb_size = EDITOR_GET("filesystem/file_dialog/thumbnail_size");
	const float gizmo_scale = EDITOR_GET("interface/touchscreen/scale_gizmo_handles");
	uint32_t icon_hash = hash_murmur3_one_float(EDSCALE);
	const Color accent_color = theme->get_style_parameter("default_accent_color");
	icon_hash = hash_murmur3_one_32(accent_color.to_rgba32(), icon_hash);
	icon_hash = hash_murmur3_one_float(saturation, icon_hash);
	icon_hash = hash_murmur3_one_32(thumb_size, icon_hash);
	icon_hash = hash_murmur3_one_float(gizmo_scale, icon_hash);
	icon_hash = hash_murmur3_one_32(dark_icons, icon_hash);
	// Icon conversion also consumes the theme's semantic colors.
	const char *const ICON_COLORS[] = { "error_color", "success_color", "warning_color" };
	for (const char *name : ICON_COLORS) {
		icon_hash = hash_murmur3_one_32(theme->get_color(name, EditorStringName(Editor)).to_rgba32(), icon_hash);
	}
	theme->set_generated_icons_hash(icon_hash);
	editor_configure_icons(dark_icons);
	if (p_old_theme.is_valid() && p_old_theme->get_generated_icons_hash() == icon_hash) {
		editor_copy_icons(theme, p_old_theme);
	} else {
		editor_register_icons(theme, dark_icons, saturation, thumb_size, gizmo_scale);
	}
	editor_register_fonts(theme);
	EditorSounds::populate_theme(theme);
	_apply_preference_defaults(theme);
	_apply_editor_preferences(theme);
	_active_dark_theme.store(dark, std::memory_order_relaxed);
	_active_icon_saturation.store(saturation, std::memory_order_relaxed);
	return theme;
}

void EditorThemeManager::_apply_editor_preferences(const Ref<Theme> &p_theme) {
	EditorSettings *settings = EditorSettings::get_singleton();
	struct FontSizePreference {
		const char *setting;
		const char *item;
		int offset;
	};
	const FontSizePreference FONT_SIZES[] = {
		{ "interface/editor/fonts/code_font_size", "source_size", 0 },
		{ "interface/editor/fonts/code_font_size", "expression_size", -1 },
		{ "text_editor/help/help_font_size", "doc_size", 0 },
		{ "text_editor/help/help_source_font_size", "doc_source_size", 0 },
		{ "text_editor/help/help_source_font_size", "doc_keyboard_size", -1 },
		{ "text_editor/help/help_title_font_size", "doc_title_size", 0 },
		{ "run/output/font_size", "output_source_size", 0 },
	};
	for (const FontSizePreference &preference : FONT_SIZES) {
		const int font_size = settings->get_setting(preference.setting);
		p_theme->set_font_size(preference.item, EditorStringName(Editor), int((font_size + preference.offset) * EDSCALE));
	}

	List<StringName> syntax_colors;
	p_theme->get_color_list(EditorStringName(Editor), &syntax_colors);
	for (const StringName &item : syntax_colors) {
		const String setting = _get_text_editor_color_setting(item);
		if (!setting.is_empty() && settings->has_setting(setting)) {
			p_theme->set_color(item, EditorStringName(Editor), settings->get_setting(setting));
		}
	}
	p_theme->set_constant("line_spacing", "CodeEdit", EDITOR_GET("text_editor/appearance/whitespace/line_spacing"));
	p_theme->set_constant("thumb_size", EditorStringName(Editor), EDITOR_GET("filesystem/file_dialog/thumbnail_size"));
	p_theme->set_constant("thumbnail_size", "EditorFileDialog", EDITOR_GET("filesystem/file_dialog/thumbnail_size"));
	p_theme->set_constant("gizmo_handle_scale", EditorStringName(Editor), int(float(EDITOR_GET("interface/touchscreen/scale_gizmo_handles"))));
	p_theme->set_constant("scroll_max_sticky_items", "Tree", EDITOR_GET("interface/editor/appearance/max_sticky_tree_items"));
	const int dragging_wait_msec = int(float(EDITOR_GET("interface/editor/timers/dragging_hover_wait_seconds")) * 1000);
	p_theme->set_constant("hover_switch_wait_msec", "TabBar", dragging_wait_msec);
	p_theme->set_constant("dragging_unfold_wait_msec", "Tree", dragging_wait_msec);
	if (OS::get_singleton()->has_feature("android")) {
		p_theme->set_constant("click_margin", "BaseButton", int(EDSCALE_RND(4)));
	}

	const Color mono_color = p_theme->get_color("mono_color", "Editor");
	const float relationship_opacity = EDITOR_GET("interface/theme/relationship_line_opacity");
	p_theme->set_color("relationship_line_color", "Tree", mono_color * Color(1, 1, 1, relationship_opacity));
	p_theme->set_color("children_hl_line_color", "Tree", mono_color * Color(1, 1, 1, CLAMP(relationship_opacity + 0.25f, 0.0f, 1.0f)));
	p_theme->set_color("parent_hl_line_color", "Tree", mono_color * Color(1, 1, 1, CLAMP(relationship_opacity + 0.45f, 0.0f, 1.0f)));
	const bool relationship_lines = p_theme->get_constant("draw_relationship_lines", EditorStringName(Editor)) != 0 && relationship_opacity >= 0.01f;
	p_theme->set_constant("draw_relationship_lines", "Tree", relationship_lines);
	p_theme->set_constant("draw_guides", "Tree", !relationship_lines);
	const int grid_pattern = EDITOR_GET("editors/visual_editors/grid_pattern");
	p_theme->set_color("grid_major", "GraphEdit", mono_color * Color(1, 1, 1, grid_pattern == 0 ? 0.1f : 0.07f));
	p_theme->set_color("grid_minor", "GraphEdit", mono_color * Color(1, 1, 1, grid_pattern == 0 ? 0.05f : 0.07f));

	const Color accent_color = p_theme->get_style_parameter("default_accent_color");
	const float subresource_tint = EDITOR_GET("docks/property_editor/subresource_hue_tint");
	for (int i = 0; i < 8; i++) {
		const Color rotated = Color::from_hsv(Math::fmod(accent_color.get_h() + i / 8.0f, 1.0f), accent_color.get_s(), accent_color.get_v());
		p_theme->set_color("subresource_color_" + itos(i), "EditorInspector", accent_color.lerp(rotated, subresource_tint));
	}

	if (EDITOR_GET("interface/touchscreen/enable_touch_optimizations")) {
		p_theme->set_constant("search_bar_separation", "PopupMenu", int(12 * EDSCALE));
		p_theme->set_constant("v_separation", "PopupMenu", int(12 * EDSCALE));
		const char *const SCROLLBARS[] = { "HScrollBar", "VScrollBar" };
		for (int i = 0; i < 2; i++) {
			const StringName type = SCROLLBARS[i];
			if (!p_theme->has_stylebox("scroll", type)) {
				continue;
			}
			const Ref<StyleBox> definition = p_theme->get_stylebox_definition("scroll", type);
			ERR_CONTINUE(definition.is_null());
			const Ref<StyleBox> scroll = definition->duplicate();
			ERR_CONTINUE(scroll.is_null());
			scroll->set_background_color(p_theme->get_color("separator_color", "Editor"));
			scroll->set_expand_margin(i == 0 ? SIDE_LEFT : SIDE_TOP, 1);
			scroll->set_expand_margin(i == 0 ? SIDE_RIGHT : SIDE_BOTTOM, 1);
			scroll->set_minimum_size(i == 0 ? Vector2(0, 50) : Vector2(50, 0));
			scroll->set_padding_all(0);
			scroll->set_texture(Ref<Texture2D>());
			scroll->set_texture_slice_all(0);
			p_theme->set_stylebox("scroll", type, scroll);
		}
	}
}

String EditorThemeManager::_get_text_editor_color_setting(const StringName &p_name) {
	const String item_name = p_name;
	if (!item_name.begins_with("text_editor_")) {
		return String();
	}
	String setting = item_name.trim_prefix("text_editor_");
	const char *const GROUPS[] = { "comment_markers", "gdscript" };
	for (const char *group : GROUPS) {
		const String prefix = String(group) + "_";
		if (setting.begins_with(prefix)) {
			setting = String(group) + "/" + setting.trim_prefix(prefix);
			break;
		}
	}
	return "text_editor/theme/highlighting/" + setting;
}

void EditorThemeManager::_apply_preference_defaults(const Ref<Theme> &p_theme) {
	EditorSettings *settings = EditorSettings::get_singleton();
	const String syntax = EDITOR_GET("text_editor/theme/color_theme");
	if (syntax == "Default" || syntax == "Custom") {
		List<StringName> names;
		p_theme->get_color_list(EditorStringName(Editor), &names);
		for (const StringName &name : names) {
			const String key = _get_text_editor_color_setting(name);
			if (key.is_empty()) {
				continue;
			}
			const Color color = p_theme->get_color(name, EditorStringName(Editor));
			settings->set_initial_value(key, color);
			if (syntax == "Default") {
				settings->set_manually(key, color);
			}
		}
	} else if (syntax == "Godot 2") {
		for (const KeyValue<StringName, Color> &entry : EditorSettings::get_godot2_text_editor_theme()) {
			settings->set_initial_value(entry.key, entry.value);
			settings->set_manually(entry.key, entry.value);
		}
	} else {
		_load_text_editor_theme();
	}
	const String shader = EDITOR_GET("editors/visual_editors/color_theme");
	if (shader == "Default" || shader == "Legacy") {
		const String prefix = shader == "Default" ? "visual_shader_default_" : "visual_shader_legacy_";
		List<StringName> names;
		p_theme->get_color_list(EditorStringName(Editor), &names);
		for (const StringName &name : names) {
			const String item_name = name;
			if (!item_name.begins_with(prefix)) {
				continue;
			}
			const String item = item_name.trim_prefix(prefix);
			String setting;
			if (item.begins_with("category_colors_")) {
				setting = "editors/visual_editors/category_colors/" + item.trim_prefix("category_colors_");
			} else if (item.begins_with("connection_colors_")) {
				setting = "editors/visual_editors/connection_colors/" + item.trim_prefix("connection_colors_");
			}
			if (!setting.is_empty()) {
				settings->set_initial_value(setting, p_theme->get_color(name, EditorStringName(Editor)), true);
			}
		}
	}
}

void EditorThemeManager::_load_text_editor_theme() {
	EditorSettings *settings = EditorSettings::get_singleton();
	const String theme_name = settings->get_setting("text_editor/theme/color_theme");

	ERR_FAIL_COND(EditorSettings::is_default_text_editor_theme(theme_name.get_file().to_lower()));

	const String theme_path = EditorPaths::get_singleton()->get_text_editor_themes_dir().path_join(theme_name + ".tet");

	Ref<ConfigFile> cf;
	cf.instantiate();
	Error err = cf->load(theme_path);
	ERR_FAIL_COND_MSG(err != OK, vformat("Failed to load text editor theme file '%s': %s", theme_name, error_names[err]));

	const PackedStringArray keys = cf->get_section_keys("color_theme");

	for (const String &key : keys) {
		const String setting_key = "text_editor/theme/highlighting/" + key;
		// Don't load if it's not an actual setting, or if it isn't a color setting.
		if (!settings->has_setting(setting_key) || !key.contains("color")) {
			continue;
		}
		const String val = cf->get_value("color_theme", key);
		// Make sure it is actually a color.
		if (val.is_valid_html_color()) {
			const Color color_value = Color::html(val);
			// Change manually to prevent settings_changed spam.
			settings->set_initial_value(setting_key, color_value);
			settings->set_manually(setting_key, color_value);
		}
	}
	// If it doesn't load a setting just use what is currently loaded.
}

bool EditorThemeManager::_is_complete_theme_definition(const Ref<Theme> &p_theme) {
	if (p_theme.is_null()) {
		return false;
	}
	ERR_FAIL_COND_V_MSG(!p_theme->has_default_font_family() || !p_theme->has_default_font_size(), false, vformat("Editor theme '%s' must define its default font and size.", p_theme->get_path()));
	const char *const PARAMETERS[] = { "default_base_color", "default_accent_color", "default_contrast", "default_icon_saturation", "default_padding", "default_border_width", "default_border_radius", "default_focus_border_width" };
	for (const char *name : PARAMETERS) {
		ERR_FAIL_COND_V_MSG(!p_theme->has_style_parameter(name), false, vformat("Editor theme '%s' is missing parameter '%s'.", p_theme->get_path(), name));
	}
	const char *const CONTROL_TYPES[] = { "Button", "Label", "CodeEdit" };
	for (const char *type : CONTROL_TYPES) {
		ERR_FAIL_COND_V_MSG(!p_theme->has_stylebox("normal", type), false, vformat("Editor theme '%s' is missing %s/normal.", p_theme->get_path(), type));
	}
	return true;
}

Ref<Theme> EditorThemeManager::_get_theme_definition() {
	const Ref<Theme> default_theme = get_default_theme();
	ERR_FAIL_COND_V_MSG(default_theme.is_null(), Ref<Theme>(), "The bundled Default editor theme is missing or invalid.");
	const Variant selected = EDITOR_GET("interface/theme/theme");
	Ref<Theme> definition;
	if (selected.get_type() == Variant::OBJECT) {
		definition = selected;
	}
	const bool follow_system = EDITOR_GET("interface/theme/follow_system_theme");
	if (follow_system) {
		DisplayServer *display = DisplayServer::get_singleton();
		const Color system_base = display->get_base_color();
		const bool dark = system_base != Color(0, 0, 0, 0) ? system_base.get_luminance() < 0.5 : display->is_dark_mode_supported() && display->is_dark_mode();
		definition = ResourceLoader::load(dark ? "res://.content/resources/themes/dark.tres" : "res://.content/resources/themes/light.tres", "Theme");
	}
	if (definition.is_null()) {
		definition = default_theme;
	}
	if (!_is_complete_theme_definition(definition)) {
		definition = default_theme;
	}
	if (_theme_definition != definition) {
		const Callable changed = callable_mp_static(&EditorThemeManager::_theme_definition_changed);
		if (_theme_definition.is_valid()) {
			_theme_definition->disconnect_changed(changed);
		}
		_theme_definition = definition;
		_theme_definition->connect_changed(changed, Object::CONNECT_DEFERRED);
	}
	return definition;
}

void EditorThemeManager::_theme_definition_changed() {
	if (!_definition_refresh_pending) {
		_definition_refresh_pending = true;
		callable_mp_static(&EditorThemeManager::_refresh_theme_definition).call_deferred();
	}
}

void EditorThemeManager::_refresh_theme_definition() {
	_definition_refresh_pending = false;
	set_theme_outdated();
	if (EditorSettings::get_singleton()) {
		EditorSettings::get_singleton()->mark_setting_changed("interface/theme/theme");
		EditorSettings::get_singleton()->notify_changes();
	}
}

void EditorThemeManager::_reset_dirty_flag() {
	_outdated_cache_dirty = true;
}
