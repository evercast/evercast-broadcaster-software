 #include "ExtendedEncoderFactory.h"

std::unique_ptr<VideoEncoder>
ExtendedEncoderFactory::CreateVideoEncoder(const SdpVideoFormat &format)
{
	if (format.name == "vp9" && format.parameters["profile-id"] == "2") {
		return VP9P2Encoder::Create(cricket::VideoCodec(format));
	}

	return BuiltinVideoEncoderFactory::CreateVideoEncoder(format);
}
