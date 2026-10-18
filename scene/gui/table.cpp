/**************************************************************************/
/*  table.cpp                                                           */
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

#include "table.h"

#include "core/input/input_event.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "scene/gui/box_container.h"
#include "scene/gui/scroll_bar.h"
#include "scene/resources/font.h"
#include "scene/resources/style_box.h"
#include "scene/resources/texture.h"
#include "scene/theme/theme_db.h"

Table::Table() {
	set_clip_contents(true);

	_frozen = memnew(Control);
	_frozen->set_clip_contents(true);
	_frozen->set_mouse_filter(MOUSE_FILTER_IGNORE);
	add_child(_frozen, false, INTERNAL_MODE_FRONT);
	_frozen->connect("draw", callable_mp(this, &Table::_draw_frozen));

	_body = memnew(Control);
	_body->set_clip_contents(true);
	_body->set_mouse_filter(MOUSE_FILTER_IGNORE);
	add_child(_body, false, INTERNAL_MODE_FRONT);
	_body->connect("draw", callable_mp(this, &Table::_draw_body));

	_footer_background = memnew(Control);
	_footer_background->set_mouse_filter(MOUSE_FILTER_IGNORE);
	add_child(_footer_background, false, INTERNAL_MODE_FRONT);
	_footer_background->connect("draw", callable_mp(this, &Table::_draw_footer));

	_footer_hbox = memnew(HBoxContainer);
	_footer_hbox->set_alignment(BoxContainer::ALIGNMENT_END);
	_footer_hbox->set_name("Footer");
	add_child(_footer_hbox, false, INTERNAL_MODE_FRONT);
	_footer_hbox->connect("minimum_size_changed", callable_mp(this, &Table::_queue_layout));
	_footer_hbox->connect("visibility_changed", callable_mp(this, &Table::_queue_layout));
	_footer_hbox->connect("child_order_changed", callable_mp(this, &Table::_queue_layout));
	_footer_hbox->connect("child_entered_tree", callable_mp(this, &Table::_footer_child_entered_tree));
	_footer_hbox->connect("child_exiting_tree", callable_mp(this, &Table::_footer_child_exiting_tree));

	_h_scroll = memnew(HScrollBar);
	_h_scroll->set_step(1);
	add_child(_h_scroll, false, INTERNAL_MODE_FRONT);
	_h_scroll->connect("value_changed", callable_mp(this, &Table::_scroll_changed));

	_v_scroll = memnew(VScrollBar);
	_v_scroll->set_step(1);
	add_child(_v_scroll, false, INTERNAL_MODE_FRONT);
	_v_scroll->connect("value_changed", callable_mp(this, &Table::_scroll_changed));

	_columns.resize(2);
}

void Table::set_column_count(int p_count) {
	ERR_FAIL_COND(p_count < 1);
	if (p_count == _columns.size()) {
		return;
	}
	_columns.resize(p_count);
	for (int row = 0; row < _rows.size(); row++) {
		_rows.write[row].resize(p_count);
		for (int column = 0; column < p_count; column++) {
			Cell &cell = _rows.write[row].write[column];
			cell.column_span = MIN(cell.column_span, p_count - column);
		}
	}
	_rebuild_owners();
	_changed(true);
}

int Table::get_column_count() const {
	return _columns.size();
}

int Table::insert_column(int p_at) {
	const int at = p_at < 0 ? _columns.size() : p_at;
	ERR_FAIL_COND_V(at > _columns.size(), -1);
	Vector<Vector<Cell>> rows;
	rows.resize(_rows.size());
	for (int row = 0; row < rows.size(); row++) {
		rows.write[row].resize(_columns.size() + 1);
	}
	for (int row = 0; row < _rows.size(); row++) {
		for (int column = 0; column < _columns.size(); column++) {
			if (!_is_anchor(row, column)) {
				continue;
			}
			Cell cell = _rows[row][column];
			if (column < at && column + cell.column_span > at) {
				cell.column_span++;
			}
			rows.write[row].write[column >= at ? column + 1 : column] = cell;
		}
	}
	_columns.insert(at, Column());
	_rows = rows;
	_rebuild_owners();
	_changed(true);
	return at;
}

void Table::remove_column(int p_column) {
	ERR_FAIL_INDEX(p_column, _columns.size());
	ERR_FAIL_COND(_columns.size() == 1);
	Vector<Vector<Cell>> rows;
	rows.resize(_rows.size());
	for (int row = 0; row < rows.size(); row++) {
		rows.write[row].resize(_columns.size() - 1);
	}
	for (int row = 0; row < _rows.size(); row++) {
		for (int column = 0; column < _columns.size(); column++) {
			if (!_is_anchor(row, column)) {
				continue;
			}
			Cell cell = _rows[row][column];
			if (column <= p_column && column + cell.column_span > p_column) {
				if (--cell.column_span == 0) {
					continue;
				}
			}
			rows.write[row].write[column > p_column ? column - 1 : column] = cell;
		}
	}
	_columns.remove_at(p_column);
	_rows = rows;
	_rebuild_owners();
	_changed(true);
}

void Table::set_column_width_mode(int p_column, ColumnWidthMode p_mode) {
	ERR_FAIL_INDEX(p_column, _columns.size());
	ERR_FAIL_COND(p_mode < FIXED || p_mode > EXPAND);
	_columns.write[p_column].width_mode = p_mode;
	_changed();
}

Table::ColumnWidthMode Table::get_column_width_mode(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, _columns.size(), EXPAND);
	return _columns[p_column].width_mode;
}

void Table::set_column_width(int p_column, int p_width) {
	ERR_FAIL_INDEX(p_column, _columns.size());
	ERR_FAIL_COND(p_width < 1);
	_columns.write[p_column].width = p_width;
	_changed();
}

int Table::get_column_width(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, _columns.size(), 0);
	return _columns[p_column].width;
}

void Table::set_column_expand_ratio(int p_column, float p_ratio) {
	ERR_FAIL_INDEX(p_column, _columns.size());
	ERR_FAIL_COND(!Math::is_finite(p_ratio) || p_ratio <= 0);
	_columns.write[p_column].expand_ratio = p_ratio;
	_changed();
}

float Table::get_column_expand_ratio(int p_column) const {
	ERR_FAIL_INDEX_V(p_column, _columns.size(), 1);
	return _columns[p_column].expand_ratio;
}

void Table::set_row_count(int p_count) {
	ERR_FAIL_COND(p_count < 0);
	if (p_count == _rows.size()) {
		return;
	}
	_rows.resize(p_count);
	for (int row = 0; row < p_count; row++) {
		_rows.write[row].resize(_columns.size());
		for (int column = 0; column < _columns.size(); column++) {
			Cell &cell = _rows.write[row].write[column];
			cell.row_span = MIN(cell.row_span, p_count - row);
		}
	}
	_frozen_rows = MIN(_frozen_rows, p_count);
	_rebuild_owners();
	_changed(true);
}

int Table::get_row_count() const {
	return _rows.size();
}

int Table::add_row() {
	return insert_row();
}

int Table::insert_row(int p_at) {
	const int at = p_at < 0 ? _rows.size() : p_at;
	ERR_FAIL_COND_V(at > _rows.size(), -1);
	Vector<Vector<Cell>> rows;
	rows.resize(_rows.size() + 1);
	for (int row = 0; row < rows.size(); row++) {
		rows.write[row].resize(_columns.size());
	}
	for (int row = 0; row < _rows.size(); row++) {
		for (int column = 0; column < _columns.size(); column++) {
			if (!_is_anchor(row, column)) {
				continue;
			}
			Cell cell = _rows[row][column];
			if (row < at && row + cell.row_span > at) {
				cell.row_span++;
			}
			rows.write[row >= at ? row + 1 : row].write[column] = cell;
		}
	}
	if (at < _frozen_rows) {
		_frozen_rows++;
	}
	_rows = rows;
	_rebuild_owners();
	_changed(true);
	return at;
}

