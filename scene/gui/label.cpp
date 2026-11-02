/**************************************************************************/
/*  label.cpp                                                             */
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

#include "label.h"

#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/font.h"
#include "scene/resources/label_settings.h"
#include "scene/resources/style_box.h"
#include "scene/theme/theme_db.h"
#include "servers/rendering/rendering_server.h"
#include "servers/text/text_server.h"

namespace {

inline void draw_glyph(const Glyph &p_gl, const RID &p_canvas, const Color &p_font_color, const Vector2 &p_ofs) {
	if (p_gl.font_rid != RID()) {
		TS->font_draw_glyph(p_gl.font_rid, p_canvas, p_gl.font_size, p_ofs + Vector2(p_gl.x_off, p_gl.y_off), p_gl.index, p_font_color);
	} else if (((p_gl.flags & TextServer::GRAPHEME_IS_VIRTUAL) != TextServer::GRAPHEME_IS_VIRTUAL) && ((p_gl.flags & TextServer::GRAPHEME_IS_EMBEDDED_OBJECT) != TextServer::GRAPHEME_IS_EMBEDDED_OBJECT)) {
		TS->draw_hex_code_box(p_canvas, p_gl.font_size, p_ofs + Vector2(p_gl.x_off, p_gl.y_off), p_gl.index, p_font_color);
	}
}

inline void draw_glyph_shadow(const Glyph &p_gl, const RID &p_canvas, const Color &p_font_shadow_color, const Vector2 &p_ofs, const Vector2 &p_shadow_offset) {
	if (p_gl.font_rid != RID()) {
		TS->font_draw_glyph(p_gl.font_rid, p_canvas, p_gl.font_size, p_ofs + Vector2(p_gl.x_off, p_gl.y_off) + p_shadow_offset, p_gl.index, p_font_shadow_color);
	}
}

inline void draw_glyph_shadow_outline(const Glyph &p_gl, const RID &p_canvas, const Color &p_font_shadow_color, const Vector2 &p_ofs, int p_shadow_outline_size, const Vector2 &p_shadow_offset) {
	if (p_gl.font_rid != RID()) {
		TS->font_draw_glyph_outline(p_gl.font_rid, p_canvas, p_gl.font_size, p_shadow_outline_size, p_ofs + Vector2(p_gl.x_off, p_gl.y_off) + p_shadow_offset, p_gl.index, p_font_shadow_color);
	}
}

inline void draw_glyph_outline(const Glyph &p_gl, const RID &p_canvas, const Color &p_font_outline_color, const Vector2 &p_ofs, int p_outline_size) {
	if (p_gl.font_rid != RID()) {
		if (p_font_outline_color.a != 0.0 && p_outline_size > 0) {
			TS->font_draw_glyph_outline(p_gl.font_rid, p_canvas, p_gl.font_size, p_outline_size, p_ofs + Vector2(p_gl.x_off, p_gl.y_off), p_gl.index, p_font_outline_color);
		}
	}
}

} // namespace

Label::Label(const String &p_text) {
	connect(SceneStringName(maximum_size_changed), callable_mp(this, &Label::_maximum_size_changed));

	set_mouse_filter(MOUSE_FILTER_IGNORE);
	set_text(p_text);
	set_v_size_flags(SIZE_SHRINK_CENTER);
}

Label::~Label() {
	_clear_paragraphs();
}

Size2 Label::get_minimum_size() const {
	_ensure_shaped();

	Size2 min_size = _minimum_size;
	if (_resize_font_to_fit) {
		min_size = Size2(1, 1);
	}

	const Ref<Font> font = _get_font();
	const int font_size = _resize_font_to_fit ? _minimum_font_size : _get_base_font_size();

	min_size.height = MAX(min_size.height, font->get_height(font_size));

	const Size2 min_style = theme_cache.normal_style->get_minimum_size();
	if (_autowrap_mode != TextServer::AUTOWRAP_OFF) {
		if (!_clip_text && _text_overrun_behavior != TextServer::OVERRUN_NO_TRIMMING && _max_lines_visible > 0) {
			const int line_spacing = _get_line_spacing();
			min_size.height = MIN(min_size.height, (font->get_height(font_size) + line_spacing) * _max_lines_visible);
		} else if (_clip_text || _text_overrun_behavior != TextServer::OVERRUN_NO_TRIMMING) {
			min_size.height = 1;
		}
		return Size2(1, min_size.height) + min_style;
	} else {
		if (_clip_text || _text_overrun_behavior != TextServer::OVERRUN_NO_TRIMMING || _resize_font_to_fit) {
			min_size.width = 1;
		}
		return min_size + min_style;
	}
}

Size2 Label::get_desired_size() const {
	const Size2 combined_max = get_combined_maximum_size();
	if (combined_max.width < 0) {
		return Size2();
	}

	_ensure_shaped();
	const Size2 min_style = theme_cache.normal_style->get_minimum_size();
	Size2 content_size = _minimum_size + min_style;
	content_size.width = MIN(content_size.width, int(combined_max.width));

	return content_size;
}

PackedStringArray Label::get_configuration_warnings() const {
	PackedStringArray warnings = Control::get_configuration_warnings();

	// FIXME: This is not ideal and the sizing model should be fixed,
	// but for now we have to warn about this impossible to resolve combination.
	// See GH-83546.
	if (is_inside_tree() && get_tree()->get_edited_scene_root() != this) {
		if (_autowrap_mode != TextServer::AUTOWRAP_OFF && get_combined_maximum_size().width <= 0 && get_custom_minimum_size().width <= 0) {
			warnings.push_back(RTR("Labels with autowrapping enabled must have a positive custom minimum or maximum width configured to work correctly."));
		}
	}

	// Ensure that the font can render all of the required glyphs.
	const Ref<Font> font = _get_font();

	if (font.is_valid()) {
		_ensure_shaped();

		for (const Paragraph &para : _paragraphs) {
			const Glyph *glyph = TS->shaped_text_get_glyphs(para.text_rid);
			const int64_t glyph_count = TS->shaped_text_get_glyph_count(para.text_rid);
			for (int64_t i = 0; i < glyph_count; i++) {
				if (glyph[i].font_rid == RID() && glyph[i].index != 0) {
					warnings.push_back(RTR("The current font does not support rendering one or more characters used in this Label's text."));
					break;
				}
			}
		}
	}

	const Ref<FontFile> ff = font;
	if (ff.is_valid() && ff->is_multichannel_signed_distance_field()) {
		const bool has_settings = _label_settings.is_valid();
		const int font_size = get_rendered_font_size();
		const int outline_size = has_settings ? _label_settings->get_outline_size() : theme_cache.font_outline_size;
		const Vector<LabelSettings::StackedOutlineData> stacked_outline_datas = has_settings ? _label_settings->get_stacked_outline_data() : Vector<LabelSettings::StackedOutlineData>();
		const Vector<LabelSettings::StackedShadowData> stacked_shadow_datas = has_settings ? _label_settings->get_stacked_shadow_data() : Vector<LabelSettings::StackedShadowData>();
		int max_outline_draw_size = outline_size;
		if (stacked_outline_datas.size() != 0) {
			const int draw_iterations = stacked_outline_datas.size();
			for (int j = 0; j < draw_iterations; j++) {
				const int stacked_outline_size = stacked_outline_datas[j].size;
				if (stacked_outline_size <= 0) {
					continue;
				}
				max_outline_draw_size += stacked_outline_size;
			}
		}
		if (stacked_shadow_datas.size() != 0) {
			const int draw_iterations = stacked_shadow_datas.size();
			for (int j = 0; j < draw_iterations; j++) {
				const LabelSettings::StackedShadowData stacked_shadow_data = stacked_shadow_datas[j];
				if (stacked_shadow_data.outline_size > 0) {
					max_outline_draw_size = MAX(max_outline_draw_size, stacked_shadow_data.outline_size);
				}
			}
		}
		float scale = (float)font_size / (float)ff->get_msdf_size();
		float ol = (float)max_outline_draw_size / scale / 4.0;
		float pxr = (float)ff->get_msdf_pixel_range() / 2.0 - 1.0;
		float r_pxr = (ol + 1.0) * 2.0;

		if (ol > pxr) {
			warnings.push_back(vformat(RTR("MSDF font pixel range is too small, some outlines/shadows will not render. Set MSDF pixel range to be at least %d to render all outlines/shadows."), Math::ceil(r_pxr)));
		}
	}

	return warnings;
}

