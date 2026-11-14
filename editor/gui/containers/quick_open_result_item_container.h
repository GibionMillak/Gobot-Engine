/**************************************************************************/
/*  quick_open_result_item_container.h                                    */
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

#include "core/math/color.h"
#include "core/math/vector2i.h"
#include "editor/gui/dialogs/editor_quick_open_dialog.h"
#include "scene/gui/margin_container.h"

class BoxContainer;
class HighlightedLabel;
class StyleBox;
class TextureRect;
struct QuickOpenResultCandidate;

class QuickOpenResultItemContainer : public MarginContainer {
	GDCLASS(QuickOpenResultItemContainer, MarginContainer);

public:
	explicit QuickOpenResultItemContainer(EditorQuickOpenDialog::DisplayMode p_display_mode);

	void set_grid_item_scale(float p_scale);
	void reset();
	void set_content(const QuickOpenResultCandidate &p_candidate, bool p_recently_opened);
	void highlight_item(bool p_enabled);

	bool enable_highlights = true;

protected:
	void _notification(int p_what);

private:
	enum class ContentSource {
		PROJECT,
		ADDON,
		EDITOR,
	};

	static Vector2i _get_path_interval(const Vector2i &p_interval, int p_dir_index);
	static Vector2i _get_name_interval(const Vector2i &p_interval, int p_dir_index);

	void _set_enabled(bool p_enabled);
	void _update_grid_size();
	void _update_thumbnail_stretch();
	void _update_badges();

	BoxContainer *_badges_box = nullptr;
	TextureRect *_thumbnail = nullptr;
	HighlightedLabel *_name_lbl = nullptr;
	HighlightedLabel *_path_lbl = nullptr;
	TextureRect *_source_badge = nullptr;
	TextureRect *_recent_badge = nullptr;
	const EditorQuickOpenDialog::DisplayMode _display_mode;
	float _grid_item_scale = 1.0f;
	ContentSource _content_source = ContentSource::PROJECT;
	Ref<StyleBox> _selected_stylebox;
	Ref<StyleBox> _hovering_stylebox;
	Color _highlighted_font_color;
	bool _is_hovering = false;
	bool _is_selected = false;
	bool _recently_opened = false;
};