void Table::remove_row(int p_row) {
	ERR_FAIL_INDEX(p_row, _rows.size());
	Vector<Vector<Cell>> rows;
	rows.resize(_rows.size() - 1);
	for (int row = 0; row < rows.size(); row++) {
		rows.write[row].resize(_columns.size());
	}
	for (int row = 0; row < _rows.size(); row++) {
		for (int column = 0; column < _columns.size(); column++) {
			if (!_is_anchor(row, column)) {
				continue;
			}
			Cell cell = _rows[row][column];
			if (row <= p_row && row + cell.row_span > p_row) {
				if (--cell.row_span == 0) {
					continue;
				}
			}
			rows.write[row > p_row ? row - 1 : row].write[column] = cell;
		}
	}
	if (p_row < _frozen_rows) {
		_frozen_rows--;
	}
	_rows = rows;
	_rebuild_owners();
	_changed(true);
}

void Table::clear() {
	_rows.clear();
	_owners.clear();
	_frozen_rows = 0;
	_h_scroll->set_value(0);
	_v_scroll->set_value(0);
	_changed(true);
}

void Table::set_cell(int p_row, int p_column, const String &p_text, const Ref<Texture2D> &p_icon) {
	ERR_FAIL_COND(!_is_anchor(p_row, p_column));
	Cell &cell = _rows.write[p_row].write[p_column];
	cell.text = p_text;
	cell.icon = p_icon;
	_changed();
}

void Table::clear_cell(int p_row, int p_column) {
	ERR_FAIL_COND(!_is_anchor(p_row, p_column));
	const Cell &cell = _rows[p_row][p_column];
	const bool structure_changed = cell.column_span != 1 || cell.row_span != 1;
	_rows.write[p_row].write[p_column] = Cell();
	_rebuild_owners();
	_changed(structure_changed);
}

void Table::set_cell_text(int p_row, int p_column, const String &p_text) {
	ERR_FAIL_COND(!_is_anchor(p_row, p_column));
	_rows.write[p_row].write[p_column].text = p_text;
	_changed();
}

String Table::get_cell_text(int p_row, int p_column) const {
	ERR_FAIL_COND_V(!_is_anchor(p_row, p_column), String());
	return _rows[p_row][p_column].text;
}

void Table::set_cell_icon(int p_row, int p_column, const Ref<Texture2D> &p_icon) {
	ERR_FAIL_COND(!_is_anchor(p_row, p_column));
	_rows.write[p_row].write[p_column].icon = p_icon;
	_changed();
}

Ref<Texture2D> Table::get_cell_icon(int p_row, int p_column) const {
	ERR_FAIL_COND_V(!_is_anchor(p_row, p_column), Ref<Texture2D>());
	return _rows[p_row][p_column].icon;
}

void Table::set_cell_role(int p_row, int p_column, CellRole p_role) {
	ERR_FAIL_COND(!_is_anchor(p_row, p_column));
	ERR_FAIL_COND(p_role < DATA || p_role > HEADER);
	_rows.write[p_row].write[p_column].role = p_role;
	_changed();
}

Table::CellRole Table::get_cell_role(int p_row, int p_column) const {
	ERR_FAIL_COND_V(!_is_anchor(p_row, p_column), DATA);
	return _rows[p_row][p_column].role;
}

bool Table::set_cell_span(int p_row, int p_column, int p_column_span, int p_row_span) {
	ERR_FAIL_COND_V(!_is_anchor(p_row, p_column), false);
	ERR_FAIL_COND_V(p_column_span < 1 || p_row_span < 1, false);
	ERR_FAIL_COND_V(p_column_span > _columns.size() - p_column || p_row_span > _rows.size() - p_row, false);
	Vector<Vector<Cell>> rows = _rows;
	rows.write[p_row].write[p_column].column_span = p_column_span;
	rows.write[p_row].write[p_column].row_span = p_row_span;
	Vector<Vector<Vector2i>> owners;
	if (!_validate_grid(rows, _columns.size(), _frozen_rows, owners)) {
		return false;
	}
	_rows = rows;
	_owners = owners;
	_changed(true);
	return true;
}

Vector2i Table::get_cell_span(int p_row, int p_column) const {
	ERR_FAIL_COND_V(!_is_anchor(p_row, p_column), Vector2i());
	return Vector2i(_rows[p_row][p_column].column_span, _rows[p_row][p_column].row_span);
}

Vector2i Table::get_cell_owner(int p_row, int p_column) const {
	ERR_FAIL_INDEX_V(p_row, _rows.size(), Vector2i(-1, -1));
	ERR_FAIL_INDEX_V(p_column, _columns.size(), Vector2i(-1, -1));
	return _owners[p_row][p_column];
}

void Table::set_cell_horizontal_alignment(int p_row, int p_column, HorizontalAlignment p_horizontal_alignment) {
	ERR_FAIL_COND(!_is_anchor(p_row, p_column));
	ERR_FAIL_COND(p_horizontal_alignment < HORIZONTAL_ALIGNMENT_LEFT || p_horizontal_alignment > HORIZONTAL_ALIGNMENT_RIGHT);
	_rows.write[p_row].write[p_column].horizontal_alignment = p_horizontal_alignment;
	_changed();
}

HorizontalAlignment Table::get_cell_horizontal_alignment(int p_row, int p_column) const {
	ERR_FAIL_COND_V(!_is_anchor(p_row, p_column), HORIZONTAL_ALIGNMENT_LEFT);
	return _rows[p_row][p_column].horizontal_alignment;
}

void Table::set_cell_vertical_alignment(int p_row, int p_column, VerticalAlignment p_vertical_alignment) {
	ERR_FAIL_COND(!_is_anchor(p_row, p_column));
	ERR_FAIL_COND(p_vertical_alignment < VERTICAL_ALIGNMENT_TOP || p_vertical_alignment > VERTICAL_ALIGNMENT_BOTTOM);
	_rows.write[p_row].write[p_column].vertical_alignment = p_vertical_alignment;
	_changed();
}

VerticalAlignment Table::get_cell_vertical_alignment(int p_row, int p_column) const {
	ERR_FAIL_COND_V(!_is_anchor(p_row, p_column), VERTICAL_ALIGNMENT_CENTER);
	return _rows[p_row][p_column].vertical_alignment;
}

void Table::set_cell_wrap_mode(int p_row, int p_column, CellWrapMode p_wrap_mode) {
	ERR_FAIL_COND(!_is_anchor(p_row, p_column));
	ERR_FAIL_COND(p_wrap_mode < INHERIT || p_wrap_mode > ENABLED);
	_rows.write[p_row].write[p_column].wrap_mode = p_wrap_mode;
	_changed();
}

Table::CellWrapMode Table::get_cell_wrap_mode(int p_row, int p_column) const {
	ERR_FAIL_COND_V(!_is_anchor(p_row, p_column), INHERIT);
	return _rows[p_row][p_column].wrap_mode;
}

bool Table::set_frozen_rows(int p_count) {
	ERR_FAIL_COND_V(p_count < 0 || p_count > _rows.size(), false);
	Vector<Vector<Vector2i>> owners;
	if (!_validate_grid(_rows, _columns.size(), p_count, owners)) {
		return false;
	}
	_frozen_rows = p_count;
	_changed();
	return true;
}

int Table::get_frozen_rows() const {
	return _frozen_rows;
}

void Table::set_fit_content_height(bool p_enabled) {
	_fit_content_height = p_enabled;
	_fitted_height = 0;
	_changed();
}

