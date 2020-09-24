#pragma once

#include "api/video_codecs/video_encoder.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "api/video_codecs/sdp_video_format.h"

std::unique_ptr<webrtc::VideoEncoderFactory> CreateExtendedVideoEncoderFactory();

