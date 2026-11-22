/**************************************************************************/
/*  quick_open_result_item_container.cpp                                  */
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

#include "quick_open_result_item_container.h"

#include "editor/content/editor_content_resources.h"
#include "editor/editor_string_names.h"
#include "editor/gui/data/quick_open_result_candidate.h"
#include "editor/gui/labels/highlighted_label.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/control.h"
#include "scene/gui/texture_rect.h"
#include "scene/scene_string_names.h"

QuickOpenResultItemContainer::QuickOpenResultItemContainer(EditorQuickOpenDialog::DisplayMode p_display_mode) :
		_display_mode(p_display_mode) {
	add_theme_constant_override("margin_bottom", 4 * EDSCALE);
	add_theme_constant_override("margin_left", 4 * EDSCALE);
	add_theme_constant_override("margin_right", 4 * EDSCALE);
	add_theme_constant_override("margin_top", 4 * EDSCALE);
	set_focus_mode(FocusMode::FOCUS_NONE);
	_set_enabled(false);

	const bool grid = _display_mode == EditorQuickOpenDialog::DisplayMode::GRID;

	BoxContainer *content_box = memnew(BoxContainer(grid));
	content_box->set_h_size_flags(Control::SIZE_FILL);
	content_box->set_v_size_flags(grid ? Control::SIZE_EXPAND_FILL : Control::SIZE_FILL);
	add_child(content_box);

	{
		_thumbnail = memnew(TextureRect);
		_thumbnail->set_custom_minimum_size(Size2(36, 36) * EDSCALE);
		_thumbnail->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
		_thumbnail->set_h_size_flags(Control::SIZE_SHRINK_CENTER);
		_thumbnail->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
		_thumbnail->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
		content_box->add_child(_thumbnail);

		if (grid) {
			_name_lbl = memnew(HighlightedLabel);
			_name_lbl->add_theme_font_size_override(SceneStringName(font_size), 13 * EDSCALE);
			_name_lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			_name_lbl->set_text_horizontal_alignment(HorizontalAlignment::HORIZONTAL_ALIGNMENT_CENTER);
			_name_lbl->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
			content_box->add_child(_name_lbl);
		} else {
			VBoxContainer *text_vbox = memnew(VBoxContainer);
			text_vbox->add_theme_constant_override(SNAME("separation"), -7 * EDSCALE);
			text_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			text_vbox->set_v_size_flags(Control::SIZE_FILL);
			content_box->add_child(text_vbox);

			{
				_name_lbl = memnew(HighlightedLabel);
				_name_lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
				_name_lbl->set_text_horizontal_alignment(HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT);
				_name_lbl->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
				text_vbox->add_child(_name_lbl);

				_path_lbl = memnew(HighlightedLabel);
				_path_lbl->add_theme_font_size_override(SceneStringName(font_size), 12 * EDSCALE);
				_path_lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
				_path_lbl->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
				text_vbox->add_child(_path_lbl);
			}
		}
	}

	_badges_box = memnew(BoxContainer(!grid));
	_badges_box->hide();
	_badges_box->set_alignment(BoxContainer::ALIGNMENT_END);
	_badges_box->set_h_size_flags(Control::SIZE_SHRINK_END);
	_badges_box->set_reverse_sort(!grid);
	_badges_box->set_v_size_flags(Control::SIZE_SHRINK_BEGIN);
	add_child(_badges_box);

	{
		_recent_badge = memnew(TextureRect);
		_recent_badge->hide();
		_recent_badge->set_custom_minimum_size(Size2(16, 16) * EDSCALE);
		_recent_badge->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
		_recent_badge->set_h_size_flags(Control::SIZE_SHRINK_CENTER);
		_recent_badge->set_mouse_filter(Control::MOUSE_FILTER_PASS);
		_recent_badge->set_self_modulate(Color(1, 1, 1, 0.75));
		_recent_badge->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
		_recent_badge->set_tooltip_text(TTR("Item recently opened"));
		_recent_badge->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
		_badges_box->add_child(_recent_badge);

		_source_badge = memnew(TextureRect);
		_source_badge->hide();
		_source_badge->set_custom_minimum_size(Size2(16, 16) * EDSCALE);
		_source_badge->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
		_source_badge->set_h_size_flags(Control::SIZE_SHRINK_CENTER);
		_source_badge->set_mouse_filter(Control::MOUSE_FILTER_PASS);
		_source_badge->set_self_modulate(Color(1, 1, 1, 0.75));
		_source_badge->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
		_source_badge->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
		_badges_box->add_child(_source_badge);
	}

	if (grid) {
		_update_grid_size();
	}
}

