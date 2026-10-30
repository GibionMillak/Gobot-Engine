/**************************************************************************/
/*  editor_version_button.cpp                                             */
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

#include "editor_version_button.h"

#include "core/error/error_macros.h"
#include "core/os/time.h"
#include "core/version.h"
#include "editor/gui/containers/editor_toaster_container.h"
#include "servers/display/display_server.h"

EditorVersionButton::EditorVersionButton(VersionFormat p_format) :
		_format(p_format) {
	set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	set_text(_get_version_string(_format));
}

void EditorVersionButton::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY:
		case NOTIFICATION_TRANSLATION_CHANGED: {
			String build_date;
			if (GODOT_VERSION_TIMESTAMP > 0) {
				build_date = Time::get_singleton()->get_datetime_string_from_unix_time(GODOT_VERSION_TIMESTAMP, true) + " UTC";
			} else {
				build_date = TTR("(unknown)");
			}
			set_tooltip_text(vformat(TTR("Git commit date: %s\nClick to copy the version information."), build_date));
		} break;
	}
}

void EditorVersionButton::pressed() {
	DisplayServer::get_singleton()->clipboard_set(_get_version_string(WITH_BUILD));
	EditorToasterContainer *const toaster = EditorToasterContainer::get_singleton();
	if (toaster) {
		toaster->popup_str(TTR("Copied Godot editor version."));
	}
}

String EditorVersionButton::_get_version_string(VersionFormat p_format) {
	String version_string;
	switch (p_format) {
		case BASIC: {
			return GODOT_VERSION_FULL_CONFIG;
		} break;
		case WITH_BUILD: {
			version_string = "v" GODOT_VERSION_FULL_BUILD;
		} break;
		case WITH_NAME_AND_BUILD: {
			version_string = GODOT_VERSION_FULL_NAME;
		} break;
		default: {
			ERR_FAIL_V_MSG(GODOT_VERSION_FULL_NAME, "Unexpected format: " + itos(p_format));
		} break;
	}

	String hash = GODOT_VERSION_HASH;
	if (!hash.is_empty()) {
		hash = vformat(" [%s]", hash.left(9));
	}
	return version_string + hash;
}