void Label::set_horizontal_alignment(HorizontalAlignment p_alignment) {
	ERR_FAIL_INDEX((int)p_alignment, 4);
	if (_horizontal_alignment == p_alignment) {
		return;
	}

	if (_horizontal_alignment == HORIZONTAL_ALIGNMENT_FILL || p_alignment == HORIZONTAL_ALIGNMENT_FILL) {
		_mark_lines_dirty();
	}
	_horizontal_alignment = p_alignment;
	queue_redraw();
}

HorizontalAlignment Label::get_horizontal_alignment() const {
	return _horizontal_alignment;
}

void Label::set_vertical_alignment(VerticalAlignment p_alignment) {
	ERR_FAIL_INDEX((int)p_alignment, 4);

	if (_vertical_alignment == p_alignment) {
		return;
	}

	_vertical_alignment = p_alignment;
	queue_redraw();
}

VerticalAlignment Label::get_vertical_alignment() const {
	return _vertical_alignment;
}

void Label::set_text(const String &p_string) {
	if (_text == p_string) {
		return;
	}
	_text = p_string;
	_translated_text = atr(p_string);
	_text_dirty = true;
	if (_visible_ratio < 1) {
		_visible_characters = get_total_character_count() * _visible_ratio;
	}
	queue_redraw();
	update_minimum_size();
	update_desired_size();
	update_configuration_warnings();
}

String Label::get_text() const {
	return _text;
}

void Label::set_label_settings(const Ref<LabelSettings> &p_settings) {
	if (_label_settings == p_settings) {
		return;
	}

	if (_label_settings.is_valid()) {
		_label_settings->disconnect_changed(callable_mp(this, &Label::_invalidate));
	}
	_label_settings = p_settings;
	if (_label_settings.is_valid()) {
		_label_settings->connect_changed(callable_mp(this, &Label::_invalidate), CONNECT_REFERENCE_COUNTED);
	}
	_invalidate();
}

Ref<LabelSettings> Label::get_label_settings() const {
	return _label_settings;
}

void Label::set_language(const String &p_language) {
	if (_language != p_language) {
		_language = p_language;
		for (Paragraph &para : _paragraphs) {
			para.text_dirty = true;
		}
		queue_redraw();
	}
}

String Label::get_language() const {
	return _language;
}

void Label::set_paragraph_separator(const String &p_paragraph_separator) {
	if (_paragraph_separator != p_paragraph_separator) {
		_paragraph_separator = p_paragraph_separator;
		_text_dirty = true;
		queue_redraw();
	}
}

String Label::get_paragraph_separator() const {
	return _paragraph_separator;
}

void Label::set_resize_font_to_fit(bool p_enabled) {
	if (_resize_font_to_fit == p_enabled) {
		return;
	}

	_resize_font_to_fit = p_enabled;
	if (!p_enabled) {
		_current_fitted_font_size = 0;
	}
	_font_dirty = true;
	_mark_lines_dirty();
	queue_redraw();
	update_minimum_size();
	update_configuration_warnings();
}

bool Label::is_resize_font_to_fit_enabled() const {
	return _resize_font_to_fit;
}

void Label::set_minimum_font_size(int p_size) {
	if (_minimum_font_size == p_size) {
		return;
	}

	_minimum_font_size = p_size;
	if (_resize_font_to_fit) {
		_mark_lines_dirty();
		queue_redraw();
		update_minimum_size();
	}
}

int Label::get_minimum_font_size() const {
	return _minimum_font_size;
}

void Label::set_maximum_font_size(int p_size) {
	if (_maximum_font_size == p_size) {
		return;
	}

	_maximum_font_size = p_size;
	if (_resize_font_to_fit) {
		_mark_lines_dirty();
		queue_redraw();
		update_minimum_size();
	}
}

int Label::get_maximum_font_size() const {
	return _maximum_font_size;
}

int Label::get_rendered_font_size() const {
	return _current_fitted_font_size > 0 ? _current_fitted_font_size : _get_base_font_size();
}

void Label::set_autowrap_mode(TextServer::AutowrapMode p_mode) {
	if (_autowrap_mode == p_mode) {
		return;
	}

	_autowrap_mode = p_mode;
	_mark_lines_dirty();
	queue_redraw();
	update_configuration_warnings();

	if (_clip_text || _text_overrun_behavior != TextServer::OVERRUN_NO_TRIMMING) {
		update_minimum_size();
	}
	update_desired_size();
}

TextServer::AutowrapMode Label::get_autowrap_mode() const {
	return _autowrap_mode;
}

void Label::set_autowrap_trim_flags(BitField<TextServer::LineBreakFlag> p_flags) {
	if (_autowrap_trim_flags == (p_flags & TextServer::BREAK_TRIM_MASK)) {
		return;
	}

	_autowrap_trim_flags = p_flags & TextServer::BREAK_TRIM_MASK;
	_mark_lines_dirty();
	queue_redraw();
	update_configuration_warnings();

	if (_clip_text || _text_overrun_behavior != TextServer::OVERRUN_NO_TRIMMING) {
		update_minimum_size();
	}
	update_desired_size();
}

BitField<TextServer::LineBreakFlag> Label::get_autowrap_trim_flags() const {
	return _autowrap_trim_flags;
}

void Label::set_justification_flags(BitField<TextServer::JustificationFlag> p_flags) {
	if (_justification_flags == p_flags) {
		return;
	}

	_justification_flags = p_flags;
	_mark_lines_dirty();
	queue_redraw();
}

BitField<TextServer::JustificationFlag> Label::get_justification_flags() const {
	return _justification_flags;
}

void Label::set_uppercase(bool p_uppercase) {
	if (_uppercase == p_uppercase) {
		return;
	}

	_uppercase = p_uppercase;
	_text_dirty = true;

	queue_redraw();
}

bool Label::is_uppercase() const {
	return _uppercase;
}

void Label::set_visible_characters_behavior(TextServer::VisibleCharactersBehavior p_behavior) {
	if (_visible_characters_behavior != p_behavior) {
		if (_visible_characters_behavior == TextServer::VC_CHARS_BEFORE_SHAPING || p_behavior == TextServer::VC_CHARS_BEFORE_SHAPING) {
			_text_dirty = true;
		}
		_visible_characters_behavior = p_behavior;
		queue_redraw();
	}
}

TextServer::VisibleCharactersBehavior Label::get_visible_characters_behavior() const {
	return _visible_characters_behavior;
}

void Label::set_visible_characters(int p_amount) {
	if (_visible_characters != p_amount) {
		_visible_characters = p_amount;
		if (p_amount == -1 || get_total_character_count() == 0) {
			_visible_ratio = 1.0;
		} else {
			_visible_ratio = (float)p_amount / (float)get_total_character_count();
		}
		if (_visible_characters_behavior == TextServer::VC_CHARS_BEFORE_SHAPING) {
			_text_dirty = true;
		}
		queue_redraw();
	}
}

int Label::get_visible_characters() const {
	return _visible_characters;
}

int Label::get_total_character_count() const {
	return _translated_text.length();
}

