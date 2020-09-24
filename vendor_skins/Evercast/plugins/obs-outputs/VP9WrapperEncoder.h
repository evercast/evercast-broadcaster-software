#pragma once

#include "media/base/codec.h"
#include "api/video_codecs/video_encoder.h"

using namespace webrtc;

class VP9WrapperEncoder : public VideoEncoder {
public:
	static std::unique_ptr<VP9WrapperEncoder> Create(std::unique_ptr<VideoEncoder> wrappedEncoder);
	VP9WrapperEncoder(std::unique_ptr<VideoEncoder> wrappedEncoder);

	void SetFecControllerOverride(FecControllerOverride* fec_controller_override) override;

	int32_t InitEncode(
		const VideoCodec* codec_settings,
		int32_t number_of_cores,
		size_t max_payload_size) override;
	int InitEncode(const VideoCodec* codec_settings,
		const VideoEncoder::Settings& settings) override;

	int32_t RegisterEncodeCompleteCallback(
		EncodedImageCallback* callback) override;

	int32_t Release() override;

	int32_t Encode(const VideoFrame& frame,
		const std::vector<VideoFrameType>* frame_types) override;

	void SetRates(const RateControlParameters& parameters) override;

	void OnPacketLossRateUpdate(float packet_loss_rate) override;

	void OnRttUpdate(int64_t rtt_ms) override;

	void OnLossNotification(const LossNotification& loss_notification) override;

	VideoEncoder::EncoderInfo GetEncoderInfo() const override;
private:
	std::unique_ptr<VideoEncoder> wrappedEncoder;
};
