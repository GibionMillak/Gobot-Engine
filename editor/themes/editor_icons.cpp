/**************************************************************************/
/*  editor_icons.cpp                                                      */
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

#include "editor_icons.h"

#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "core/variant/dictionary.h"
#include "editor/content/assets/icons/editor_icons.gen.h"
#include "editor/editor_string_names.h"
#include "editor/themes/editor_color_map.h"
#include "editor/themes/editor_scale.h"
#include "scene/resources/dpi_texture.h"
#include "scene/resources/theme.h"

#include "modules/svg/image_loader_svg.h"

static HashMap<StringName, StringName> &get_type_icon_cache() {
	static HashMap<StringName, StringName> type_icons;
	return type_icons;
}

void editor_initialize_type_icons() {
	HashMap<StringName, StringName> &type_icons = get_type_icon_cache();
	for (int i = 0; i < EDITOR_TYPE_ICONS_COUNT; i++) {
		const EditorTypeIcon &entry = EDITOR_TYPE_ICONS[i];
		type_icons.insert(entry.type_name, entry.icon_name);
	}
}

void editor_finalize_type_icons() {
	get_type_icon_cache().clear();
}

StringName editor_get_type_icon_name(const StringName &p_type) {
	const StringName *icon_name = get_type_icon_cache().getptr(p_type);
	return icon_name ? *icon_name : StringName();
}

void editor_configure_icons(bool p_dark_icon_and_font) {
	if (p_dark_icon_and_font) {
		ImageLoaderSVG::set_forced_color_map(HashMap<Color, Color>());
	} else {
		ImageLoaderSVG::set_forced_color_map(EditorColorMap::get_color_conversion_map());
	}
}

struct IconRenderingOptions {
	float saturation;
	const Dictionary &color_conversion_map;
	const Dictionary &accent_color_map;
	const HashSet<StringName> &accent_color_icons;
	const HashSet<StringName> &conversion_exceptions;
	const HashSet<StringName> &saturation_exceptions;
};

static Ref<DPITexture> generate_editor_icon(int p_index, float p_scale, const IconRenderingOptions &p_options) {
	const StringName name = EDITOR_ICONS_NAMES[p_index];
	if (p_options.accent_color_icons.has(name)) {
		return DPITexture::create_from_string(EDITOR_ICONS_SOURCES[p_index], p_scale, 1.0, p_options.accent_color_map);
	}
	const float saturation = p_options.saturation_exceptions.has(name) ? 1.0 : p_options.saturation;
	const Dictionary colors = p_options.conversion_exceptions.has(name) ? Dictionary() : p_options.color_conversion_map;
	return DPITexture::create_from_string(EDITOR_ICONS_SOURCES[p_index], p_scale, saturation, colors);
}

static float get_gizmo_handle_scale(const String &p_gizmo_handle_name, float p_gizmo_handle_scale) {
	if (p_gizmo_handle_scale > 1.0f) {
		// The names of the icons that require additional scaling.
		static HashSet<StringName> gizmo_to_scale;
		if (gizmo_to_scale.is_empty()) {
			gizmo_to_scale.insert("viewport_editor_handle");
			gizmo_to_scale.insert("viewport_editor_handle_add");
			gizmo_to_scale.insert("viewport_editor_handle_disabled");
			gizmo_to_scale.insert("viewport_editor_curve_handle");
			gizmo_to_scale.insert("viewport_editor_path_sharp_handle");
			gizmo_to_scale.insert("viewport_editor_path_smooth_handle");
			gizmo_to_scale.insert("viewport_editor_control_anchor");
		}

		if (gizmo_to_scale.has(p_gizmo_handle_name)) {
			return EDSCALE * p_gizmo_handle_scale;
		}
	}

	return EDSCALE;
}