void QuickOpenResultItemContainer::set_grid_item_scale(float p_scale) {
	_grid_item_scale = p_scale;
	if (_display_mode == EditorQuickOpenDialog::DisplayMode::GRID) {
		_update_grid_size();
	}
}

void QuickOpenResultItemContainer::reset() {
	_set_enabled(false);
	_is_hovering = false;
	_is_selected = false;
	_recently_opened = false;
	_thumbnail->set_texture(nullptr);
	_name_lbl->set_text("");
	_name_lbl->reset_highlights();
	if (_path_lbl) {
		_path_lbl->set_text("");
		_path_lbl->reset_highlights();
	}
	_name_lbl->remove_theme_color_override(SceneStringName(font_normal_color));
	_content_source = ContentSource::PROJECT;
	set_tooltip_text("");
	_update_badges();
}

void QuickOpenResultItemContainer::set_content(const QuickOpenResultCandidate &p_candidate, bool p_recently_opened) {
	_set_enabled(true);
	_recently_opened = p_recently_opened;
	const String file_path = ResourceUID::get_singleton()->get_id_path(p_candidate.uid);
	String source_label;
	if (EditorContentResources::get_resource_uid(file_path) != ResourceUID::INVALID_ID) {
		_content_source = ContentSource::EDITOR;
		source_label = TTR("Editor content (read-only)");
	} else if (file_path.begins_with("res://addons/")) {
		_content_source = ContentSource::ADDON;
		source_label = TTR("Addon content");
	} else {
		_content_source = ContentSource::PROJECT;
		source_label = TTR("Project content");
	}
	set_tooltip_text(source_label + "\n" + file_path);
	_update_badges();

	_thumbnail->set_texture(p_candidate.thumbnail);
	_update_thumbnail_stretch();
	_name_lbl->set_text(file_path.get_file());
	_name_lbl->reset_highlights();
	if (_path_lbl) {
		_path_lbl->set_text(file_path.get_base_dir());
		_path_lbl->reset_highlights();
	}

	if (enable_highlights && p_candidate.result.is_valid()) {
		for (const FuzzyTokenMatch &match : p_candidate.result->get_token_matches()) {
			for (const Vector2i &interval : match.substrings) {
				if (_path_lbl) {
					_path_lbl->add_highlight(_get_path_interval(interval, p_candidate.result->get_dir_index()));
				}
				_name_lbl->add_highlight(_get_name_interval(interval, p_candidate.result->get_dir_index()));
			}
		}
	}

	queue_redraw();
}

void QuickOpenResultItemContainer::highlight_item(bool p_enabled) {
	_is_selected = p_enabled;
	if (_is_selected) {
		_name_lbl->add_theme_color_override(SceneStringName(font_normal_color), _highlighted_font_color);
	} else {
		_name_lbl->remove_theme_color_override(SceneStringName(font_normal_color));
	}
	queue_redraw();
}