bool Table::is_fitting_content_height() const {
	return _fit_content_height;
}

void Table::set_wrap_cells(bool p_enabled) {
	_wrap_cells = p_enabled;
	_changed();
}

bool Table::is_wrapping_cells() const {
	return _wrap_cells;
}

void Table::set_show_row_count(bool p_enabled) {
	if (_show_row_count == p_enabled) {
		return;
	}
	_show_row_count = p_enabled;
	_changed();
}

bool Table::is_row_count_visible() const {
	return _show_row_count;
}

HBoxContainer *Table::get_footer() {
	return _footer_hbox;
}

Vector2i Table::get_cell_at_position(const Point2 &p_position) const {
	if (_layout_dirty) {
		return Vector2i(-1, -1);
	}
	const bool frozen = Rect2(_frozen->get_position(), _frozen->get_size()).has_point(p_position);
	if (!frozen && !Rect2(_body->get_position(), _body->get_size()).has_point(p_position)) {
		return Vector2i(-1, -1);
	}
	Point2 local = p_position - (frozen ? _frozen->get_position() : _body->get_position());
	local.x = (is_layout_rtl() ? _body->get_size().x - local.x : local.x) + _h_scroll->get_value();
	local.y += frozen ? 0 : _frozen_height + _v_scroll->get_value();
	for (int row = 0; row < _rows.size(); row++) {
		for (int column = 0; column < _columns.size(); column++) {
			if (_is_anchor(row, column) && _get_cell_content_rect(row, column).has_point(local)) {
				return Vector2i(column, row);
			}
		}
	}
	return Vector2i(-1, -1);
}

Rect2 Table::get_cell_rect(int p_row, int p_column) const {
	ERR_FAIL_COND_V(!_is_anchor(p_row, p_column), Rect2());
	if (_layout_dirty || _row_offsets.size() != _rows.size()) {
		return Rect2();
	}
	Rect2 rect = _get_cell_content_rect(p_row, p_column);
	rect.position.x -= _h_scroll->get_value();
	if (is_layout_rtl()) {
		rect.position.x = _body->get_size().x - rect.position.x - rect.size.x;
	}
	if (p_row < _frozen_rows) {
		rect.position += _frozen->get_position();
	} else {
		rect.position.y -= _frozen_height + _v_scroll->get_value();
		rect.position += _body->get_position();
	}
	return rect;
}

Size2 Table::get_minimum_size() const {
	Size2 minimum = _theme_cache.background.is_valid() ? _theme_cache.background->get_minimum_size() : Size2();
	if (_theme_cache.font.is_valid()) {
		const float row_height = _theme_cache.font->get_height(_theme_cache.font_size) + MAX(0, _theme_cache.vertical_cell_padding) * 2;
		minimum.y += _fit_content_height ? MAX(_fitted_height, row_height * _rows.size()) : row_height * MAX(1, _frozen_rows + 1);
	}
	if (_v_scroll && _h_scroll) {
		minimum.x += _v_scroll->get_combined_minimum_size().x;
		if (!_fit_content_height) {
			minimum.y += _h_scroll->get_combined_minimum_size().y;
		}
	}
	const Size2 footer_minimum = _get_footer_minimum_size();
	minimum.x = MAX(minimum.x, footer_minimum.x + (_theme_cache.background.is_valid() ? _theme_cache.background->get_minimum_size().x : 0));
	minimum.y += footer_minimum.y + (_rows.is_empty() ? 0 : MAX(0, _theme_cache.gap));
	return minimum;
}

