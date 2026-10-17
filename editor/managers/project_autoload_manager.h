/**************************************************************************/
/*  project_autoload_manager.h                                            */
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

#include "core/templates/list.h"
#include "core/templates/vector.h"
#include "scene/main/node.h"

class ProjectAutoloadManager : public Node {
	GDCLASS(ProjectAutoloadManager, Node);

public:
	struct AutoloadEntry {
		String name;
		String path;
		bool is_singleton = false;
	};

	ProjectAutoloadManager();
	~ProjectAutoloadManager();

	// Called after global script classes are discovered and before editor plugins initialize.
	void init_autoloads();
	// Reconcile runtime instances with project settings, then emit autoloads_changed.
	void refresh_autoloads();
	Vector<AutoloadEntry> get_autoloads() const;
	bool is_name_valid(const String &p_name, String *r_error = nullptr) const;
	// Mutations save project settings; p_use_undo controls whether an editor undo action is recorded.
	bool add_autoload(const String &p_name, const String &p_path, bool p_use_undo, String *r_error = nullptr);
	void remove_autoload(const String &p_name, bool p_use_undo);
	bool rename_autoload(const String &p_name, const String &p_new_name, String *r_error = nullptr);
	void set_autoload_singleton(const String &p_name, bool p_enabled);
	void swap_autoloads(const String &p_name, const String &p_other_name);
	// An empty destination appends the selected entries, preserving their supplied order.
	void reorder_autoloads(const PackedStringArray &p_names, const String &p_before_name);

protected:
	static void _bind_methods();

private:
	struct AutoloadInfo {
		String name;
		String path;
		bool is_singleton = false;
		bool in_editor = false;
		int order = 0;
		Node *node = nullptr;
	};

	Node *_create_autoload(const String &p_path);
	void _commit_changes();

	List<AutoloadInfo> _autoload_cache;
	bool _updating_autoload = false;
};
