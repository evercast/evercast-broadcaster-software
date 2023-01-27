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

#include <stdint.h>
#include <string>

#include "obs.h"
#include "util/util.hpp"
#include <WebsocketClient.h>

class EvercastStreamInfo {
public:
	std::string streamType();
	std::string userId();
	std::string roomId();
	std::string resolution();
	std::string framerate();
	std::string colorSpace();
	std::string colorSpacePrimaries();
	std::string colorSpaceMatrix();
	std::string colorSpaceTransfer();
	std::string colorRange();
	std::string streamId();

	static WEBSOCKETCLIENT_API EvercastStreamInfo *instance();

	bool WEBSOCKETCLIENT_API assignStreamSettings(obs_output_t *output);

	void WEBSOCKETCLIENT_API assignStreamId(std::string streamId);

	bool WEBSOCKETCLIENT_API refreshStreamConfig();

	void WEBSOCKETCLIENT_API refreshStreamType();

private:
	static EvercastStreamInfo *_instance;
	std::string _streamType = "";
	std::string _userId = "";
	std::string _roomId = "";
	std::string _resolution = "";
	std::string _framerate = "";
	std::string _colorSpace = "";
	std::string _colorRange = "";
	std::string _streamId = "";

	EvercastStreamInfo(){};
	static std::string resString(uint64_t cx, uint64_t cy);
	std::string webRTCColorSpace();
};

