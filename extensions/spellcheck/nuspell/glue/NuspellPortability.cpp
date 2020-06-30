/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/NuspellPortability.h"

#include <string>
#include <string_view>

#include "mozilla/Encoding.h"
#include "nsUTF8Utils.h"

#include "utils.hxx"

static const void*
NuspellEncodingForLabelNoReplacement(std::string_view aLabel) {
	return mozilla::Encoding::ForLabelNoReplacement(aLabel);
}

static bool NuspellUTF8ToUTF32(std::string_view aSrc, std::wstring& aDst) {
	aDst.clear();
	bool error = false;
	const char* current = aSrc.data();
	const char* end = current + aSrc.size();
	while (current < end) {
		uint32_t scalar = UTF8CharEnumerator::NextChar(&current, end, &error);
		aDst.push_back(static_cast<wchar_t>(scalar));
	}
	return !error;
}

static bool
NuspellEncodingToUTF32(const void* aEncoding, std::string_view aSrc, std::wstring& aDst) {
	MOZ_ASSERT(aEncoding);
	auto encoding = reinterpret_cast<const mozilla::Encoding*>(aEncoding);
	if (aEncoding == UTF_8_ENCODING) {
		return NuspellUTF8ToUTF32(aSrc, aDst);
	}
	nsDependentCString src(aSrc.data(), aSrc.size());
	nsAutoCString dst;
	nsresult rv = encoding->DecodeWithoutBOMHandling(src, dst);
	MOZ_RELEASE_ASSERT(rv != NS_ERROR_OUT_OF_MEMORY);
	mozilla::DebugOnly<bool> valid = NuspellUTF8ToUTF32(std::string_view(dst.BeginReading(), dst.Length()), aDst);
	MOZ_ASSERT(valid);
	return rv == NS_OK;
}

void mozilla::InitializeNuspellPortability() {
	nuspell::initialize_portability(&NuspellEncodingForLabelNoReplacement,
		                            &NuspellUTF8ToUTF32,
		                            &NuspellEncodingToUTF32);
}
