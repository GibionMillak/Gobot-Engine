/**************************************************************************/
/*  highlighted_label.cpp                                                 */
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

#include "highlighted_label.h"

#include "servers/text/text_server.h"

void HighlightedLabel::add_highlight(const Vector2i &p_interval) {
	if (p_interval.y > 0) {
		_highlights.append(p_interval);
		queue_redraw();
	}
}

void HighlightedLabel::reset_highlights() {
	_highlights.clear();
	queue_redraw();
}

void HighlightedLabel::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_DRAW: {
			if (_highlights.is_empty()) {
				return;
			}

			Vector2 offset;
			int line_limit;
			int line_spacing;
			_get_layout_data(offset, line_limit, line_spacing);

			for (const Vector2i &substr : _highlights) {
				_draw_substr_rects(substr, offset, line_limit, line_spacing);
			}
		} break;
	}
}

void HighlightedLabel::_draw_substr_rects(const Vector2i &p_substr, Vector2 p_offset, int p_line_limit, int p_line_spacing) {
	for (int i = get_lines_skipped(); i < p_line_limit; i++) {
		RID line = _get_line_rid(i);
		Vector<Vector2> ranges = TS->shaped_text_get_selection(line, p_substr.x, p_substr.x + p_substr.y);
		Rect2 line_rect = _get_line_rect(i);
		for (const Vector2 &range : ranges) {
			Rect2 rect = Rect2(Point2(range.x, 0) + line_rect.position, Size2(range.y - range.x, line_rect.size.y));
			rect.position = p_offset + line_rect.position;
			rect.position.x += range.x;
			rect.size = Size2(range.y - range.x, line_rect.size.y);
			rect.size.x = MIN(rect.size.x, line_rect.size.x - range.x);
			if (rect.size.x > 0) {
				draw_rect(rect, Color(1, 1, 1, 0.07), true);
				draw_rect(rect, Color(0.5, 0.7, 1.0, 0.4), false, 1);
			}
		}
		p_offset.y += p_line_spacing + TS->shaped_text_get_ascent(line) + TS->shaped_text_get_descent(line);
	}
}
