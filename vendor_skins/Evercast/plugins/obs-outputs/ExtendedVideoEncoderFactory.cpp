#include "ExtendedVideoEncoderFactory.h"
#include "VP9WrapperEncoder.h"

#include <memory>
#include <vector>

#include "absl/strings/match.h"
#include "api/video_codecs/sdp_video_format.h"
#include "api/video_codecs/video_encoder.h"
#include "media/base/codec.h"
#include "media/base/media_constants.h"
#include "media/engine/encoder_simulcast_proxy.h"
#include "media/engine/internal_encoder_factory.h"
#include "rtc_base/checks.h"

using namespace webrtc;

class ExtendedVideoEncoderFactory : public VideoEncoderFactory {
public:
	ExtendedVideoEncoderFactory()
		: internal_encoder_factory_(new InternalEncoderFactory()) {}

	VideoEncoderFactory::CodecInfo QueryVideoEncoder(
		const SdpVideoFormat& format) const override {
		// Format must be one of the internal formats.
		/*RTC_DCHECK(IsFormatSupported(
			internal_encoder_factory_->GetSupportedFormats(), format));*/
		VideoEncoderFactory::CodecInfo info;
		info.has_internal_source = false;
		info.is_hardware_accelerated = false;
		return info;
	}

	std::unique_ptr<VideoEncoder> CreateVideoEncoder(const SdpVideoFormat& format) override {

		// Try creating internal encoder.
		std::unique_ptr<VideoEncoder> internal_encoder;
		internal_encoder = std::make_unique<EncoderSimulcastProxy>(
			internal_encoder_factory_.get(), format);

		auto profileIdPair = format.parameters.find("profile-id");
		std::string profileId = profileIdPair == format.parameters.end() ? "0" : profileIdPair->second;
		if (format.name == "VP9" && profileId == "2") {
			internal_encoder = VP9WrapperEncoder::Create(std::move(internal_encoder));
				/*
					return VP9P2EncoderImpl::Create(cricket::VideoCodec(format));
					*/
		}


		return internal_encoder;
	}

	std::vector<SdpVideoFormat> GetSupportedFormats() const override {
		return internal_encoder_factory_->GetSupportedFormats();
	}

private:
	const std::unique_ptr<VideoEncoderFactory> internal_encoder_factory_;
};

std::unique_ptr<VideoEncoderFactory> CreateExtendedVideoEncoderFactory() {
	return std::make_unique<ExtendedVideoEncoderFactory>();
}