void Label::set_visible_ratio(float p_ratio) {
	if (_visible_ratio != p_ratio) {
		if (p_ratio >= 1.0) {
			_visible_characters = -1;
			_visible_ratio = 1.0;
		} else if (p_ratio < 0.0) {
			_visible_characters = 0;
			_visible_ratio = 0.0;
		} else {
			_visible_characters = get_total_character_count() * p_ratio;
			_visible_ratio = p_ratio;
		}

		if (_visible_characters_behavior == TextServer::VC_CHARS_BEFORE_SHAPING) {
			_text_dirty = true;
		}
		queue_redraw();
	}
}

float Label::get_visible_ratio() const {
	return _visible_ratio;
}

void Label::set_clip_text(bool p_clip) {
	if (_clip_text == p_clip) {
		return;
	}

	_clip_text = p_clip;
	queue_redraw();
	update_minimum_size();
	update_desired_size();
}

bool Label::is_clipping_text() const {
	return _clip_text;
}

void Label::set_tab_stops(const PackedFloat32Array &p_tab_stops) {
	if (_tab_stops != p_tab_stops) {
		_tab_stops = p_tab_stops;
		for (Paragraph &para : _paragraphs) {
			para.text_dirty = true;
		}
		queue_redraw();
	}
}

PackedFloat32Array Label::get_tab_stops() const {
	return _tab_stops;
}

void Label::set_text_overrun_behavior(TextServer::OverrunBehavior p_behavior) {
	if (_text_overrun_behavior == p_behavior) {
		return;
	}

	_text_overrun_behavior = p_behavior;
	_mark_lines_dirty();
	queue_redraw();
	if (_clip_text || _text_overrun_behavior != TextServer::OVERRUN_NO_TRIMMING) {
		update_minimum_size();
	}
	update_desired_size();
}

TextServer::OverrunBehavior Label::get_text_overrun_behavior() const {
	return _text_overrun_behavior;
}

void Label::set_ellipsis_char(const String &p_char) {
	String c = p_char;
	if (c.length() > 1) {
		WARN_PRINT("Ellipsis must be exactly one character long (" + itos(c.length()) + " characters given).");
		c = c.left(1);
	}
	if (_ellipsis_char == c) {
		return;
	}
	_ellipsis_char = c;
	_mark_lines_dirty();
	queue_redraw();
	if (_clip_text || _text_overrun_behavior != TextServer::OVERRUN_NO_TRIMMING) {
		update_minimum_size();
	}
	update_desired_size();
}

String Label::get_ellipsis_char() const {
	return _ellipsis_char;
}

void Label::set_lines_skipped(int p_lines) {
	ERR_FAIL_COND(p_lines < 0);

	if (_lines_skipped == p_lines) {
		return;
	}

	_lines_skipped = p_lines;
	_update_visible();
	queue_redraw();
}

int Label::get_lines_skipped() const {
	return _lines_skipped;
}

void Label::set_max_lines_visible(int p_lines) {
	if (_max_lines_visible == p_lines) {
		return;
	}

	_max_lines_visible = p_lines;
	_update_visible();
	queue_redraw();
}

int Label::get_max_lines_visible() const {
	return _max_lines_visible;
}

int Label::get_line_height(int p_line) const {
	const Ref<Font> font = _get_font();
	const int font_size = get_rendered_font_size();
	const int font_h = font->get_height(font_size);
	if (p_line >= 0 && p_line < _total_line_count) {
		const RID rid = _get_line_rid(p_line);
		const LineMetrics metrics = _get_line_metrics(rid, font_h);
		return metrics.ascent + metrics.descent;
	} else if (_total_line_count > 0) {
		int h = font_h;
		for (const Paragraph &para : _paragraphs) {
			for (const RID &line_rid : para.lines_rid) {
				h = MAX(h, TS->shaped_text_get_size(line_rid).y);
			}
		}
		return h;
	} else {
		return font->get_height(font_size);
	}
}

int Label::get_line_count() const {
	if (!is_inside_tree()) {
		return 1;
	}
	_ensure_shaped();

	return _total_line_count;
}

int Label::get_visible_line_count() const {
	const Ref<StyleBox> style = theme_cache.normal_style;
	const Ref<Font> font = _get_font();
	const int font_size = get_rendered_font_size();
	const int font_h = font->get_height(font_size);
	const int line_spacing = _get_line_spacing();
	const int paragraph_spacing = _get_paragraph_spacing();
	int lines_visible = 0;
	float total_h = 0.0;

	int line_index = 0;
	for (const Paragraph &para : _paragraphs) {
		if (line_index + para.lines_rid.size() <= _lines_skipped) {
			line_index += para.lines_rid.size();
		} else {
			const int start = (line_index < _lines_skipped) ? _lines_skipped - line_index : 0;
			for (int i = start; i < para.lines_rid.size(); i++) {
				const LineMetrics metrics = _get_line_metrics(para.lines_rid[i], font_h);
				total_h += metrics.ascent + metrics.descent + line_spacing;
				if (total_h > Math::ceil(get_size().height - style->get_minimum_size().height + line_spacing)) {
					break;
				}
				lines_visible++;
			}
			total_h += paragraph_spacing;
			line_index += para.lines_rid.size();
		}
	}

	if (_max_lines_visible >= 0 && lines_visible > _max_lines_visible) {
		lines_visible = _max_lines_visible;
	}

	return lines_visible;
}

Rect2 Label::get_character_bounds(int p_pos) const {
	_ensure_shaped();

	const int paragraph_spacing = _get_paragraph_spacing();
	const Ref<Font> font = _get_font();
	const int font_size = get_rendered_font_size();
	const int font_h = font->get_height(font_size);

	Vector2 ofs;
	int line_spacing;
	int last_line;
	_get_layout_data(ofs, last_line, line_spacing);

	int line_index = 0;
	for (int p = 0; p < _paragraphs.size(); p++) {
		const Paragraph &para = _paragraphs[p];
		if (line_index + para.lines_rid.size() <= _lines_skipped) {
			line_index += para.lines_rid.size();
		} else {
			const int start = (line_index < _lines_skipped) ? _lines_skipped - line_index : 0;
			const int end = (line_index + para.lines_rid.size() < last_line) ? para.lines_rid.size() : last_line - line_index;
			if (end <= 0) {
				break;
			}
			for (int i = start; i < end; i++) {
				const RID line_rid = para.lines_rid[i];
				const Rect2 line_rect = _get_paragraph_line_rect(p, i);
				ofs.x = line_rect.position.x;

				const int v_size = TS->shaped_text_get_glyph_count(line_rid);
				const Glyph *glyphs = TS->shaped_text_get_glyphs(line_rid);

				float gl_off = 0.0f;
				for (int j = 0; j < v_size; j++) {
					if ((glyphs[j].count > 0) && ((glyphs[j].index != 0) || ((glyphs[j].flags & TextServer::GRAPHEME_IS_SPACE) == TextServer::GRAPHEME_IS_SPACE))) {
						if (p_pos >= glyphs[j].start + para.start && p_pos < glyphs[j].end + para.start) {
							float advance = 0.f;
							for (int k = 0; k < glyphs[j].count; k++) {
								advance += glyphs[j + k].advance;
							}
							Rect2 rect;
							rect.position = ofs + Vector2(gl_off, 0);
							rect.size = Vector2(advance, line_rect.size.y);
							return rect;
						}
					}
					gl_off += glyphs[j].advance * glyphs[j].repeat;
				}
				const LineMetrics metrics = _get_line_metrics(line_rid, font_h);
				ofs.y += metrics.ascent + metrics.descent + line_spacing;
			}
			ofs.y += paragraph_spacing;
			line_index += para.lines_rid.size();
		}
	}
	return Rect2();
}

