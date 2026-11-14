/**************************************************************************/
/*  quick_open_result_candidate.cpp                                       */
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

#include "quick_open_result_candidate.h"

#include "editor/file_system/editor_file_system.h"

QuickOpenResultCandidate QuickOpenResultCandidate::from_uid(const ResourceUID::ID &p_uid, bool &r_success) {
	if (p_uid == ResourceUID::INVALID_ID || !ResourceUID::get_singleton()->has_id(p_uid)) {
		r_success = false;
		return QuickOpenResultCandidate();
	}

	QuickOpenResultCandidate candidate;
	candidate.uid = p_uid;
	candidate.result = nullptr;
	r_success = true;
	return candidate;
}

QuickOpenResultCandidate QuickOpenResultCandidate::from_result(const Ref<FuzzySearchMatch> &p_result, bool &r_success) {
	ResourceUID::ID uid = EditorFileSystem::get_singleton()->get_file_uid(p_result->get_target());

	QuickOpenResultCandidate candidate = from_uid(uid, r_success);
	if (!r_success) {
		return QuickOpenResultCandidate();
	}

	candidate.result = p_result;
	return candidate;
}
