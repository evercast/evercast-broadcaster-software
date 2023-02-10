/*
 * Copyright (c) 2022 Evercast, LLC
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "osx-version.h"

#include "obs-internal.h"

const char * getOSXVersion() {
	Class NSProcessInfo = objc_getClass("NSProcessInfo");
	id pi = ((id (*)(id, SEL))objc_msgSend)((id)NSProcessInfo,
			     sel_registerName("processInfo"));

	SEL UTF8String = sel_registerName("UTF8String");
	id vs = ((id (*)(id, SEL))objc_msgSend)(pi,
			     sel_registerName("operatingSystemVersionString"));
	return (const char *)((id (*)(id, SEL))objc_msgSend)(vs, UTF8String);
}