void Label::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_horizontal_alignment", "horizontal_alignment"), &Label::set_horizontal_alignment);
	ClassDB::bind_method(D_METHOD("get_horizontal_alignment"), &Label::get_horizontal_alignment);
	ClassDB::bind_method(D_METHOD("set_vertical_alignment", "vertical_alignment"), &Label::set_vertical_alignment);
	ClassDB::bind_method(D_METHOD("get_vertical_alignment"), &Label::get_vertical_alignment);
	ClassDB::bind_method(D_METHOD("set_text", "text"), &Label::set_text);
	ClassDB::bind_method(D_METHOD("get_text"), &Label::get_text);
	ClassDB::bind_method(D_METHOD("set_label_settings", "label_settings"), &Label::set_label_settings);
	ClassDB::bind_method(D_METHOD("get_label_settings"), &Label::get_label_settings);
	ClassDB::bind_method(D_METHOD("set_language", "language"), &Label::set_language);
	ClassDB::bind_method(D_METHOD("get_language"), &Label::get_language);
	ClassDB::bind_method(D_METHOD("set_paragraph_separator", "paragraph_separator"), &Label::set_paragraph_separator);
	ClassDB::bind_method(D_METHOD("get_paragraph_separator"), &Label::get_paragraph_separator);
	ClassDB::bind_method(D_METHOD("set_autowrap_mode", "autowrap_mode"), &Label::set_autowrap_mode);
	ClassDB::bind_method(D_METHOD("get_autowrap_mode"), &Label::get_autowrap_mode);
	ClassDB::bind_method(D_METHOD("set_autowrap_trim_flags", "autowrap_trim_flags"), &Label::set_autowrap_trim_flags);
	ClassDB::bind_method(D_METHOD("get_autowrap_trim_flags"), &Label::get_autowrap_trim_flags);
	ClassDB::bind_method(D_METHOD("set_justification_flags", "justification_flags"), &Label::set_justification_flags);
	ClassDB::bind_method(D_METHOD("get_justification_flags"), &Label::get_justification_flags);
	ClassDB::bind_method(D_METHOD("set_clip_text", "clip_text"), &Label::set_clip_text);
	ClassDB::bind_method(D_METHOD("is_clipping_text"), &Label::is_clipping_text);
	ClassDB::bind_method(D_METHOD("set_tab_stops", "tab_stops"), &Label::set_tab_stops);
	ClassDB::bind_method(D_METHOD("get_tab_stops"), &Label::get_tab_stops);
	ClassDB::bind_method(D_METHOD("set_text_overrun_behavior", "text_overrun_behavior"), &Label::set_text_overrun_behavior);
	ClassDB::bind_method(D_METHOD("get_text_overrun_behavior"), &Label::get_text_overrun_behavior);
	ClassDB::bind_method(D_METHOD("set_ellipsis_char", "ellipsis_char"), &Label::set_ellipsis_char);
	ClassDB::bind_method(D_METHOD("get_ellipsis_char"), &Label::get_ellipsis_char);
	ClassDB::bind_method(D_METHOD("set_uppercase", "uppercase"), &Label::set_uppercase);
	ClassDB::bind_method(D_METHOD("is_uppercase"), &Label::is_uppercase);
	ClassDB::bind_method(D_METHOD("get_line_height", "line"), &Label::get_line_height, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("get_line_count"), &Label::get_line_count);
	ClassDB::bind_method(D_METHOD("get_visible_line_count"), &Label::get_visible_line_count);
	ClassDB::bind_method(D_METHOD("get_total_character_count"), &Label::get_total_character_count);
	ClassDB::bind_method(D_METHOD("set_visible_characters", "visible_characters"), &Label::set_visible_characters);
	ClassDB::bind_method(D_METHOD("get_visible_characters"), &Label::get_visible_characters);
	ClassDB::bind_method(D_METHOD("set_visible_characters_behavior", "visible_characters_behavior"), &Label::set_visible_characters_behavior);
	ClassDB::bind_method(D_METHOD("get_visible_characters_behavior"), &Label::get_visible_characters_behavior);
	ClassDB::bind_method(D_METHOD("set_visible_ratio", "visible_ratio"), &Label::set_visible_ratio);
	ClassDB::bind_method(D_METHOD("get_visible_ratio"), &Label::get_visible_ratio);
	ClassDB::bind_method(D_METHOD("set_lines_skipped", "lines_skipped"), &Label::set_lines_skipped);
	ClassDB::bind_method(D_METHOD("get_lines_skipped"), &Label::get_lines_skipped);
	ClassDB::bind_method(D_METHOD("set_max_lines_visible", "max_lines_visible"), &Label::set_max_lines_visible);
	ClassDB::bind_method(D_METHOD("get_max_lines_visible"), &Label::get_max_lines_visible);

	ClassDB::bind_method(D_METHOD("set_resize_font_to_fit", "resize_font_to_fit"), &Label::set_resize_font_to_fit);
	ClassDB::bind_method(D_METHOD("is_resize_font_to_fit_enabled"), &Label::is_resize_font_to_fit_enabled);
	ClassDB::bind_method(D_METHOD("set_minimum_font_size", "minimum_font_size"), &Label::set_minimum_font_size);
	ClassDB::bind_method(D_METHOD("get_minimum_font_size"), &Label::get_minimum_font_size);
	ClassDB::bind_method(D_METHOD("set_maximum_font_size", "maximum_font_size"), &Label::set_maximum_font_size);
	ClassDB::bind_method(D_METHOD("get_maximum_font_size"), &Label::get_maximum_font_size);
	ClassDB::bind_method(D_METHOD("get_rendered_font_size"), &Label::get_rendered_font_size);

	ClassDB::bind_method(D_METHOD("get_character_bounds", "pos"), &Label::get_character_bounds);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "text", PROPERTY_HINT_MULTILINE_TEXT), "set_text", "get_text");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "label_settings", PROPERTY_HINT_RESOURCE_TYPE, LabelSettings::get_class_static()), "set_label_settings", "get_label_settings");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "horizontal_alignment", PROPERTY_HINT_ENUM, "Left,Center,Right,Fill"), "set_horizontal_alignment", "get_horizontal_alignment");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "vertical_alignment", PROPERTY_HINT_ENUM, "Top,Center,Bottom,Fill"), "set_vertical_alignment", "get_vertical_alignment");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "autowrap_mode", PROPERTY_HINT_ENUM, "Off,Arbitrary,Word,Word (Smart)"), "set_autowrap_mode", "get_autowrap_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "autowrap_trim_flags", PROPERTY_HINT_FLAGS, vformat("Trim Spaces After Break:%d,Trim Spaces Before Break:%d", TextServer::BREAK_TRIM_START_EDGE_SPACES, TextServer::BREAK_TRIM_END_EDGE_SPACES)), "set_autowrap_trim_flags", "get_autowrap_trim_flags");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "justification_flags", PROPERTY_HINT_FLAGS, "Kashida Justification:1,Word Justification:2,Justify Only After Last Tab:8,Skip Last Line:32,Skip Last Line With Visible Characters:64,Do Not Skip Single Line:128"), "set_justification_flags", "get_justification_flags");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "paragraph_separator"), "set_paragraph_separator", "get_paragraph_separator");

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "clip_text"), "set_clip_text", "is_clipping_text");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "text_overrun_behavior", PROPERTY_HINT_ENUM, "Trim Nothing,Trim Characters,Trim Words,Ellipsis (6+ Characters),Word Ellipsis (6+ Characters),Ellipsis (Always),Word Ellipsis (Always)"), "set_text_overrun_behavior", "get_text_overrun_behavior");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "ellipsis_char"), "set_ellipsis_char", "get_ellipsis_char");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "uppercase"), "set_uppercase", "is_uppercase");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "tab_stops"), "set_tab_stops", "get_tab_stops");

	ADD_GROUP("Resize Font to Fit", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "resize_font_to_fit", PROPERTY_HINT_GROUP_ENABLE), "set_resize_font_to_fit", "is_resize_font_to_fit_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "minimum_font_size", PROPERTY_HINT_RANGE, "1,256,1,or_greater"), "set_minimum_font_size", "get_minimum_font_size");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "maximum_font_size", PROPERTY_HINT_RANGE, "1,256,1,or_greater"), "set_maximum_font_size", "get_maximum_font_size");

	ADD_GROUP("Displayed Text", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "lines_skipped", PROPERTY_HINT_RANGE, "0,999,1"), "set_lines_skipped", "get_lines_skipped");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_lines_visible", PROPERTY_HINT_RANGE, "-1,999,1"), "set_max_lines_visible", "get_max_lines_visible");
	// Note: "visible_characters" and "visible_ratio" should be set after "text" to be correctly applied.
	ADD_PROPERTY(PropertyInfo(Variant::INT, "visible_characters", PROPERTY_HINT_RANGE, "-1,128000,1"), "set_visible_characters", "get_visible_characters");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "visible_characters_behavior", PROPERTY_HINT_ENUM, "Characters Before Shaping,Characters After Shaping,Glyphs"), "set_visible_characters_behavior", "get_visible_characters_behavior");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "visible_ratio", PROPERTY_HINT_RANGE, "0,1,0.001"), "set_visible_ratio", "get_visible_ratio");

	ADD_GROUP("Localization", "");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "language", PROPERTY_HINT_LOCALE_ID, ""), "set_language", "get_language");

	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_STYLEBOX, Label, normal_style, "normal");
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_STYLEBOX, Label, focus_style, "focus");
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, Label, line_spacing);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, Label, paragraph_spacing);

	BIND_THEME_ITEM(Theme::DATA_TYPE_FONT, Label, font);
	BIND_THEME_ITEM(Theme::DATA_TYPE_FONT_SIZE, Label, font_size);
	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, Label, font_color);
	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, Label, font_shadow_color);
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_CONSTANT, Label, font_shadow_offset.x, "shadow_offset_x");
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_CONSTANT, Label, font_shadow_offset.y, "shadow_offset_y");
	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, Label, font_outline_color);
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_CONSTANT, Label, font_outline_size, "outline_size");
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_CONSTANT, Label, font_shadow_outline_size, "shadow_outline_size");
}

