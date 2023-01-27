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

#pragma once

#include "util/config-file.h"

#ifdef __cplusplus
extern "C" {
#endif

struct stream_config {
	uint64_t output_resolution_x;
	uint64_t output_resolution_y;
	uint64_t framerate;
	char *color_space;
	char *color_range;
	char *stream_type;
};

typedef void (*stream_config_change_callback)(void *);

typedef struct stream_config stream_config_t;

EXPORT bool stream_config_init();

EXPORT void stream_config_assign(config_t *config);

EXPORT void stream_config_type(const char *type, size_t size);

EXPORT stream_config_t * stream_config_get();

EXPORT bool stream_config_register_change_callback(stream_config_change_callback callback, void *param);

EXPORT bool stream_config_unregister_change_callback(stream_config_change_callback callback, void *param);

#ifdef __cplusplus
}
#endif