void Table::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_column_count", "column_count"), &Table::set_column_count);
	ClassDB::bind_method(D_METHOD("get_column_count"), &Table::get_column_count);
	ClassDB::bind_method(D_METHOD("insert_column", "at"), &Table::insert_column, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("remove_column", "column"), &Table::remove_column);
	ClassDB::bind_method(D_METHOD("set_column_width_mode", "column", "mode"), &Table::set_column_width_mode);
	ClassDB::bind_method(D_METHOD("get_column_width_mode", "column"), &Table::get_column_width_mode);
	ClassDB::bind_method(D_METHOD("set_column_width", "column", "width"), &Table::set_column_width);
	ClassDB::bind_method(D_METHOD("get_column_width", "column"), &Table::get_column_width);
	ClassDB::bind_method(D_METHOD("set_column_expand_ratio", "column", "ratio"), &Table::set_column_expand_ratio);
	ClassDB::bind_method(D_METHOD("get_column_expand_ratio", "column"), &Table::get_column_expand_ratio);
	ClassDB::bind_method(D_METHOD("set_row_count", "row_count"), &Table::set_row_count);
	ClassDB::bind_method(D_METHOD("get_row_count"), &Table::get_row_count);
	ClassDB::bind_method(D_METHOD("add_row"), &Table::add_row);
	ClassDB::bind_method(D_METHOD("insert_row", "at"), &Table::insert_row, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("remove_row", "row"), &Table::remove_row);
	ClassDB::bind_method(D_METHOD("clear"), &Table::clear);
	ClassDB::bind_method(D_METHOD("set_cell", "row", "column", "text", "icon"), &Table::set_cell, DEFVAL(Ref<Texture2D>()));
	ClassDB::bind_method(D_METHOD("clear_cell", "row", "column"), &Table::clear_cell);
	ClassDB::bind_method(D_METHOD("set_cell_text", "row", "column", "text"), &Table::set_cell_text);
	ClassDB::bind_method(D_METHOD("get_cell_text", "row", "column"), &Table::get_cell_text);
	ClassDB::bind_method(D_METHOD("set_cell_icon", "row", "column", "icon"), &Table::set_cell_icon);
	ClassDB::bind_method(D_METHOD("get_cell_icon", "row", "column"), &Table::get_cell_icon);
	ClassDB::bind_method(D_METHOD("set_cell_role", "row", "column", "role"), &Table::set_cell_role);
	ClassDB::bind_method(D_METHOD("get_cell_role", "row", "column"), &Table::get_cell_role);
	ClassDB::bind_method(D_METHOD("set_cell_span", "row", "column", "column_span", "row_span"), &Table::set_cell_span);
	ClassDB::bind_method(D_METHOD("get_cell_span", "row", "column"), &Table::get_cell_span);
	ClassDB::bind_method(D_METHOD("get_cell_owner", "row", "column"), &Table::get_cell_owner);
	ClassDB::bind_method(D_METHOD("set_cell_horizontal_alignment", "row", "column", "horizontal_alignment"), &Table::set_cell_horizontal_alignment);
	ClassDB::bind_method(D_METHOD("get_cell_horizontal_alignment", "row", "column"), &Table::get_cell_horizontal_alignment);
	ClassDB::bind_method(D_METHOD("set_cell_vertical_alignment", "row", "column", "vertical_alignment"), &Table::set_cell_vertical_alignment);
	ClassDB::bind_method(D_METHOD("get_cell_vertical_alignment", "row", "column"), &Table::get_cell_vertical_alignment);
	ClassDB::bind_method(D_METHOD("set_cell_wrap_mode", "row", "column", "wrap_mode"), &Table::set_cell_wrap_mode);
	ClassDB::bind_method(D_METHOD("get_cell_wrap_mode", "row", "column"), &Table::get_cell_wrap_mode);
	ClassDB::bind_method(D_METHOD("set_frozen_rows", "frozen_rows"), &Table::set_frozen_rows);
	ClassDB::bind_method(D_METHOD("get_frozen_rows"), &Table::get_frozen_rows);
	ClassDB::bind_method(D_METHOD("set_fit_content_height", "fit_content_height"), &Table::set_fit_content_height);
	ClassDB::bind_method(D_METHOD("is_fitting_content_height"), &Table::is_fitting_content_height);
	ClassDB::bind_method(D_METHOD("set_wrap_cells", "wrap_cells"), &Table::set_wrap_cells);
	ClassDB::bind_method(D_METHOD("is_wrapping_cells"), &Table::is_wrapping_cells);
	ClassDB::bind_method(D_METHOD("set_show_row_count", "show_row_count"), &Table::set_show_row_count);
	ClassDB::bind_method(D_METHOD("is_row_count_visible"), &Table::is_row_count_visible);
	ClassDB::bind_method(D_METHOD("get_footer"), &Table::get_footer);
	ClassDB::bind_method(D_METHOD("get_cell_at_position", "position"), &Table::get_cell_at_position);
	ClassDB::bind_method(D_METHOD("get_cell_rect", "row", "column"), &Table::get_cell_rect);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "column_count", PROPERTY_HINT_RANGE, "1,1024,1,or_greater"), "set_column_count", "get_column_count");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "row_count", PROPERTY_HINT_RANGE, "0,1024,1,or_greater"), "set_row_count", "get_row_count");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "frozen_rows", PROPERTY_HINT_RANGE, "0,1024,1,or_greater"), "set_frozen_rows", "get_frozen_rows");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "fit_content_height"), "set_fit_content_height", "is_fitting_content_height");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "wrap_cells"), "set_wrap_cells", "is_wrapping_cells");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_row_count"), "set_show_row_count", "is_row_count_visible");

	BIND_ENUM_CONSTANT(FIXED);
	BIND_ENUM_CONSTANT(CONTENT);
	BIND_ENUM_CONSTANT(EXPAND);
	BIND_ENUM_CONSTANT(DATA);
	BIND_ENUM_CONSTANT(HEADER);
	BIND_ENUM_CONSTANT(INHERIT);
	BIND_ENUM_CONSTANT(DISABLED);
	BIND_ENUM_CONSTANT(ENABLED);

	ADD_SIGNAL(MethodInfo("table_changed"));
	ADD_SIGNAL(MethodInfo("cell_layout_changed"));
	ADD_SIGNAL(MethodInfo("cell_clicked", PropertyInfo(Variant::INT, "row"), PropertyInfo(Variant::INT, "column")));

	ThemeDB::get_singleton()->bind_class_item(Theme::DATA_TYPE_STYLEBOX, get_class_static(), "background", "background",
			[](Node *p_instance, const StringName &p_item_name, const StringName &p_type_name) {
				Table *table = Object::cast_to<Table>(p_instance);
				table->_theme_cache.background = table->get_theme_item(Theme::DATA_TYPE_STYLEBOX, p_item_name, p_type_name);
			});
	ThemeDB::get_singleton()->bind_class_item(Theme::DATA_TYPE_STYLEBOX, get_class_static(), "header", "header",
			[](Node *p_instance, const StringName &p_item_name, const StringName &p_type_name) {
				Table *table = Object::cast_to<Table>(p_instance);
				table->_theme_cache.header = table->get_theme_item(Theme::DATA_TYPE_STYLEBOX, p_item_name, p_type_name);
			});
	ThemeDB::get_singleton()->bind_class_item(Theme::DATA_TYPE_STYLEBOX, get_class_static(), "cell", "cell",
			[](Node *p_instance, const StringName &p_item_name, const StringName &p_type_name) {
				Table *table = Object::cast_to<Table>(p_instance);
				table->_theme_cache.cell = table->get_theme_item(Theme::DATA_TYPE_STYLEBOX, p_item_name, p_type_name);
			});
	ThemeDB::get_singleton()->bind_class_item(Theme::DATA_TYPE_STYLEBOX, get_class_static(), "row_even", "row_even",
			[](Node *p_instance, const StringName &p_item_name, const StringName &p_type_name) {
				Table *table = Object::cast_to<Table>(p_instance);
				table->_theme_cache.row_even = table->get_theme_item(Theme::DATA_TYPE_STYLEBOX, p_item_name, p_type_name);
			});
	ThemeDB::get_singleton()->bind_class_item(Theme::DATA_TYPE_STYLEBOX, get_class_static(), "row_odd", "row_odd",
			[](Node *p_instance, const StringName &p_item_name, const StringName &p_type_name) {
				Table *table = Object::cast_to<Table>(p_instance);
				table->_theme_cache.row_odd = table->get_theme_item(Theme::DATA_TYPE_STYLEBOX, p_item_name, p_type_name);
			});
	ThemeDB::get_singleton()->bind_class_item(Theme::DATA_TYPE_STYLEBOX, get_class_static(), "footer", "footer",
			[](Node *p_instance, const StringName &p_item_name, const StringName &p_type_name) {
				Table *table = Object::cast_to<Table>(p_instance);
				table->_theme_cache.footer = table->get_theme_item(Theme::DATA_TYPE_STYLEBOX, p_item_name, p_type_name);
			});
	ThemeDB::get_singleton()->bind_class_item(Theme::DATA_TYPE_FONT, get_class_static(), "font", "font",
			[](Node *p_instance, const StringName &p_item_name, const StringName &p_type_name) {
				Table *table = Object::cast_to<Table>(p_instance);
				table->_theme_cache.font = table->get_theme_item(Theme::DATA_TYPE_FONT, p_item_name, p_type_name);
			});
	ThemeDB::get_singleton()->bind_class_item(Theme::DATA_TYPE_FONT_SIZE, get_class_static(), "font_size", "font_size",
			[](Node *p_instance, const StringName &p_item_name, const StringName &p_type_name) {
				Table *table = Object::cast_to<Table>(p_instance);
				table->_theme_cache.font_size = table->get_theme_item(Theme::DATA_TYPE_FONT_SIZE, p_item_name, p_type_name);
			});
	ThemeDB::get_singleton()->bind_class_item(Theme::DATA_TYPE_COLOR, get_class_static(), "font_color", "font_color",
			[](Node *p_instance, const StringName &p_item_name, const StringName &p_type_name) {
				Table *table = Object::cast_to<Table>(p_instance);
				table->_theme_cache.font_color = table->get_theme_item(Theme::DATA_TYPE_COLOR, p_item_name, p_type_name);
			});
	ThemeDB::get_singleton()->bind_class_item(Theme::DATA_TYPE_COLOR, get_class_static(), "header_font_color", "header_font_color",
			[](Node *p_instance, const StringName &p_item_name, const StringName &p_type_name) {
				Table *table = Object::cast_to<Table>(p_instance);
				table->_theme_cache.header_font_color = table->get_theme_item(Theme::DATA_TYPE_COLOR, p_item_name, p_type_name);
			});
	ThemeDB::get_singleton()->bind_class_item(Theme::DATA_TYPE_CONSTANT, get_class_static(), "gap", "gap",
			[](Node *p_instance, const StringName &p_item_name, const StringName &p_type_name) {
				Table *table = Object::cast_to<Table>(p_instance);
				table->_theme_cache.gap = table->get_theme_item(Theme::DATA_TYPE_CONSTANT, p_item_name, p_type_name);
			});
	ThemeDB::get_singleton()->bind_class_item(Theme::DATA_TYPE_CONSTANT, get_class_static(), "horizontal_cell_padding", "horizontal_cell_padding",
			[](Node *p_instance, const StringName &p_item_name, const StringName &p_type_name) {
				Table *table = Object::cast_to<Table>(p_instance);
				table->_theme_cache.horizontal_cell_padding = table->get_theme_item(Theme::DATA_TYPE_CONSTANT, p_item_name, p_type_name);
			});
	ThemeDB::get_singleton()->bind_class_item(Theme::DATA_TYPE_CONSTANT, get_class_static(), "vertical_cell_padding", "vertical_cell_padding",
			[](Node *p_instance, const StringName &p_item_name, const StringName &p_type_name) {
				Table *table = Object::cast_to<Table>(p_instance);
				table->_theme_cache.vertical_cell_padding = table->get_theme_item(Theme::DATA_TYPE_CONSTANT, p_item_name, p_type_name);
			});
	ThemeDB::get_singleton()->bind_class_item(Theme::DATA_TYPE_CONSTANT, get_class_static(), "icon_gap", "icon_gap",
			[](Node *p_instance, const StringName &p_item_name, const StringName &p_type_name) {
				Table *table = Object::cast_to<Table>(p_instance);
				table->_theme_cache.icon_gap = table->get_theme_item(Theme::DATA_TYPE_CONSTANT, p_item_name, p_type_name);
			});
	ThemeDB::get_singleton()->bind_class_item(Theme::DATA_TYPE_CONSTANT, get_class_static(), "icon_max_width", "icon_max_width",
			[](Node *p_instance, const StringName &p_item_name, const StringName &p_type_name) {
				Table *table = Object::cast_to<Table>(p_instance);
				table->_theme_cache.icon_max_width = table->get_theme_item(Theme::DATA_TYPE_CONSTANT, p_item_name, p_type_name);
			});
}