void Label::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_TRANSLATION_CHANGED: {
			const String new_text = atr(_text);
			if (new_text != _translated_text) {
				_translated_text = new_text;
				if (_visible_ratio < 1) {
					_visible_characters = get_total_character_count() * _visible_ratio;
				}
			}
			_text_dirty = true; // Language update might change the appearance of some characters.

			queue_redraw();
			update_configuration_warnings();
		} break;

		case NOTIFICATION_DRAW: {
			_draw();
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			_font_dirty = true;
			queue_redraw();
			update_desired_size();
			update_configuration_warnings();
		} break;

		case NOTIFICATION_RESIZED: {
			_mark_lines_dirty();
		} break;
	}
}

RID Label::_get_line_rid(int p_line) const {
	if (p_line < 0 || p_line >= _total_line_count) {
		return RID();
	}

	int line_index = 0;
	for (const Paragraph &para : _paragraphs) {
		if (line_index + para.lines_rid.size() <= p_line) {
			line_index += para.lines_rid.size();
		} else {
			return para.lines_rid[p_line - line_index];
		}
	}

	return RID();
}

Rect2 Label::_get_line_rect(int p_line) const {
	if (p_line < 0 || p_line >= _total_line_count) {
		return Rect2();
	}

	int line_index = 0;
	for (int p = 0; p < _paragraphs.size(); p++) {
		const Paragraph &para = _paragraphs[p];
		if (line_index + para.lines_rid.size() <= p_line) {
			line_index += para.lines_rid.size();
		} else {
			return _get_paragraph_line_rect(p, p_line - line_index);
		}
	}

	return Rect2();
}

int Label::_get_layout_data(Vector2 &r_offset, int &r_last_line, int &r_line_spacing) const {
	// Computes several common parameters involved in laying out and rendering text set to this label.
	// Only vertical margin is considered in r_offset: use _get_line_rect() to get the horizontal offset
	// for a given line of text.
	const Size2 size = get_size();
	const Ref<StyleBox> style = theme_cache.normal_style;
	const Ref<Font> font = _get_font();
	const int font_h = font->get_height(get_rendered_font_size());
	const int line_spacing = _get_line_spacing();
	const int paragraph_spacing = _get_paragraph_spacing();
	const int lines_visible = get_visible_line_count();

	r_last_line = MIN(_total_line_count, lines_visible + _lines_skipped);

	// Get real total height.
	int total_glyphs = 0;
	float total_h = 0;
	int line_index = 0;
	for (const Paragraph &para : _paragraphs) {
		if (line_index + para.lines_rid.size() <= _lines_skipped) {
			line_index += para.lines_rid.size();
		} else {
			const int start = (line_index < _lines_skipped) ? _lines_skipped - line_index : 0;
			const int end = (line_index + para.lines_rid.size() < r_last_line) ? para.lines_rid.size() : r_last_line - line_index;
			if (end <= 0) {
				break;
			}
			for (int i = start; i < end; i++) {
				const LineMetrics metrics = _get_line_metrics(para.lines_rid[i], font_h);
				total_h += metrics.ascent + metrics.descent + line_spacing;
				total_glyphs += TS->shaped_text_get_glyph_count(para.lines_rid[i]) + TS->shaped_text_get_ellipsis_glyph_count(para.lines_rid[i]);
			}
			total_h += paragraph_spacing;
			line_index += para.lines_rid.size();
		}
	}
	total_h += style->get_content_inset(SIDE_TOP) + style->get_content_inset(SIDE_BOTTOM);
	int vbegin = 0, vsep = 0;
	if (lines_visible > 0) {
		switch (_vertical_alignment) {
			case VERTICAL_ALIGNMENT_TOP: {
				// Nothing.
			} break;
			case VERTICAL_ALIGNMENT_CENTER: {
				vbegin = (size.y - (total_h - line_spacing - paragraph_spacing)) / 2;
				vsep = 0;
			} break;
			case VERTICAL_ALIGNMENT_BOTTOM: {
				vbegin = size.y - (total_h - line_spacing - paragraph_spacing);
				vsep = 0;
			} break;
			case VERTICAL_ALIGNMENT_FILL: {
				vbegin = 0;
				if (lines_visible > 1) {
					vsep = (size.y - (total_h - line_spacing - paragraph_spacing)) / (lines_visible - 1);
				} else {
					vsep = 0;
				}
			} break;
		}
	}
	r_offset = { 0, style->get_content_offset().y + vbegin };
	r_line_spacing = line_spacing + vsep;

	return total_glyphs;
}

Ref<Font> Label::_get_font() const {
	if (_label_settings.is_valid() && _label_settings->get_font().is_valid()) {
		return _label_settings->get_font();
	}
	return theme_cache.font;
}

int Label::_get_base_font_size() const {
	return _label_settings.is_valid() ? _label_settings->get_font_size() : theme_cache.font_size;
}

int Label::_get_line_spacing() const {
	return _label_settings.is_valid() ? _label_settings->get_line_spacing() : theme_cache.line_spacing;
}

int Label::_get_paragraph_spacing() const {
	return _label_settings.is_valid() ? _label_settings->get_paragraph_spacing() : theme_cache.paragraph_spacing;
}

