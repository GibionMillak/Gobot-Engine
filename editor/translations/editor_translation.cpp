/**************************************************************************/
/*  editor_translation.cpp                                                */
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

#include "editor_translation.h"

#include "core/io/compression.h"
#include "core/io/file_access_memory.h"
#include "core/io/translation_loader_po.h"
#include "core/string/translation_server.h"
#include "editor/translations/doc_translations.gen.h"
#include "editor/translations/editor_translations.gen.h"
#include "editor/translations/property_translations.gen.h"

Vector<String> get_editor_locales() {
	Vector<String> locales;

	for (const EditorTranslationList *etl = _editor_translations; etl->lang; etl++) {
		const String &locale = etl->lang;
		locales.push_back(locale);
	}

	return locales;
}

String select_editor_locale(const Vector<String> &p_preferred_locales, const LocalVector<String> &p_skipped_locales) {
	const Vector<String> locales = get_editor_locales();
	for (const String &preferred : p_preferred_locales) {
		String best = "en_US";
		int best_score = 0;
		for (const String &locale : locales) {
			if (p_skipped_locales.has(locale.get_slicec('_', 0))) {
				continue;
			}
			const int score = TranslationServer::get_singleton()->compare_locales(preferred, locale);
			if (score > best_score) {
				best = locale;
				best_score = score;
			}
		}
		if (best_score > 0) {
			return best;
		}
	}
	return "en_US";
}

static void _load(const Ref<TranslationDomain> p_domain, const String &p_locale, const EditorTranslationList *p_etl, const uint8_t *p_data, unsigned int p_decomp_size, unsigned int p_comp_size) {
	LocalVector<uint8_t> data;
	data.resize_uninitialized(p_decomp_size);
	const int64_t ret = Compression::decompress(data.ptr(), p_decomp_size, p_data, p_comp_size, Compression::MODE_ZSTD);
	ERR_FAIL_COND_MSG(ret == -1, "Compressed file is corrupt.");

	String full_locale = p_locale;
	String base_locale = p_locale.get_slicec('@', 0).split("_")[0];
	for (const EditorTranslationList *etl = p_etl; etl->lang; etl++) {
		if (etl->lang == full_locale || etl->lang == base_locale) {
			ERR_CONTINUE(etl->offset + etl->size > data.size());

			Ref<FileAccessMemory> fa;
			fa.instantiate();
			fa->open_custom(data.ptr() + etl->offset, etl->size);

			Ref<Translation> tr = TranslationLoaderPO::load_translation(fa);
			if (tr.is_valid()) {
				tr->set_locale(etl->lang);
				p_domain->add_translation(tr);
			}
		}
	}
}

void load_editor_translations(const String &p_locale) {
	Ref<TranslationDomain> domain;

	domain = TranslationServer::get_singleton()->get_editor_domain();
	domain->clear();
	_load(domain, p_locale, _editor_translations, (const uint8_t *)_editor_translation_compressed, _editor_decomp_size, _editor_comp_size);

	domain = TranslationServer::get_singleton()->get_property_domain();
	domain->clear();
	_load(domain, p_locale, _property_translations, (const uint8_t *)_property_translation_compressed, _property_decomp_size, _property_comp_size);
}

void load_doc_translations(const String &p_locale) {
	const Ref<TranslationDomain> domain = TranslationServer::get_singleton()->get_doc_domain();
	domain->clear();
	_load(domain, p_locale, _doc_translations, (const uint8_t *)_doc_translation_compressed, _doc_decomp_size, _doc_comp_size);
}