void QuickOpenResultItemContainer::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_MOUSE_ENTER:
		case NOTIFICATION_MOUSE_EXIT: {
			_is_hovering = is_visible() && p_what == NOTIFICATION_MOUSE_ENTER;
			queue_redraw();
		} break;
		case NOTIFICATION_THEME_CHANGED: {
			_selected_stylebox = get_theme_stylebox("selected", "Tree");
			_hovering_stylebox = get_theme_stylebox(SNAME("hovered"), "Tree");
			_highlighted_font_color = get_theme_color("font_focus_color", EditorStringName(Editor));
			if (_path_lbl) {
				_path_lbl->add_theme_color_override(SceneStringName(font_normal_color), get_theme_color("font_disabled_color", EditorStringName(Editor)));
			}
			highlight_item(_is_selected);
			_update_badges();
		} break;
		case NOTIFICATION_DRAW: {
			if (_is_selected) {
				draw_style_box(_selected_stylebox, Rect2(Point2(), get_size()));
			} else if (_is_hovering) {
				draw_style_box(_hovering_stylebox, Rect2(Point2(), get_size()));
			}
		} break;
	}
}

Vector2i QuickOpenResultItemContainer::_get_path_interval(const Vector2i &p_interval, int p_dir_index) {
	if (p_interval.x >= p_dir_index || p_interval.y < 1) {
		return { -1, -1 };
	}
	return { p_interval.x, MIN(p_interval.x + p_interval.y, p_dir_index) - p_interval.x };
}

Vector2i QuickOpenResultItemContainer::_get_name_interval(const Vector2i &p_interval, int p_dir_index) {
	if (p_interval.x + p_interval.y <= p_dir_index || p_interval.y < 1) {
		return { -1, -1 };
	}
	const int first_name_idx = p_dir_index + 1;
	const int start = MAX(p_interval.x, first_name_idx);
	return { start - first_name_idx, p_interval.y - start + p_interval.x };
}

void QuickOpenResultItemContainer::_set_enabled(bool p_enabled) {
	set_visible(p_enabled);
	set_process(p_enabled);
	set_process_input(p_enabled);
}

void QuickOpenResultItemContainer::_update_grid_size() {
	set_custom_minimum_size(Size2(128 * _grid_item_scale * EDSCALE, 0));
	const int thumbnail_size = 64 * _grid_item_scale * EDSCALE;
	_thumbnail->set_custom_minimum_size(Size2i(thumbnail_size, thumbnail_size));
	queue_redraw();
}

void QuickOpenResultItemContainer::_update_thumbnail_stretch() {
	const Ref<Texture2D> texture = _thumbnail->get_texture();
	const bool uses_icon = _display_mode == EditorQuickOpenDialog::DisplayMode::GRID && texture.is_valid() && texture->get_width() < 32 * EDSCALE;
	_thumbnail->set_stretch_mode(uses_icon ? TextureRect::STRETCH_KEEP_CENTERED : TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
}

void QuickOpenResultItemContainer::_update_badges() {
	if (!_badges_box || !_source_badge || !_recent_badge) {
		return;
	}
	_badges_box->set_visible(_recently_opened || _content_source != ContentSource::PROJECT);
	_recent_badge->set_visible(_recently_opened);
	if (_recently_opened) {
		_recent_badge->set_texture(get_editor_theme_icon(SNAME("dock_history")));
	} else {
		_recent_badge->set_texture(nullptr);
	}

	StringName icon_name;
	switch (_content_source) {
		case ContentSource::PROJECT: {
			_source_badge->hide();
			_source_badge->set_texture(nullptr);
			_source_badge->set_tooltip_text("");
			return;
		}
		case ContentSource::ADDON: {
			icon_name = SNAME("action_plugin");
			_source_badge->set_tooltip_text(TTR("Item from an addon"));
		} break;
		case ContentSource::EDITOR: {
			icon_name = SNAME("branding_logo_monochrome");
			_source_badge->set_tooltip_text(TTR("Editor content (read-only)"));
		} break;
	}
	_source_badge->set_texture(get_editor_theme_icon(icon_name));
	_source_badge->show();
}
