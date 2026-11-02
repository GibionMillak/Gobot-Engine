/**************************************************************************/
/*  label.h                                                               */
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

#include "scene/gui/control.h"
#include "servers/text/text_server.h"

class Font;
class LabelSettings;
class StyleBox;

class Label : public Control {
	GDCLASS(Label, Control);

public:
	explicit Label(const String &p_text = String());
	~Label();

	Size2 get_minimum_size() const override;
	Size2 get_desired_size() const override;
	PackedStringArray get_configuration_warnings() const override;

	void set_horizontal_alignment(HorizontalAlignment p_alignment);
	HorizontalAlignment get_horizontal_alignment() const;

	void set_vertical_alignment(VerticalAlignment p_alignment);
	VerticalAlignment get_vertical_alignment() const;

	void set_text(const String &p_string);
	String get_text() const;

	void set_label_settings(const Ref<LabelSettings> &p_settings);
	Ref<LabelSettings> get_label_settings() const;

	void set_language(const String &p_language);
	String get_language() const;

	void set_paragraph_separator(const String &p_paragraph_separator);
	String get_paragraph_separator() const;

	void set_resize_font_to_fit(bool p_enabled);
	bool is_resize_font_to_fit_enabled() const;

	void set_minimum_font_size(int p_size);
	int get_minimum_font_size() const;

	void set_maximum_font_size(int p_size);
	int get_maximum_font_size() const;

	int get_rendered_font_size() const;

	void set_autowrap_mode(TextServer::AutowrapMode p_mode);
	TextServer::AutowrapMode get_autowrap_mode() const;

	void set_autowrap_trim_flags(BitField<TextServer::LineBreakFlag> p_flags);
	BitField<TextServer::LineBreakFlag> get_autowrap_trim_flags() const;

	void set_justification_flags(BitField<TextServer::JustificationFlag> p_flags);
	BitField<TextServer::JustificationFlag> get_justification_flags() const;

	void set_uppercase(bool p_uppercase);
	bool is_uppercase() const;

	void set_visible_characters_behavior(TextServer::VisibleCharactersBehavior p_behavior);
	TextServer::VisibleCharactersBehavior get_visible_characters_behavior() const;

	void set_visible_characters(int p_amount);
	int get_visible_characters() const;
	int get_total_character_count() const;

	void set_visible_ratio(float p_ratio);
	float get_visible_ratio() const;

	void set_clip_text(bool p_clip);
	bool is_clipping_text() const;

	void set_tab_stops(const PackedFloat32Array &p_tab_stops);
	PackedFloat32Array get_tab_stops() const;

	void set_text_overrun_behavior(TextServer::OverrunBehavior p_behavior);
	TextServer::OverrunBehavior get_text_overrun_behavior() const;

	void set_ellipsis_char(const String &p_char);
	String get_ellipsis_char() const;

	void set_lines_skipped(int p_lines);
	int get_lines_skipped() const;

	void set_max_lines_visible(int p_lines);
	int get_max_lines_visible() const;

	int get_line_height(int p_line = -1) const;
	int get_line_count() const;
	int get_visible_line_count() const;

	Rect2 get_character_bounds(int p_pos) const;

protected:
	static void _bind_methods();
	void _notification(int p_what);

	// These helpers read the current shaped layout without triggering reshaping.
	RID _get_line_rid(int p_line) const;
	Rect2 _get_line_rect(int p_line) const;
	int _get_layout_data(Vector2 &r_offset, int &r_last_line, int &r_line_spacing) const;

private:
	struct Paragraph {
		bool lines_dirty = true;
		bool text_dirty = true;
		int start = 0;

		String text;
		RID text_rid;
		Vector<RID> lines_rid;
	};

	struct LineMetrics {
		double ascent = 0;
		double descent = 0;
	};

	struct ThemeCache {
		Ref<StyleBox> normal_style;
		Ref<StyleBox> focus_style;
		Ref<Font> font;

		int font_size = 0;
		int line_spacing = 0;
		int paragraph_spacing = 0;
		Color font_color;
		Color font_shadow_color;
		Point2 font_shadow_offset;
		Color font_outline_color;
		int font_outline_size = 0;
		int font_shadow_outline_size = 0;
	};

	Ref<Font> _get_font() const;
	int _get_base_font_size() const;
	int _get_line_spacing() const;
	int _get_paragraph_spacing() const;
	int _get_maximum_width() const;
	int _get_shaping_width() const;
	static LineMetrics _get_line_metrics(const RID &p_line, int p_font_height);
	static int _get_justification_line_count(const Paragraph &p_paragraph, BitField<TextServer::JustificationFlag> p_flags);
	static void _clear_lines(Paragraph &r_paragraph);
	void _clear_paragraphs() const;
	void _mark_lines_dirty() const;
	Rect2 _get_paragraph_line_rect(int p_paragraph, int p_line) const;
	void _ensure_shaped() const;
	void _update_visible() const;
	void _shape() const;
	void _shape_internal() const;
	bool _shape_lines(int p_font_size, int p_width, int p_height) const;
	void _draw();
	void _invalidate();
	void _maximum_size_changed();

	HorizontalAlignment _horizontal_alignment = HORIZONTAL_ALIGNMENT_LEFT;
	VerticalAlignment _vertical_alignment = VERTICAL_ALIGNMENT_TOP;
	String _text;
	String _translated_text;
	TextServer::AutowrapMode _autowrap_mode = TextServer::AUTOWRAP_OFF;
	BitField<TextServer::LineBreakFlag> _autowrap_trim_flags = TextServer::BREAK_TRIM_START_EDGE_SPACES | TextServer::BREAK_TRIM_END_EDGE_SPACES;
	BitField<TextServer::JustificationFlag> _justification_flags = TextServer::JUSTIFICATION_WORD_BOUND | TextServer::JUSTIFICATION_KASHIDA | TextServer::JUSTIFICATION_SKIP_LAST_LINE | TextServer::JUSTIFICATION_DO_NOT_SKIP_SINGLE_LINE;
	bool _clip_text = false;
	String _ellipsis_char = U"…";
	TextServer::OverrunBehavior _text_overrun_behavior = TextServer::OVERRUN_NO_TRIMMING;
	mutable Size2 _minimum_size;
	bool _uppercase = false;

	mutable bool _font_dirty = true;
	mutable bool _text_dirty = true;
	mutable Vector<Paragraph> _paragraphs;
	mutable int _total_line_count = 0;
	String _paragraph_separator = "\\n";

	String _language;

	TextServer::VisibleCharactersBehavior _visible_characters_behavior = TextServer::VC_CHARS_BEFORE_SHAPING;
	int _visible_characters = -1;
	float _visible_ratio = 1.0;
	int _lines_skipped = 0;
	int _max_lines_visible = -1;
	PackedFloat32Array _tab_stops;

	bool _resize_font_to_fit = false;
	int _minimum_font_size = 10;
	int _maximum_font_size = 60;
	mutable int _current_fitted_font_size = -1;

	Ref<LabelSettings> _label_settings;
	// Theme binding macros require this member name.
	ThemeCache theme_cache;
};