int Label::_get_maximum_width() const {
	const float maximum_width = get_combined_maximum_size().x;
	if (_autowrap_mode == TextServer::AUTOWRAP_OFF || maximum_width <= 0) {
		return -1;
	}
	return MAX(1, int(maximum_width - theme_cache.normal_style->get_minimum_size().width));
}

int Label::_get_shaping_width() const {
	const int width = get_size().width - theme_cache.normal_style->get_minimum_size().width;
	const int maximum_width = _get_maximum_width();
	if (maximum_width <= 0) {
		return width;
	}
	return width > 0 && !is_expanded_by_desired_size() ? MIN(width, maximum_width) : maximum_width;
}

Label::LineMetrics Label::_get_line_metrics(const RID &p_line, int p_font_height) {
	LineMetrics metrics;
	metrics.ascent = TS->shaped_text_get_ascent(p_line);
	metrics.descent = TS->shaped_text_get_descent(p_line);
	if (metrics.ascent + metrics.descent < p_font_height) {
		const double difference = p_font_height - (metrics.ascent + metrics.descent);
		metrics.ascent += difference / 2;
		metrics.descent += difference - (difference / 2);
	}
	return metrics;
}

int Label::_get_justification_line_count(const Paragraph &p_paragraph, BitField<TextServer::JustificationFlag> p_flags) {
	const int line_count = p_paragraph.lines_rid.size();
	if (line_count == 1 && p_flags.has_flag(TextServer::JUSTIFICATION_DO_NOT_SKIP_SINGLE_LINE)) {
		return line_count;
	}

	int justified_lines = p_flags.has_flag(TextServer::JUSTIFICATION_SKIP_LAST_LINE) ? line_count - 1 : line_count;
	if (p_flags.has_flag(TextServer::JUSTIFICATION_SKIP_LAST_LINE_WITH_VISIBLE_CHARS)) {
		for (int i = line_count - 1; i >= 0; i--) {
			if (TS->shaped_text_has_visible_chars(p_paragraph.lines_rid[i])) {
				justified_lines = i;
				break;
			}
		}
	}
	return justified_lines;
}

void Label::_clear_lines(Paragraph &r_paragraph) {
	for (const RID &line_rid : r_paragraph.lines_rid) {
		TS->free_rid(line_rid);
	}
	r_paragraph.lines_rid.clear();
}

void Label::_clear_paragraphs() const {
	for (Paragraph &paragraph : _paragraphs) {
		_clear_lines(paragraph);
		TS->free_rid(paragraph.text_rid);
	}
	_paragraphs.clear();
}

void Label::_mark_lines_dirty() const {
	for (Paragraph &paragraph : _paragraphs) {
		paragraph.lines_dirty = true;
	}
}

Rect2 Label::_get_paragraph_line_rect(int p_paragraph, int p_line) const {
	// Returns a rect providing the line's horizontal offset and total size. To determine the vertical
	// offset, use r_offset and r_line_spacing from _get_layout_data().
	const Ref<StyleBox> style = theme_cache.normal_style;
	const Ref<Font> font = _get_font();
	const int font_size = get_rendered_font_size();
	const int font_h = font->get_height(font_size);
	const Size2 size = get_size();
	const RID rid = _paragraphs[p_paragraph].lines_rid[p_line];
	Size2 line_size = TS->shaped_text_get_size(rid);
	const LineMetrics metrics = _get_line_metrics(rid, font_h);
	line_size.y = metrics.ascent + metrics.descent;
	Vector2 offset;
	switch (_horizontal_alignment) {
		case HORIZONTAL_ALIGNMENT_FILL:
			offset.x = style->get_content_offset().x;
			break;
		case HORIZONTAL_ALIGNMENT_LEFT: {
			offset.x = style->get_content_offset().x;
		} break;
		case HORIZONTAL_ALIGNMENT_CENTER: {
			offset.x = int(size.width - line_size.width) / 2;
		} break;
		case HORIZONTAL_ALIGNMENT_RIGHT: {
			offset.x = int(size.width - style->get_content_inset(SIDE_RIGHT) - line_size.width);
		} break;
	}
	return Rect2(offset, line_size);
}

void Label::_ensure_shaped() const {
	if (_font_dirty || _text_dirty) {
		_shape();
		return;
	}

	for (const Paragraph &para : _paragraphs) {
		if (para.lines_dirty || para.text_dirty) {
			_shape();
			return;
		}
	}
}

void Label::_update_visible() const {
	const int line_spacing = _get_line_spacing();
	const int paragraph_spacing = _get_paragraph_spacing();
	const Ref<Font> font = _get_font();
	const int font_size = get_rendered_font_size();
	const int font_h = font->get_height(font_size);
	int lines_visible = _total_line_count;

	if (_max_lines_visible >= 0 && lines_visible > _max_lines_visible) {
		lines_visible = _max_lines_visible;
	}

	_minimum_size.height = 0;
	const int last_line = MIN(_total_line_count, lines_visible + _lines_skipped);

	int line_index = 0;
	for (const Paragraph &para : _paragraphs) {
		if (line_index + para.lines_rid.size() <= _lines_skipped) {
			line_index += para.lines_rid.size();
		} else {
			const int start = (line_index < _lines_skipped) ? _lines_skipped - line_index : 0;
			const int end = (line_index + para.lines_rid.size() < last_line) ? para.lines_rid.size() : last_line - line_index;
			if (end <= 0) {
				break;
			}
			for (int i = start; i < end; i++) {
				const LineMetrics metrics = _get_line_metrics(para.lines_rid[i], font_h);
				_minimum_size.height += metrics.ascent + metrics.descent + line_spacing;
			}
			_minimum_size.height += paragraph_spacing;
			line_index += para.lines_rid.size();
		}
	}

	if (_minimum_size.height > 0) {
		_minimum_size.height -= (line_spacing + paragraph_spacing);
	}
}

void Label::_shape() const {
	const String &lang = _language.is_empty() ? _get_locale() : _language;

	const int width = _get_shaping_width();
	const int height = get_size().height - theme_cache.normal_style->get_minimum_size().height;

	if (_text_dirty) {
		_clear_paragraphs();

		String txt = _uppercase ? TS->string_to_upper(_translated_text, lang) : _translated_text;
		if (_visible_characters >= 0 && _visible_characters_behavior == TextServer::VC_CHARS_BEFORE_SHAPING) {
			txt = txt.substr(0, _visible_characters);
		}
		const String ps = _paragraph_separator.c_unescape();
		const Vector<String> para_text = txt.split(ps);
		int start = 0;
		for (const String &str : para_text) {
			Paragraph para;
			para.text_rid = TS->create_shaped_text();
			para.text = str + String::chr(0x200B);
			para.start = start;
			start += str.length() + ps.length();
			_paragraphs.push_back(para);
		}
		_text_dirty = false;
	}

	const int base_font_size = _get_base_font_size();
	if (_resize_font_to_fit && width > 0 && height > 0) {
		int low = _minimum_font_size;
		int high = _maximum_font_size;
		int best = low;

		while (low <= high) {
			const int mid = low + (high - low) / 2;
			if (_shape_lines(mid, width, height)) {
				best = mid;
				low = mid + 1;
			} else {
				high = mid - 1;
			}
		}
		_current_fitted_font_size = best;

		_font_dirty = true;
		_mark_lines_dirty();
		_shape_internal();
	} else {
		_current_fitted_font_size = base_font_size;
		_shape_internal();
	}
}

