/**************************************************************************/
/*  project_plugin_manager.h                                              */
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

#include "core/templates/hash_map.h"
#include "core/templates/local_vector.h"
#include "core/templates/vector.h"
#include "scene/main/node.h"

class EditorPlugin;

class ProjectPluginManager : public Node {
	GDCLASS(ProjectPluginManager, Node);

public:
	struct PluginEntry {
		String config_path;
		String name;
		String author;
		String version;
		String description;
		String script;
		bool enabled = false;
	};

	ProjectPluginManager();

	// Called after Autoloads initialize; scripts that need the first scan are retried later.
	void init_plugins();
	Vector<PluginEntry> get_plugins() const;
	// Activation failures emit plugin_load_failed; p_config_changed invokes enable/disable callbacks.
	void set_plugin_enabled(const String &p_plugin, bool p_enabled, bool p_config_changed = false);
	bool is_plugin_enabled(const String &p_plugin) const;
	// Unloads instances before editor teardown without changing the saved enabled list.
	void unload_plugins();

protected:
	static void _bind_methods();

private:
	static Vector<String> _find_plugin_configs(const String &p_dir);
	void _enable_pending_plugins();
	void _remove_from_enabled(const String &p_name);
	void _update_config();
	void _save_config();

	bool _initializing = false;
	// Instances are attached to EditorNode and explicitly detached during unload.
	HashMap<String, EditorPlugin *> _plugins;
	LocalVector<String> _pending_plugins;
};