void Table::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED:
		case NOTIFICATION_ENTER_TREE:
		case NOTIFICATION_RESIZED:
		case NOTIFICATION_TRANSLATION_CHANGED:
		case NOTIFICATION_LAYOUT_DIRECTION_CHANGED: {
			_queue_layout();
		} break;
		case NOTIFICATION_DRAW: {
			if (_layout_dirty) {
				_update_layout();
			}
			if (_theme_cache.background.is_valid()) {
				draw_style_box(_theme_cache.background, Rect2(Point2(), get_size()));
			}
		} break;
	}
}

void Table::gui_input(const Ref<InputEvent> &p_event) {
	const Ref<InputEventMouseButton> mouse_button = p_event;
	if (mouse_button.is_null() || !mouse_button->is_pressed()) {
		return;
	}
	if (mouse_button->get_button_index() == MouseButton::LEFT) {
		const Vector2i cell = get_cell_at_position(mouse_button->get_position());
		if (cell.x >= 0) {
			emit_signal("cell_clicked", cell.y, cell.x);
		}
		return;
	}
	ScrollBar *scroll = nullptr;
	float direction = 0;
	switch (mouse_button->get_button_index()) {
		case MouseButton::WHEEL_UP:
		case MouseButton::WHEEL_DOWN: {
			scroll = mouse_button->is_shift_pressed() || !_v_scroll->is_visible() ? static_cast<ScrollBar *>(_h_scroll) : _v_scroll;
			direction = mouse_button->get_button_index() == MouseButton::WHEEL_UP ? -1 : 1;
		} break;
		case MouseButton::WHEEL_LEFT:
		case MouseButton::WHEEL_RIGHT: {
			scroll = _h_scroll;
			direction = mouse_button->get_button_index() == MouseButton::WHEEL_LEFT ? -1 : 1;
		} break;
		default:
			return;
	}
	const double previous = scroll->get_value();
	scroll->set_value(previous + direction * MAX(1.0, scroll->get_page() / 8) * mouse_button->get_factor());
	if (scroll->get_value() != previous) {
		accept_event();
	}
}

bool Table::_set(const StringName &p_name, const Variant &p_value) {
	const String property = p_name;
	if (property.begins_with("columns/")) {
		if (property.get_slice_count("/") != 3) {
			return false;
		}
		const String index = property.get_slicec('/', 1).trim_prefix("column_");
		if (!index.is_valid_int()) {
			return false;
		}
		const int column = index.to_int();
		ERR_FAIL_INDEX_V(column, _columns.size(), false);
		const String field = property.get_slicec('/', 2);
		if (field == "width_mode") {
			set_column_width_mode(column, ColumnWidthMode(int(p_value)));
		} else if (field == "width") {
			set_column_width(column, p_value);
		} else if (field == "expand_ratio") {
			set_column_expand_ratio(column, p_value);
		} else {
			return false;
		}
		return true;
	}
	if (!property.begins_with("rows/") || property.get_slice_count("/") != 4) {
		return false;
	}
	const String row_index = property.get_slicec('/', 1).trim_prefix("row_");
	const String column_index = property.get_slicec('/', 2).trim_prefix("cell_");
	if (!row_index.is_valid_int() || !column_index.is_valid_int()) {
		return false;
	}
	const int row = row_index.to_int();
	const int column = column_index.to_int();
	ERR_FAIL_COND_V(!_is_anchor(row, column), false);
	const String field = property.get_slicec('/', 3);
	if (field == "text") {
		set_cell_text(row, column, p_value);
	} else if (field == "icon") {
		set_cell_icon(row, column, p_value);
	} else if (field == "role") {
		set_cell_role(row, column, CellRole(int(p_value)));
	} else if (field == "horizontal_alignment") {
		set_cell_horizontal_alignment(row, column, HorizontalAlignment(int(p_value)));
	} else if (field == "vertical_alignment") {
		set_cell_vertical_alignment(row, column, VerticalAlignment(int(p_value)));
	} else if (field == "wrap_mode") {
		set_cell_wrap_mode(row, column, CellWrapMode(int(p_value)));
	} else if (field == "span") {
		const Vector2i span = p_value;
		if (!set_cell_span(row, column, span.x, span.y)) {
			ERR_PRINT("Cannot apply cell span: it overlaps configured cells, exceeds the grid, or crosses the frozen-row boundary.");
		}
	} else {
		return false;
	}
	return true;
}

bool Table::_get(const StringName &p_name, Variant &r_value) const {
	const String property = p_name;
	if (property.begins_with("columns/")) {
		if (property.get_slice_count("/") != 3) {
			return false;
		}
		const String index = property.get_slicec('/', 1).trim_prefix("column_");
		if (!index.is_valid_int()) {
			return false;
		}
		const int column = index.to_int();
		ERR_FAIL_INDEX_V(column, _columns.size(), false);
		const String field = property.get_slicec('/', 2);
		if (field == "width_mode") {
			r_value = get_column_width_mode(column);
		} else if (field == "width") {
			r_value = get_column_width(column);
		} else if (field == "expand_ratio") {
			r_value = get_column_expand_ratio(column);
		} else {
			return false;
		}
		return true;
	}
	if (!property.begins_with("rows/") || property.get_slice_count("/") != 4) {
		return false;
	}
	const String row_index = property.get_slicec('/', 1).trim_prefix("row_");
	const String column_index = property.get_slicec('/', 2).trim_prefix("cell_");
	if (!row_index.is_valid_int() || !column_index.is_valid_int()) {
		return false;
	}
	const int row = row_index.to_int();
	const int column = column_index.to_int();
	ERR_FAIL_COND_V(!_is_anchor(row, column), false);
	const String field = property.get_slicec('/', 3);
	if (field == "text") {
		r_value = get_cell_text(row, column);
	} else if (field == "icon") {
		r_value = get_cell_icon(row, column);
	} else if (field == "role") {
		r_value = get_cell_role(row, column);
	} else if (field == "horizontal_alignment") {
		r_value = get_cell_horizontal_alignment(row, column);
	} else if (field == "vertical_alignment") {
		r_value = get_cell_vertical_alignment(row, column);
	} else if (field == "wrap_mode") {
		r_value = get_cell_wrap_mode(row, column);
	} else if (field == "span") {
		r_value = get_cell_span(row, column);
	} else {
		return false;
	}
	return true;
}