void editor_register_icons(const Ref<Theme> &p_theme, bool p_dark_theme, float p_icon_saturation, int p_thumb_size, float p_gizmo_handle_scale) {
	// Before we register the icons, we adjust their colors and saturation.
	// Most icons follow the standard rules for color conversion to follow the editor
	// theme's polarity (dark/light). We also adjust the saturation for most icons,
	// following the editor setting.
	// Some icons are excluded from this conversion, and instead use the configured
	// accent color to replace their innate accent color to match the editor theme.
	// And then some icons are completely excluded from the conversion.

	// Standard color conversion map.
	Dictionary color_conversion_map_light;
	Dictionary color_conversion_map_dark;
	// Icons by default are set up for the dark theme, so if the theme is light,
	// we apply the dark-to-light color conversion map.
	for (KeyValue<Color, Color> &E : EditorColorMap::get_color_conversion_map()) {
		color_conversion_map_light[E.key] = E.value;
	}
	// These colors should be converted even if we are using a dark theme.
	const Color error_color = p_theme->get_color(SNAME("error_color"), EditorStringName(Editor));
	const Color success_color = p_theme->get_color(SNAME("success_color"), EditorStringName(Editor));
	const Color warning_color = p_theme->get_color(SNAME("warning_color"), EditorStringName(Editor));
	color_conversion_map_dark[Color::html("#ff5f5f")] = error_color;
	color_conversion_map_dark[Color::html("#5fff97")] = success_color;
	color_conversion_map_dark[Color::html("#ffdd65")] = warning_color;
	color_conversion_map_light[Color::html("#ff5f5f")] = error_color;
	color_conversion_map_light[Color::html("#5fff97")] = success_color;
	color_conversion_map_light[Color::html("#ffdd65")] = warning_color;

	Dictionary color_conversion_map = p_dark_theme ? color_conversion_map_dark : color_conversion_map_light;

	// The names of the icons to exclude from the standard color conversion.
	HashSet<StringName> conversion_exceptions(EditorColorMap::get_color_conversion_exceptions());

	// The names of the icons to exclude when adjusting for saturation.
	HashSet<StringName> saturation_exceptions;
	saturation_exceptions.insert("gui_project_thumbnail_default");
	saturation_exceptions.insert("gui_project_thumbnail_loading");
	saturation_exceptions.insert("gui_project_thumbnail_missing");
	saturation_exceptions.insert("branding_logo_solid");
	saturation_exceptions.insert("file_project");
	saturation_exceptions.insert("branding_logo");

	// Accent color conversion map.
	// It is used on some icons (checkbox, radio, toggle, etc.), regardless of the dark
	// or light mode.
	Dictionary accent_color_map;
	HashSet<StringName> accent_color_icons;

	const Color accent_color = p_theme->get_color(SNAME("accent_color"), EditorStringName(Editor));
	accent_color_map[Color::html("699ce8")] = accent_color;
	if (accent_color.get_luminance() > 0.75) {
		accent_color_map[Color::html("ffffff")] = Color(0.2, 0.2, 0.2);
	}

	accent_color_icons.insert("gui_checked");
	accent_color_icons.insert("gui_radio_checked");
	accent_color_icons.insert("gui_indeterminate");
	accent_color_icons.insert("gui_toggle_on");
	accent_color_icons.insert("status_verified");

	const IconRenderingOptions rendering_options = {
		p_icon_saturation,
		color_conversion_map,
		accent_color_map,
		accent_color_icons,
		conversion_exceptions,
		saturation_exceptions,
	};
	for (int i = 0; i < EDITOR_ICONS_NAMES_COUNT; i++) {
		const StringName name = EDITOR_ICONS_NAMES[i];
		const float scale = get_gizmo_handle_scale(name, p_gizmo_handle_scale);
		p_theme->set_icon(name, EditorStringName(EditorIcons), generate_editor_icon(i, scale, rendering_options));
	}

	const bool big_thumbnails = p_thumb_size >= 64;
	const int count = big_thumbnails ? EDITOR_BG_THUMBS_INDICES_COUNT : EDITOR_MD_THUMBS_INDICES_COUNT;
	const int *indices = big_thumbnails ? EDITOR_BG_THUMBS_INDICES : EDITOR_MD_THUMBS_INDICES;
	const float scale = float(p_thumb_size) / (big_thumbnails ? 64.0 : 32.0) * EDSCALE;
	for (int i = 0; i < count; i++) {
		const int index = indices[i];
		p_theme->set_icon(EDITOR_ICONS_NAMES[index], EditorStringName(EditorIcons), generate_editor_icon(index, scale, rendering_options));
	}
}

void editor_copy_icons(const Ref<Theme> &p_theme, const Ref<Theme> &p_old_theme) {
	for (int i = 0; i < EDITOR_ICONS_NAMES_COUNT; i++) {
		p_theme->set_icon(EDITOR_ICONS_NAMES[i], EditorStringName(EditorIcons), p_old_theme->get_icon(EDITOR_ICONS_NAMES[i], EditorStringName(EditorIcons)));
	}
}

String get_default_project_thumbnail() {
	return String(EDITOR_ICON_GUI_PROJECT_THUMBNAIL_DEFAULT);
}
