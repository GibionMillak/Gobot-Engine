/**************************************************************************/
/*  table.h                                                             */
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

#include "core/templates/vector.h"
#include "scene/gui/control.h"
#include "scene/resources/text_paragraph.h"
#include "scene/resources/texture.h"

class HBoxContainer;
class HScrollBar;
class VScrollBar;

class Table : public Control {
	GDCLASS(Table, Control);

public:
	enum ColumnWidthMode {
		FIXED,
		CONTENT,
		EXPAND,
	};

	enum CellRole {
		DATA,
		HEADER,
	};

	enum CellWrapMode {
		INHERIT,
		DISABLED,
		ENABLED,
	};

	Table();

	void set_column_count(int p_count);
	int get_column_count() const;
	int insert_column(int p_at = -1);
	void remove_column(int p_column);
	void set_column_width_mode(int p_column, ColumnWidthMode p_mode);
	ColumnWidthMode get_column_width_mode(int p_column) const;
	void set_column_width(int p_column, int p_width);
	int get_column_width(int p_column) const;
	void set_column_expand_ratio(int p_column, float p_ratio);
	float get_column_expand_ratio(int p_column) const;

	void set_row_count(int p_count);
	int get_row_count() const;
	int add_row();
	int insert_row(int p_at = -1);
	void remove_row(int p_row);
	void clear();

	void set_cell(int p_row, int p_column, const String &p_text, const Ref<Texture2D> &p_icon = Ref<Texture2D>());
	void clear_cell(int p_row, int p_column);
	void set_cell_text(int p_row, int p_column, const String &p_text);
	String get_cell_text(int p_row, int p_column) const;
	void set_cell_icon(int p_row, int p_column, const Ref<Texture2D> &p_icon);
	Ref<Texture2D> get_cell_icon(int p_row, int p_column) const;
	void set_cell_role(int p_row, int p_column, CellRole p_role);
	CellRole get_cell_role(int p_row, int p_column) const;
	// Span changes are atomic and never discard configured cells covered by the new span.
	[[nodiscard]] bool set_cell_span(int p_row, int p_column, int p_column_span, int p_row_span);
	Vector2i get_cell_span(int p_row, int p_column) const;
	Vector2i get_cell_owner(int p_row, int p_column) const;
	void set_cell_horizontal_alignment(int p_row, int p_column, HorizontalAlignment p_alignment);
	HorizontalAlignment get_cell_horizontal_alignment(int p_row, int p_column) const;
	void set_cell_vertical_alignment(int p_row, int p_column, VerticalAlignment p_alignment);
	VerticalAlignment get_cell_vertical_alignment(int p_row, int p_column) const;
	void set_cell_wrap_mode(int p_row, int p_column, CellWrapMode p_mode);
	CellWrapMode get_cell_wrap_mode(int p_row, int p_column) const;

	[[nodiscard]] bool set_frozen_rows(int p_count);
	int get_frozen_rows() const;
	void set_fit_content_height(bool p_enabled);
	bool is_fitting_content_height() const;
	void set_wrap_cells(bool p_enabled);
	bool is_wrapping_cells() const;
	void set_show_row_count(bool p_enabled);
	bool is_row_count_visible() const;
	// Returns the internal footer container. Table owns its lifetime.
	HBoxContainer *get_footer();
	Vector2i get_cell_at_position(const Point2 &p_position) const;
	Rect2 get_cell_rect(int p_row, int p_column) const;

	Size2 get_minimum_size() const override;

protected:
	static void _bind_methods();
	void _notification(int p_what);
	void gui_input(const Ref<InputEvent> &p_event) override;
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_value) const;
	void _get_property_list(List<PropertyInfo> *r_list) const;

private:
	struct Cell {
		String text;
		Ref<Texture2D> icon;
		CellRole role = DATA;
		int column_span = 1;
		int row_span = 1;
		HorizontalAlignment horizontal_alignment = HORIZONTAL_ALIGNMENT_LEFT;
		VerticalAlignment vertical_alignment = VERTICAL_ALIGNMENT_CENTER;
		CellWrapMode wrap_mode = INHERIT;
		Ref<TextParagraph> text_buf;
	};

	struct Column {
		ColumnWidthMode width_mode = EXPAND;
		int width = 120;
		float expand_ratio = 1;
	};

	struct ThemeCache {
		Ref<StyleBox> background;
		Ref<StyleBox> header;
		Ref<StyleBox> cell;
		Ref<StyleBox> row_even;
		Ref<StyleBox> row_odd;
		Ref<StyleBox> footer;
		Ref<Font> font;
		int font_size = 0;
		Color font_color;
		Color header_font_color;
		int gap = 0;
		int horizontal_cell_padding = 0;
		int vertical_cell_padding = 0;
		int icon_gap = 0;
		int icon_max_width = 0;
	};

	bool _is_anchor(int p_row, int p_column) const;
	bool _is_empty(const Cell &p_cell) const;
	bool _validate_grid(const Vector<Vector<Cell>> &p_rows, int p_columns, int p_frozen_rows, Vector<Vector<Vector2i>> &r_owners) const;
	void _rebuild_owners();
	void _changed(bool p_structure = false);
	bool _has_footer_content() const;
	Size2 _get_footer_minimum_size() const;
	void _footer_child_entered_tree(Node *p_child);
	void _footer_child_exiting_tree(Node *p_child);
	void _queue_layout();
	void _update_layout();
	Size2 _get_icon_size(const Cell &p_cell) const;
	bool _is_wrapped(const Cell &p_cell) const;
	void _shape_cell(Cell &r_cell, float p_width);
	Rect2 _get_cell_content_rect(int p_row, int p_column) const;
	void _draw_cell(Control *p_canvas, const Cell &p_cell, const Rect2 &p_rect, int p_stripe);
	void _draw_frozen();
	void _draw_body();
	void _draw_region(Control *p_canvas, bool p_frozen);
	void _draw_footer();
	void _scroll_changed(double p_value);

	Vector<Column> _columns;
	Vector<Vector<Cell>> _rows;
	Vector<Vector<Vector2i>> _owners;
	Vector<float> _column_widths;
	Vector<float> _column_offsets;
	Vector<float> _row_heights;
	Vector<float> _row_offsets;
	Vector<int> _row_stripes;
	ThemeCache _theme_cache;
	Control *_frozen = nullptr;
	Control *_body = nullptr;
	Control *_footer_background = nullptr;
	HBoxContainer *_footer_hbox = nullptr;
	HScrollBar *_h_scroll = nullptr;
	VScrollBar *_v_scroll = nullptr;
	int _frozen_rows = 0;
	bool _fit_content_height = true;
	float _fitted_height = 0;
	bool _wrap_cells = true;
	bool _show_row_count = true;
	bool _layout_dirty = true;
	float _frozen_height = 0;
};

VARIANT_ENUM_CAST(Table::ColumnWidthMode);
VARIANT_ENUM_CAST(Table::CellRole);
VARIANT_ENUM_CAST(Table::CellWrapMode);