void Label::_shape_internal() const {
	const String &lang = _language.is_empty() ? _get_locale() : _language;

	const int width = _get_shaping_width();
	const int maximum_width = _get_maximum_width();
	const bool wrap_with_max_width = maximum_width > 0;

	_total_line_count = 0;
	for (Paragraph &para : _paragraphs) {
		if (para.text_dirty || _font_dirty) {
			if (para.text_dirty) {
				TS->shaped_text_clear(para.text_rid);
			}
			const Ref<Font> font = _get_font();
			const int font_size = get_rendered_font_size();
			ERR_FAIL_COND(font.is_null());

			if (para.text_dirty) {
				TS->shaped_text_add_string(para.text_rid, para.text, font->get_rids(), font_size, font->get_opentype_features(), lang);
			} else {
				const int spans = TS->shaped_get_span_count(para.text_rid);
				for (int i = 0; i < spans; i++) {
					TS->shaped_set_span_update_font(para.text_rid, i, font->get_rids(), font_size, font->get_opentype_features());
				}
			}
			if (!_tab_stops.is_empty()) {
				TS->shaped_text_tab_align(para.text_rid, _tab_stops);
			}
			para.text_dirty = false;
			para.lines_dirty = true;
		}

		if (para.lines_dirty) {
			_clear_lines(para);

			BitField<TextServer::LineBreakFlag> autowrap_flags = TextServer::BREAK_MANDATORY;
			switch (_autowrap_mode) {
				case TextServer::AUTOWRAP_WORD_SMART:
					autowrap_flags = TextServer::BREAK_WORD_BOUND | TextServer::BREAK_ADAPTIVE | TextServer::BREAK_MANDATORY;
					break;
				case TextServer::AUTOWRAP_WORD:
					autowrap_flags = TextServer::BREAK_WORD_BOUND | TextServer::BREAK_MANDATORY;
					break;
				case TextServer::AUTOWRAP_ARBITRARY:
					autowrap_flags = TextServer::BREAK_GRAPHEME_BOUND | TextServer::BREAK_MANDATORY;
					break;
				case TextServer::AUTOWRAP_OFF:
					break;
			}
			autowrap_flags = autowrap_flags | _autowrap_trim_flags;

			const PackedInt32Array line_breaks = TS->shaped_text_get_line_breaks(para.text_rid, width, 0, autowrap_flags);
			for (int i = 0; i < line_breaks.size(); i = i + 2) {
				const RID line = TS->shaped_text_substr(para.text_rid, line_breaks[i], line_breaks[i + 1] - line_breaks[i]);
				if (!_tab_stops.is_empty()) {
					TS->shaped_text_tab_align(line, _tab_stops);
				}
				para.lines_rid.push_back(line);
			}
		}
		_total_line_count += para.lines_rid.size();
	}
	_font_dirty = false;

	if (_translated_text.is_empty()) {
		_minimum_size = Size2(1, get_line_height());
		return;
	}

	const int visible_lines = get_visible_line_count();
	const bool lines_hidden = visible_lines > 0 && visible_lines < _total_line_count;

	int line_index = 0;
	if (_autowrap_mode == TextServer::AUTOWRAP_OFF || wrap_with_max_width) {
		_minimum_size.width = 0.0f;
	}
	for (Paragraph &para : _paragraphs) {
		if (_autowrap_mode == TextServer::AUTOWRAP_OFF) {
			for (const RID &line_rid : para.lines_rid) {
				if (_minimum_size.width < TS->shaped_text_get_size(line_rid).x) {
					_minimum_size.width = TS->shaped_text_get_size(line_rid).x;
				}
			}
		} else if (wrap_with_max_width) {
			_minimum_size.width = MAX(_minimum_size.width, TS->shaped_text_get_size(para.text_rid).x);
		}

		if (para.lines_dirty) {
			BitField<TextServer::TextOverrunFlag> overrun_flags = TextServer::get_overrun_flags_from_behavior(_text_overrun_behavior);

			// Fill after min_size calculation.

			BitField<TextServer::JustificationFlag> line_jst_flags = _justification_flags;
			if (!_tab_stops.is_empty()) {
				line_jst_flags.set_flag(TextServer::JUSTIFICATION_AFTER_LAST_TAB);
			}
			if (_autowrap_mode != TextServer::AUTOWRAP_OFF) {
				if (lines_hidden) {
					overrun_flags.set_flag(TextServer::OVERRUN_ENFORCE_ELLIPSIS);
				}
				if (_horizontal_alignment == HORIZONTAL_ALIGNMENT_FILL) {
					const int jst_to_line = _get_justification_line_count(para, line_jst_flags);
					for (int i = 0; i < para.lines_rid.size(); i++) {
						if (i < jst_to_line) {
							TS->shaped_text_fit_to_width(para.lines_rid[i], width, line_jst_flags);
						} else if (i == (visible_lines - line_index - 1)) {
							TS->shaped_text_set_custom_ellipsis(para.lines_rid[i], (_ellipsis_char.length() > 0) ? _ellipsis_char[0] : 0x2026);
							TS->shaped_text_overrun_trim_to_width(para.lines_rid[i], width, overrun_flags);
						}
					}
				} else if (lines_hidden && (visible_lines - line_index - 1 >= 0) && (visible_lines - line_index - 1) < para.lines_rid.size()) {
					TS->shaped_text_set_custom_ellipsis(para.lines_rid[visible_lines - line_index - 1], (_ellipsis_char.length() > 0) ? _ellipsis_char[0] : 0x2026);
					TS->shaped_text_overrun_trim_to_width(para.lines_rid[visible_lines - line_index - 1], width, overrun_flags);
				}
			} else {
				// Autowrap disabled.
				const int jst_to_line = _get_justification_line_count(para, line_jst_flags);
				for (int i = 0; i < para.lines_rid.size(); i++) {
					if (i < jst_to_line && _horizontal_alignment == HORIZONTAL_ALIGNMENT_FILL) {
						TS->shaped_text_fit_to_width(para.lines_rid[i], width, line_jst_flags);
						overrun_flags.set_flag(TextServer::OVERRUN_JUSTIFICATION_AWARE);
						TS->shaped_text_set_custom_ellipsis(para.lines_rid[i], (_ellipsis_char.length() > 0) ? _ellipsis_char[0] : 0x2026);
						TS->shaped_text_overrun_trim_to_width(para.lines_rid[i], width, overrun_flags);
						TS->shaped_text_fit_to_width(para.lines_rid[i], width, line_jst_flags | TextServer::JUSTIFICATION_CONSTRAIN_ELLIPSIS);
					} else {
						TS->shaped_text_set_custom_ellipsis(para.lines_rid[i], (_ellipsis_char.length() > 0) ? _ellipsis_char[0] : 0x2026);
						TS->shaped_text_overrun_trim_to_width(para.lines_rid[i], width, overrun_flags);
					}
				}
			}
			para.lines_dirty = false;
		}
		line_index += para.lines_rid.size();
	}
	if (wrap_with_max_width) {
		_minimum_size.width = MIN(_minimum_size.width, maximum_width);
	}

	_update_visible();

	if (_autowrap_mode == TextServer::AUTOWRAP_OFF || !_clip_text || _text_overrun_behavior == TextServer::OVERRUN_NO_TRIMMING) {
		const_cast<Label *>(this)->update_minimum_size();
	}
}

bool Label::_shape_lines(int p_font_size, int p_width, int p_height) const {
	_font_dirty = true;
	_mark_lines_dirty();
	_current_fitted_font_size = p_font_size;
	_shape_internal();

	if (_autowrap_mode == TextServer::AUTOWRAP_OFF && _minimum_size.width > p_width) {
		return false;
	}
	return _minimum_size.height <= p_height;
}