void Table::_get_property_list(List<PropertyInfo> *r_list) const {
	for (int column = 0; column < _columns.size(); column++) {
		const String prefix = "columns/column_" + itos(column) + "/";
		r_list->push_back(PropertyInfo(Variant::INT, prefix + "width_mode", PROPERTY_HINT_ENUM, "Fixed,Content,Expand"));
		r_list->push_back(PropertyInfo(Variant::INT, prefix + "width", PROPERTY_HINT_RANGE, "1,4096,1,or_greater"));
		r_list->push_back(PropertyInfo(Variant::FLOAT, prefix + "expand_ratio", PROPERTY_HINT_RANGE, "0.01,100,0.01,or_greater"));
	}
	for (int row = 0; row < _rows.size(); row++) {
		for (int column = 0; column < _columns.size(); column++) {
			if (!_is_anchor(row, column)) {
				continue;
			}
			const String prefix = "rows/row_" + itos(row) + "/cell_" + itos(column) + "/";
			r_list->push_back(PropertyInfo(Variant::STRING, prefix + "text", PROPERTY_HINT_MULTILINE_TEXT));
			r_list->push_back(PropertyInfo(Variant::OBJECT, prefix + "icon", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"));
			r_list->push_back(PropertyInfo(Variant::INT, prefix + "role", PROPERTY_HINT_ENUM, "Data,Header"));
			r_list->push_back(PropertyInfo(Variant::INT, prefix + "horizontal_alignment", PROPERTY_HINT_ENUM, "Left,Center,Right"));
			r_list->push_back(PropertyInfo(Variant::INT, prefix + "vertical_alignment", PROPERTY_HINT_ENUM, "Top,Center,Bottom"));
			r_list->push_back(PropertyInfo(Variant::INT, prefix + "wrap_mode", PROPERTY_HINT_ENUM, "Inherit,Disabled,Enabled"));
			r_list->push_back(PropertyInfo(Variant::VECTOR2I, prefix + "span"));
		}
	}
}

bool Table::_is_anchor(int p_row, int p_column) const {
	return p_row >= 0 && p_row < _owners.size() && p_column >= 0 && p_column < _columns.size() && _owners[p_row][p_column] == Vector2i(p_column, p_row);
}

bool Table::_is_empty(const Cell &p_cell) const {
	return p_cell.text.is_empty() && p_cell.icon.is_null() && p_cell.role == DATA && p_cell.column_span == 1 && p_cell.row_span == 1 && p_cell.horizontal_alignment == HORIZONTAL_ALIGNMENT_LEFT && p_cell.vertical_alignment == VERTICAL_ALIGNMENT_CENTER && p_cell.wrap_mode == INHERIT;
}

bool Table::_validate_grid(const Vector<Vector<Cell>> &p_rows, int p_columns, int p_frozen_rows, Vector<Vector<Vector2i>> &r_owners) const {
	r_owners.clear();
	r_owners.resize(p_rows.size());
	for (int row = 0; row < p_rows.size(); row++) {
		if (p_rows[row].size() != p_columns) {
			return false;
		}
		r_owners.write[row].resize(p_columns);
		for (int column = 0; column < p_columns; column++) {
			r_owners.write[row].write[column] = Vector2i(-1, -1);
		}
	}
	for (int row = 0; row < p_rows.size(); row++) {
		for (int column = 0; column < p_columns; column++) {
			const Cell &cell = p_rows[row][column];
			if (r_owners[row][column].x >= 0) {
				if (!_is_empty(cell)) {
					return false;
				}
				continue;
			}
			if (cell.row_span < 1 || cell.column_span < 1 || cell.row_span > p_rows.size() - row || cell.column_span > p_columns - column || (row < p_frozen_rows && row + cell.row_span > p_frozen_rows)) {
				return false;
			}
			for (int covered_row = row; covered_row < row + cell.row_span; covered_row++) {
				for (int covered_column = column; covered_column < column + cell.column_span; covered_column++) {
					if (r_owners[covered_row][covered_column].x >= 0) {
						return false;
					}
					r_owners.write[covered_row].write[covered_column] = Vector2i(column, row);
				}
			}
		}
	}
	return true;
}

void Table::_rebuild_owners() {
	ERR_FAIL_COND(!_validate_grid(_rows, _columns.size(), _frozen_rows, _owners));
}

void Table::_changed(bool p_structure) {
	if (p_structure) {
		notify_property_list_changed();
	}
	_queue_layout();
	emit_signal("table_changed");
}

bool Table::_has_footer_content() const {
	const HBoxContainer *footer = _footer_hbox;
	if (!footer || !footer->is_visible()) {
		return false;
	}
	for (int i = 0; i < footer->get_child_count(); i++) {
		const Control *control = Object::cast_to<Control>(footer->get_child(i));
		if (control && control->is_visible() && !control->is_set_as_top_level()) {
			return true;
		}
	}
	return false;
}

Size2 Table::_get_footer_minimum_size() const {
	Size2 minimum;
	if (_theme_cache.font.is_valid()) {
		if (_show_row_count) {
			minimum = _theme_cache.font->get_string_size(vformat(atr("Rows: %d"), _rows.size()), HORIZONTAL_ALIGNMENT_LEFT, -1, _theme_cache.font_size);
		}
		minimum.y = MAX(minimum.y, _theme_cache.font->get_height(_theme_cache.font_size));
	}
	if (_has_footer_content()) {
		const Size2 controls_minimum = _footer_hbox->get_combined_minimum_size();
		minimum.x += controls_minimum.x + (_show_row_count ? MAX(0, _theme_cache.gap) : 0);
		minimum.y = MAX(minimum.y, controls_minimum.y);
	}
	if (_theme_cache.footer.is_valid()) {
		minimum += _theme_cache.footer->get_minimum_size();
	}
	return minimum;
}

void Table::_footer_child_entered_tree(Node *p_child) {
	Control *control = Object::cast_to<Control>(p_child);
	if (control && !control->is_connected("visibility_changed", callable_mp(this, &Table::_queue_layout))) {
		control->connect("visibility_changed", callable_mp(this, &Table::_queue_layout));
	}
	_queue_layout();
}

void Table::_footer_child_exiting_tree(Node *p_child) {
	Control *control = Object::cast_to<Control>(p_child);
	if (control && control->is_connected("visibility_changed", callable_mp(this, &Table::_queue_layout))) {
		control->disconnect("visibility_changed", callable_mp(this, &Table::_queue_layout));
	}
	_queue_layout();
}

void Table::_queue_layout() {
	_layout_dirty = true;
	update_minimum_size();
	queue_redraw();
}

void Table::_update_layout() {
	_layout_dirty = false;
	if (_theme_cache.font.is_null() || _theme_cache.background.is_null()) {
		return;
	}
	const int horizontal_padding = MAX(0, _theme_cache.horizontal_cell_padding);
	const int vertical_padding = MAX(0, _theme_cache.vertical_cell_padding);
	const int gap = MAX(0, _theme_cache.gap);
	Rect2 inner(_theme_cache.background->get_offset(), (get_size() - _theme_cache.background->get_minimum_size()).max(Size2()));
	const float footer_height = _get_footer_minimum_size().y;
	_footer_background->set_position(inner.position + Point2(0, MAX(0.0f, inner.size.y - footer_height)));
	_footer_background->set_size(Size2(inner.size.x, footer_height));
	if (_has_footer_content()) {
		HBoxContainer *footer_hbox = _footer_hbox;
		const Vector2 footer_offset = _theme_cache.footer.is_valid() ? _theme_cache.footer->get_offset() : Vector2();
		const Size2 footer_margins = _theme_cache.footer.is_valid() ? _theme_cache.footer->get_minimum_size() : Size2();
		const float count_width = _show_row_count ? _theme_cache.font->get_string_size(vformat(atr("Rows: %d"), _rows.size()), HORIZONTAL_ALIGNMENT_LEFT, -1, _theme_cache.font_size).x : 0;
		const float count_gap = _show_row_count ? gap : 0;
		Size2 controls_size = (_footer_background->get_size() - footer_margins).max(Size2());
		controls_size.x = MAX(0.0f, controls_size.x - count_width - count_gap);
		footer_hbox->set_position(_footer_background->get_position() + footer_offset + Point2(count_width + count_gap, 0));
		footer_hbox->set_size(controls_size);
	}
	inner.size.y = MAX(0.0f, inner.size.y - footer_height - (_rows.is_empty() ? 0 : gap));
	const float scrollbar_width = _v_scroll->get_combined_minimum_size().x;
	const float scrollbar_height = _h_scroll->get_combined_minimum_size().y;
	bool vertical_scroll = false;
	bool horizontal_scroll = false;
	Size2 viewport;
	float content_width = 0;
	float content_height = 0;
	for (int pass = 0; pass < 3; pass++) {
		viewport = (inner.size - Size2(vertical_scroll ? scrollbar_width : 0, horizontal_scroll ? scrollbar_height : 0)).max(Size2());
		_column_widths.resize(_columns.size());
		_column_offsets.resize(_columns.size());
		for (int column = 0; column < _columns.size(); column++) {
			_column_widths.write[column] = MAX(_columns[column].width, horizontal_padding * 2 + 1);
		}
		// First measure individual columns, then constraints spanning several columns.
		for (int span_pass = 0; span_pass < 2; span_pass++) {
			for (int row = 0; row < _rows.size(); row++) {
				for (int column = 0; column < _columns.size(); column++) {
					if (!_is_anchor(row, column)) {
						continue;
					}
					Cell &cell = _rows.write[row].write[column];
					if ((cell.column_span == 1) != (span_pass == 0)) {
						continue;
					}
					float current_width = gap * (cell.column_span - 1);
					int flexible_columns = 0;
					bool content_sized = false;
					for (int index = column; index < column + cell.column_span; index++) {
						current_width += _column_widths[index];
						flexible_columns += _columns[index].width_mode != FIXED ? 1 : 0;
						content_sized |= _columns[index].width_mode == CONTENT;
					}
					if (!flexible_columns || !content_sized) {
						continue;
					}
					_shape_cell(cell, -1);
					float required_width = cell.text_buf->get_size().x + _get_icon_size(cell).x + horizontal_padding * 2;
					if (cell.icon.is_valid() && !cell.text.is_empty()) {
						required_width += MAX(0, _theme_cache.icon_gap);
					}
					const float extra = MAX(0.0f, required_width - current_width) / flexible_columns;
					for (int index = column; index < column + cell.column_span; index++) {
						if (_columns[index].width_mode != FIXED) {
							_column_widths.write[index] += extra;
						}
					}
				}
			}
		}
		content_width = gap * (_columns.size() - 1);
		float total_ratio = 0;
		for (int column = 0; column < _columns.size(); column++) {
			content_width += _column_widths[column];
			if (_columns[column].width_mode == EXPAND) {
				total_ratio += _columns[column].expand_ratio;
			}
		}
		const float remaining = MAX(0.0f, viewport.x - content_width);
		for (int column = 0; column < _columns.size(); column++) {
			if (_columns[column].width_mode == EXPAND && total_ratio > 0) {
				_column_widths.write[column] += remaining * _columns[column].expand_ratio / total_ratio;
			}
		}
		content_width = 0;
		for (int column = 0; column < _columns.size(); column++) {
			_column_offsets.write[column] = content_width;
			content_width += _column_widths[column] + (column < _columns.size() - 1 ? gap : 0);
		}

		_row_heights.resize(_rows.size());
		_row_offsets.resize(_rows.size());
		_row_stripes.resize(_rows.size());
		for (int row = 0; row < _rows.size(); row++) {
			_row_heights.write[row] = _theme_cache.font->get_height(_theme_cache.font_size) + vertical_padding * 2;
		}
		for (int span_pass = 0; span_pass < 2; span_pass++) {
			for (int row = 0; row < _rows.size(); row++) {
				for (int column = 0; column < _columns.size(); column++) {
					if (!_is_anchor(row, column)) {
						continue;
					}
					Cell &cell = _rows.write[row].write[column];
					if ((cell.row_span == 1) != (span_pass == 0)) {
						continue;
					}
					float width = gap * (cell.column_span - 1);
					for (int index = column; index < column + cell.column_span; index++) {
						width += _column_widths[index];
					}
					_shape_cell(cell, width);
					const float required_height = MAX(cell.text_buf->get_size().y, _get_icon_size(cell).y) + vertical_padding * 2;
					float current_height = gap * (cell.row_span - 1);
					for (int index = row; index < row + cell.row_span; index++) {
						current_height += _row_heights[index];
					}
					const float extra = MAX(0.0f, required_height - current_height) / cell.row_span;
					for (int index = row; index < row + cell.row_span; index++) {
						_row_heights.write[index] += extra;
					}
				}
			}
		}
		content_height = 0;
		int body_row = 0;
		for (int row = 0; row < _rows.size(); row++) {
			bool header_only = true;
			for (int column = 0; column < _columns.size(); column++) {
				const Vector2i owner = _owners[row][column];
				header_only &= _rows[owner.y][owner.x].role == HEADER;
			}
			_row_stripes.write[row] = header_only ? -1 : body_row++ % 2;
			_row_offsets.write[row] = content_height;
			content_height += _row_heights[row] + (row < _rows.size() - 1 ? gap : 0);
		}
		_frozen_height = _frozen_rows == 0 ? 0 : _frozen_rows == _rows.size() ? content_height : _row_offsets[_frozen_rows];
		const bool needs_horizontal = content_width > viewport.x + 0.5f;
		const bool needs_vertical = !_fit_content_height && content_height - _frozen_height > MAX(0.0f, viewport.y - _frozen_height) + 0.5f;
		if ((!needs_horizontal || horizontal_scroll) && (!needs_vertical || vertical_scroll)) {
			break;
		}
		horizontal_scroll |= needs_horizontal;
		vertical_scroll |= needs_vertical;
	}
	const float fitted_height = content_height + (horizontal_scroll ? scrollbar_height : 0);
	if (_fit_content_height && !Math::is_equal_approx(_fitted_height, fitted_height)) {
		_fitted_height = fitted_height;
		update_minimum_size();
	}
	Point2 origin = inner.position;
	if (is_layout_rtl() && vertical_scroll) {
		origin.x += scrollbar_width;
	}
	_frozen->set_position(origin);
	_frozen->set_size(Size2(viewport.x, MIN(viewport.y, _frozen_height)));
	_frozen->set_visible(_frozen_rows > 0);
	_body->set_position(origin + Point2(0, _frozen_height));
	_body->set_size(Size2(viewport.x, MAX(0.0f, viewport.y - _frozen_height)));
	_h_scroll->set_position(Point2(origin.x, inner.position.y + viewport.y));
	_h_scroll->set_size(Size2(viewport.x, scrollbar_height));
	_h_scroll->set_max(content_width);
	_h_scroll->set_page(viewport.x);
	_h_scroll->set_visible(horizontal_scroll);
	_v_scroll->set_position(Point2(is_layout_rtl() ? inner.position.x : inner.position.x + viewport.x, origin.y + _frozen_height));
	_v_scroll->set_size(Size2(scrollbar_width, MAX(0.0f, viewport.y - _frozen_height)));
	_v_scroll->set_max(MAX(0.0f, content_height - _frozen_height));
	_v_scroll->set_page(MAX(0.0f, viewport.y - _frozen_height));
	_v_scroll->set_visible(vertical_scroll);
	_frozen->queue_redraw();
	_body->queue_redraw();
	_footer_background->queue_redraw();
	emit_signal("cell_layout_changed");
}

Size2 Table::_get_icon_size(const Cell &p_cell) const {
	if (p_cell.icon.is_null()) {
		return Size2();
	}
	Size2 size = p_cell.icon->get_size();
	if (size.x > _theme_cache.icon_max_width && _theme_cache.icon_max_width > 0) {
		size *= _theme_cache.icon_max_width / size.x;
	}
	return size;
}

bool Table::_is_wrapped(const Cell &p_cell) const {
	return p_cell.wrap_mode == ENABLED || (p_cell.wrap_mode == INHERIT && _wrap_cells);
}

void Table::_shape_cell(Cell &r_cell, float p_width) {
	if (r_cell.text_buf.is_null()) {
		r_cell.text_buf.instantiate();
	}
	r_cell.text_buf->clear();
	r_cell.text_buf->set_direction(is_layout_rtl() ? TextServer::DIRECTION_RTL : TextServer::DIRECTION_LTR);
	r_cell.text_buf->set_alignment(HORIZONTAL_ALIGNMENT_LEFT);
	const bool wrap = _is_wrapped(r_cell);
	if (wrap && p_width >= 0) {
		r_cell.text_buf->set_break_flags(TextServer::BREAK_MANDATORY | TextServer::BREAK_WORD_BOUND | TextServer::BREAK_GRAPHEME_BOUND);
	} else {
		r_cell.text_buf->set_break_flags(TextServer::BREAK_MANDATORY);
	}
	r_cell.text_buf->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	r_cell.text_buf->set_max_lines_visible(wrap ? -1 : 1);
	float text_width = p_width;
	if (p_width >= 0) {
		text_width -= MAX(0, _theme_cache.horizontal_cell_padding) * 2 + _get_icon_size(r_cell).x;
		if (r_cell.icon.is_valid() && !r_cell.text.is_empty()) {
			text_width -= MAX(0, _theme_cache.icon_gap);
		}
		text_width = MAX(1.0f, text_width);
	}
	r_cell.text_buf->set_width(text_width);
	r_cell.text_buf->add_string(atr(r_cell.text), _theme_cache.font, _theme_cache.font_size);
}

Rect2 Table::_get_cell_content_rect(int p_row, int p_column) const {
	const Cell &cell = _rows[p_row][p_column];
	const int last_column = p_column + cell.column_span - 1;
	const int last_row = p_row + cell.row_span - 1;
	return Rect2(_column_offsets[p_column], _row_offsets[p_row], _column_offsets[last_column] + _column_widths[last_column] - _column_offsets[p_column], _row_offsets[last_row] + _row_heights[last_row] - _row_offsets[p_row]);
}

void Table::_draw_cell(Control *p_canvas, const Cell &p_cell, const Rect2 &p_rect, int p_stripe) {
	if (p_cell.text_buf.is_null()) {
		return;
	}
	if (p_cell.role == HEADER) {
		p_canvas->draw_style_box(_theme_cache.header, p_rect);
	} else {
		p_canvas->draw_style_box(p_stripe == 1 ? _theme_cache.row_odd : _theme_cache.row_even, p_rect);
		p_canvas->draw_style_box(_theme_cache.cell, p_rect);
	}
	const float horizontal_padding = MAX(0, _theme_cache.horizontal_cell_padding);
	const float vertical_padding = MAX(0, _theme_cache.vertical_cell_padding);
	const float available_width = MAX(0.0f, p_rect.size.x - horizontal_padding * 2);
	Size2 icon_size = _get_icon_size(p_cell);
	if (icon_size.x > available_width && icon_size.x > 0) {
		icon_size *= available_width / icon_size.x;
	}
	const float icon_gap = p_cell.icon.is_valid() && !p_cell.text.is_empty() ? MAX(0, _theme_cache.icon_gap) : 0;
	const Size2 text_size = p_cell.text_buf->get_size();
	const float width = MIN(available_width, icon_size.x + icon_gap + text_size.x);
	const float height = MAX(icon_size.y, text_size.y);
	Point2 position = p_rect.position + Point2(horizontal_padding, vertical_padding);
	if (p_cell.horizontal_alignment == HORIZONTAL_ALIGNMENT_CENTER) {
		position.x += (available_width - width) / 2;
	} else if (p_cell.horizontal_alignment == HORIZONTAL_ALIGNMENT_RIGHT) {
		position.x += available_width - width;
	}
	if (p_cell.vertical_alignment == VERTICAL_ALIGNMENT_CENTER) {
		position.y = p_rect.position.y + (p_rect.size.y - height) / 2;
	} else if (p_cell.vertical_alignment == VERTICAL_ALIGNMENT_BOTTOM) {
		position.y = p_rect.get_end().y - vertical_padding - height;
	}
	if (p_cell.icon.is_valid()) {
		const Point2 icon_position(position.x + (is_layout_rtl() ? width - icon_size.x : 0), position.y + (height - icon_size.y) / 2);
		p_canvas->draw_texture_rect(p_cell.icon, Rect2(icon_position, icon_size), false);
	}
	if (available_width <= icon_size.x + icon_gap || p_cell.text.is_empty()) {
		return;
	}
	const Point2 text_position(position.x + (is_layout_rtl() ? 0 : icon_size.x + icon_gap), position.y + (height - text_size.y) / 2);
	p_cell.text_buf->draw(p_canvas->get_canvas_item(), text_position, p_cell.role == HEADER ? _theme_cache.header_font_color : _theme_cache.font_color);
}

void Table::_draw_frozen() {
	_draw_region(_frozen, true);
}

void Table::_draw_body() {
	_draw_region(_body, false);
}

void Table::_draw_region(Control *p_canvas, bool p_frozen) {
	if (_layout_dirty || _row_heights.size() != _rows.size()) {
		return;
	}
	const int begin = p_frozen ? 0 : _frozen_rows;
	const int end = p_frozen ? _frozen_rows : _rows.size();
	const float scroll = p_frozen ? 0 : _frozen_height + _v_scroll->get_value();
	for (int row = begin; row < end; row++) {
		for (int column = 0; column < _columns.size(); column++) {
			if (!_is_anchor(row, column)) {
				continue;
			}
			Rect2 rect = _get_cell_content_rect(row, column);
			rect.position.y -= scroll;
			rect.position.x -= _h_scroll->get_value();
			if (is_layout_rtl()) {
				rect.position.x = p_canvas->get_size().x - rect.position.x - rect.size.x;
			}
			if (rect.intersects(Rect2(Point2(), p_canvas->get_size()))) {
				_draw_cell(p_canvas, _rows[row][column], rect, _row_stripes[row]);
			}
		}
	}
}

void Table::_draw_footer() {
	Rect2 content(Point2(), _footer_background->get_size());
	if (_theme_cache.footer.is_valid()) {
		_footer_background->draw_style_box(_theme_cache.footer, content);
		content.position += _theme_cache.footer->get_offset();
		content.size = (content.size - _theme_cache.footer->get_minimum_size()).max(Size2());
	}
	if (_show_row_count && _theme_cache.font.is_valid()) {
		const float baseline = (content.size.y - _theme_cache.font->get_height(_theme_cache.font_size)) / 2 + _theme_cache.font->get_ascent(_theme_cache.font_size);
		_theme_cache.font->draw_string(_footer_background->get_canvas_item(), content.position + Point2(0, baseline), vformat(atr("Rows: %d"), _rows.size()), HORIZONTAL_ALIGNMENT_LEFT, content.size.x, _theme_cache.font_size, _theme_cache.font_color);
	}
}

void Table::_scroll_changed(double p_value) {
	_frozen->queue_redraw();
	_body->queue_redraw();
	emit_signal("cell_layout_changed");
}
