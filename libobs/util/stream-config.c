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

#include "util/stream-config.h"
#include "bmem.h"

static stream_config_t stream_info;
static stream_config_change_callback change_handler = NULL;
static void *change_handler_param = NULL;

bool stream_config_init() {
	stream_info.color_space = NULL;
	stream_info.stream_type = NULL;
	return true;
}

void stream_config_assign(config_t* config) {
	stream_info.output_resolution_x = config_get_uint(config, "Video", "OutputCX");
    stream_info.output_resolution_y = config_get_uint(config, "Video", "OutputCY");

    if (NULL != stream_info.framerate) {
	    bfree(stream_info.framerate);
    }

    const char *framerate = config_get_string(config, "Video", "FPSCommon");
    stream_info.framerate = (char *)bzalloc(strlen(framerate));
    strcpy(stream_info.framerate, framerate);

	if (NULL != stream_info.color_space) {
		bfree(stream_info.color_space);
	}

	const char *space = config_get_string(config, "Video", "ColorSpace");
	stream_info.color_space = (char*)bzalloc(strlen(space));
	strcpy(stream_info.color_space, space);

    if (NULL != stream_info.color_range) {
	    bfree(stream_info.color_range);
    }

	const char *range = config_get_string(config, "Video", "ColorRange");
	stream_info.color_range = (char *)bzalloc(strlen(range));
	strcpy(stream_info.color_range, range);
}

void stream_config_type(const char* type, size_t size) {
	if (NULL != stream_info.stream_type) {
		bfree(stream_info.stream_type);
	}

	stream_info.stream_type = (char*)bzalloc(size + 1);

	strncpy(stream_info.stream_type, type, size);
	blog(LOG_DEBUG, "Stream type %s assigned", stream_info.stream_type);
	if (NULL != change_handler) {
		change_handler(change_handler_param);
	}
}

stream_config_t * stream_config_get() {
	return &stream_info;
}

bool stream_config_register_change_callback(stream_config_change_callback callback, void *param) {
	if (NULL != change_handler) {
		return false;
	}

	change_handler = callback;
	change_handler_param = param;
	return true;
}

bool stream_config_unregister_change_callback(stream_config_change_callback callback, void *param) {
	if (NULL == change_handler) {
		return false;
	}

	change_handler = NULL;
	change_handler_param = NULL;
	return true;
}
