#pragma once

#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "api/video_codecs/video_encoder.h"
#include "api/video_codecs/sdp_video_format.h"

class ExtendedEncoderFactory : public webrtc::BuiltinVideoEncoderFactory {
public:
	std::unique_ptr<VideoEncoder> CreateVideoEncoder(const SdpVideoFormat& format) override;
}
