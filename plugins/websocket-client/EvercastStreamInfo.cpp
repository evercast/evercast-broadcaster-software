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

#include "EvercastStreamInfo.h"

#include <sstream>
#include "util/stream-config.h"

#define COLOR_SPACE_BT_709 "709"
#define COLOR_SPACE_BT_601 "601"
#define COLOR_RANGE_FULL "Full"
#define COLOR_RANGE_PARTIAL "Partial"

EvercastStreamInfo * EvercastStreamInfo::_instance = new EvercastStreamInfo;

EvercastStreamInfo* EvercastStreamInfo::instance() {
	return EvercastStreamInfo::_instance;
}

std::string EvercastStreamInfo::streamType() {
	return _streamType;
}

std::string EvercastStreamInfo::userId() {
	return _userId;
}

std::string EvercastStreamInfo::roomId() {
	return _roomId;
}

std::string EvercastStreamInfo::resolution() {
	return _resolution;
}

std::string EvercastStreamInfo::framerate() {
	return _framerate;
}

std::string EvercastStreamInfo::colorSpace() {
	return _colorSpace;
}

std::string EvercastStreamInfo::colorSpacePrimaries()
{
	return webRTCColorSpace();
}

std::string EvercastStreamInfo::colorSpaceMatrix() {
	return webRTCColorSpace();
}

std::string EvercastStreamInfo::colorRange()
{
	if (COLOR_RANGE_FULL == _colorRange) {
		return "FULL";
	} else if (COLOR_RANGE_PARTIAL == _colorRange) {
		return "LIMITED";
	} else {
		return "UNSPECIFIED";
	}
}

std::string EvercastStreamInfo::colorSpaceTransfer() {
	return webRTCColorSpace();
}

std::string EvercastStreamInfo::streamId() {
	return _streamId;
}

bool EvercastStreamInfo::refreshStreamConfig() {
	stream_config_t *streamInfo = stream_config_get();
	this->_resolution = EvercastStreamInfo::resString(streamInfo->output_resolution_x, streamInfo->output_resolution_y);
	this->_framerate = streamInfo->framerate;
	this->_colorSpace = streamInfo->color_space;
	this->_colorRange = streamInfo->color_range;

	return true;
}

bool EvercastStreamInfo::assignStreamSettings(obs_output_t *output) {

    obs_service_t *service = obs_output_get_service(output);
    if (!service) {
		this->_userId = "";
		this->_roomId = "";
		blog(LOG_INFO,
		     "No service available while attempting to set service-based information.");

	    return false;
    }

    this->_userId = obs_service_get_username(service) ? obs_service_get_username(service) : "";
    this->_roomId = obs_service_get_room(service) ? obs_service_get_room(service) : "";

    return true;
}

void EvercastStreamInfo::assignStreamId(std::string streamId) {
	std::string colorSpaceTag = webRTCColorSpace();
	this->_streamId = "EBS|" + colorSpaceTag + "|" + streamId;
}

void EvercastStreamInfo::refreshStreamType() {
	stream_config_t *streamInfo = stream_config_get();
	this->_streamType = streamInfo->stream_type;
}

std::string EvercastStreamInfo::resString(uint64_t cx, uint64_t cy)
{
	std::stringstream res;
	res << cx << "x" << cy;
	return res.str();
}

std::string EvercastStreamInfo::webRTCColorSpace() {
	if (COLOR_SPACE_BT_601 == _colorSpace) {
		return "SMPTE170M"; // Identical to BT601
	} else if (COLOR_SPACE_BT_709 == _colorSpace) {
		return "BT709";
	} else {
		return "UNSPECIFIED";
	}
}