void Label::_draw() {
	if (_clip_text) {
		RenderingServer::get_singleton()->canvas_item_set_clip(get_canvas_item(), true);
	}

	// When a shaped text is invalidated by an external source, we want to reshape it.
	for (Paragraph &para : _paragraphs) {
		if (!TS->shaped_text_is_ready(para.text_rid)) {
			para.text_dirty = true;
		}

		for (const RID &line_rid : para.lines_rid) {
			if (!TS->shaped_text_is_ready(line_rid)) {
				para.lines_dirty = true;
				break;
			}
		}
	}

	_ensure_shaped();

	const RID ci = get_canvas_item();

	const bool has_settings = _label_settings.is_valid();

	const Ref<Font> font = _get_font();
	const int font_size = get_rendered_font_size();
	const int font_h = font->get_height(font_size);
	const Color font_color = has_settings ? _label_settings->get_font_color() : theme_cache.font_color;
	const Color font_shadow_color = has_settings ? _label_settings->get_shadow_color() : theme_cache.font_shadow_color;
	const Point2 shadow_ofs = has_settings ? _label_settings->get_shadow_offset() : theme_cache.font_shadow_offset;
	const int paragraph_spacing = _get_paragraph_spacing();
	const Color font_outline_color = has_settings ? _label_settings->get_outline_color() : theme_cache.font_outline_color;
	const int outline_size = has_settings ? _label_settings->get_outline_size() : theme_cache.font_outline_size;
	const int shadow_outline_size = has_settings ? _label_settings->get_shadow_size() : theme_cache.font_shadow_outline_size;
	const Vector<LabelSettings::StackedOutlineData> stacked_outline_datas = has_settings ? _label_settings->get_stacked_outline_data() : Vector<LabelSettings::StackedOutlineData>();
	const Vector<LabelSettings::StackedShadowData> stacked_shadow_datas = has_settings ? _label_settings->get_stacked_shadow_data() : Vector<LabelSettings::StackedShadowData>();

	if (has_focus(true)) {
		theme_cache.focus_style->draw(ci, Rect2(Point2(0, 0), get_size()));
	} else {
		theme_cache.normal_style->draw(ci, Rect2(Point2(0, 0), get_size()));
	}

	const bool trim_chars = (_visible_characters >= 0) && (_visible_characters_behavior == TextServer::VC_CHARS_AFTER_SHAPING);
	const bool trim_glyphs = (_visible_characters >= 0) && (_visible_characters_behavior == TextServer::VC_GLYPHS);

	Vector2 ofs;
	int line_spacing;
	int last_line;
	const int total_glyphs = _get_layout_data(ofs, last_line, line_spacing);
	int processed_glyphs = 0;
	const int visible_glyphs = total_glyphs * _visible_ratio;

	int line_index = 0;
	for (int p = 0; p < _paragraphs.size(); p++) {
		const Paragraph &para = _paragraphs[p];
		if (line_index + para.lines_rid.size() <= _lines_skipped) {
			line_index += para.lines_rid.size();
		} else {
			const int start = (line_index < _lines_skipped) ? _lines_skipped - line_index : 0;
			const int end = (line_index + para.lines_rid.size() < last_line) ? para.lines_rid.size() : last_line - line_index;
			if (end <= 0) {
				break;
			}
			for (int i = start; i < end; i++) {
				const RID line_rid = para.lines_rid[i];
				const Vector2 line_offset = _get_paragraph_line_rect(p, i).position;
				ofs.x = line_offset.x;

				const LineMetrics metrics = _get_line_metrics(line_rid, font_h);

				const Glyph *glyphs = TS->shaped_text_get_glyphs(line_rid);
				const int gl_size = TS->shaped_text_get_glyph_count(line_rid);

				const int ellipsis_pos = TS->shaped_text_get_ellipsis_pos(line_rid);
				const int trim_pos = TS->shaped_text_get_trim_pos(line_rid);

				const Glyph *ellipsis_glyphs = TS->shaped_text_get_ellipsis_glyphs(line_rid);
				const int ellipsis_gl_size = TS->shaped_text_get_ellipsis_glyph_count(line_rid);

				ofs.y += metrics.ascent;

				// Draw text effects and main texts. Note: Do not merge this into the single loop iteration, to prevent overlaps.
				auto draw_text = [&](const Color &p_color, auto p_draw_func, const auto &...p_args) {
					int processed_glyphs_step = processed_glyphs;
					Vector2 offset = ofs;
					auto draw_glyphs = [&](const Glyph *p_glyphs, int p_count) {
						for (int j = 0; j < p_count; j++) {
							const Glyph &glyph = p_glyphs[j];
							for (int k = 0; k < glyph.repeat; k++) {
								const bool skip = (trim_chars && glyph.end + para.start > _visible_characters) || (trim_glyphs && processed_glyphs_step >= visible_glyphs);
								if (!skip) {
									p_draw_func(glyph, ci, p_color, offset, p_args...);
								}
								processed_glyphs_step++;
								offset.x += glyph.advance;
							}
						}
					};
					draw_glyphs(glyphs, trim_pos >= 0 ? MIN(gl_size, trim_pos) : gl_size);
					if (ellipsis_pos >= 0) {
						draw_glyphs(ellipsis_glyphs, ellipsis_gl_size);
					}
					return processed_glyphs_step;
				};

				// Draw shadow outline.
				if (font_shadow_color.a != 0 && shadow_outline_size > 0) {
					draw_text(font_shadow_color, draw_glyph_shadow_outline, shadow_outline_size, shadow_ofs);
				}

				// Draw shadow.
				if (font_shadow_color.a > 0) {
					draw_text(font_shadow_color, draw_glyph_shadow, shadow_ofs);
				}

				// Draw stacked shadow.
				if (stacked_shadow_datas.size() != 0) {
					const int draw_iterations = stacked_shadow_datas.size();

					for (int draw_iteration_index = draw_iterations - 1; draw_iteration_index >= 0; --draw_iteration_index) {
						const LabelSettings::StackedShadowData stacked_shadow_data = stacked_shadow_datas[draw_iteration_index];
						if (stacked_shadow_data.outline_size > 0) {
							draw_text(stacked_shadow_data.color, draw_glyph_shadow_outline, stacked_shadow_data.outline_size, stacked_shadow_data.offset);
						}

						draw_text(stacked_shadow_data.color, draw_glyph_shadow, stacked_shadow_data.offset);
					}
				}

				// Draw stacked outline.
				if (stacked_outline_datas.size() != 0) {
					int stacked_outline_draw_size = outline_size;

					const int draw_iterations = stacked_outline_datas.size();

					for (int j = 0; j < draw_iterations; j++) {
						const int stacked_outline_size = stacked_outline_datas[j].size;
						if (stacked_outline_size <= 0) {
							continue;
						}
						stacked_outline_draw_size += stacked_outline_size;
					}

					for (int draw_iteration_index = draw_iterations - 1; draw_iteration_index >= 0; --draw_iteration_index) {
						const LabelSettings::StackedOutlineData stacked_outline_data = stacked_outline_datas[draw_iteration_index];
						if (stacked_outline_data.size <= 0) {
							continue;
						}
						draw_text(stacked_outline_data.color, draw_glyph_outline, stacked_outline_draw_size);
						stacked_outline_draw_size -= stacked_outline_data.size;
					}
				}

				// Draw outline.
				if (outline_size > 0 && font_outline_color.a != 0) {
					draw_text(font_outline_color, draw_glyph_outline, outline_size);
				}

				// Draw text.
				processed_glyphs = draw_text(font_color, draw_glyph);
				ofs.y += metrics.descent + line_spacing;
			}
			ofs.y += paragraph_spacing;
			line_index += para.lines_rid.size();
		}
	}
}

void Label::_invalidate() {
	_font_dirty = true;
	queue_redraw();
	update_configuration_warnings();
}

void Label::_maximum_size_changed() {
	if (_autowrap_mode == TextServer::AUTOWRAP_OFF && _text_overrun_behavior == TextServer::OVERRUN_NO_TRIMMING) {
		return;
	}

	_mark_lines_dirty();
	queue_redraw();
	update_minimum_size();
	update_desired_size();
	update_configuration_warnings();
}
