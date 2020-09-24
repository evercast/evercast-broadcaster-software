#include "VP9WrapperEncoder.h"
#include "DualPurposeI420Buffer.h"

std::unique_ptr<VP9WrapperEncoder> VP9WrapperEncoder::Create(std::unique_ptr<VideoEncoder> wrappedEncoder)
{
	return std::make_unique<VP9WrapperEncoder>(std::move(wrappedEncoder));
}

VP9WrapperEncoder::VP9WrapperEncoder(std::unique_ptr<VideoEncoder> wrappedEncoder)
{
	this->wrappedEncoder = std::move(wrappedEncoder);
}

void VP9WrapperEncoder::SetFecControllerOverride(FecControllerOverride* fec_controller_override)
{
	this->wrappedEncoder->SetFecControllerOverride(fec_controller_override);
}

int32_t VP9WrapperEncoder::InitEncode(
	const VideoCodec* codec_settings,
	int32_t number_of_cores,
	size_t max_payload_size)
{
	return this->wrappedEncoder->InitEncode(codec_settings, number_of_cores, max_payload_size);
}

int VP9WrapperEncoder::InitEncode(const VideoCodec* codec_settings,
	const VideoEncoder::Settings& settings)
{
	return this->wrappedEncoder->InitEncode(codec_settings, settings);
}

int32_t VP9WrapperEncoder::RegisterEncodeCompleteCallback(
	EncodedImageCallback* callback)
{
	return this->wrappedEncoder->RegisterEncodeCompleteCallback(callback);
}

int32_t VP9WrapperEncoder::Release()
{
	return this->wrappedEncoder->Release();
}

int32_t VP9WrapperEncoder::Encode(const VideoFrame& frame,
	const std::vector<VideoFrameType>* frame_types)
{
	rtc::scoped_refptr<VideoFrameBuffer> buffer = frame.video_frame_buffer();
	DualPurposeI420Buffer* ptr = (DualPurposeI420Buffer*)buffer.get();
	rtc::scoped_refptr<VideoFrameBuffer> realBuffer = ptr->getRealFrame();
	VideoFrame realFrame(
		realBuffer, VideoRotation::kVideoRotation_0, frame.timestamp_us());
	int32_t result = this->wrappedEncoder->Encode(realFrame, frame_types);
	realBuffer = nullptr;
	buffer = nullptr;

	return result;
}

void VP9WrapperEncoder::SetRates(const RateControlParameters& parameters)
{
	this->wrappedEncoder->SetRates(parameters);
}

void VP9WrapperEncoder::OnPacketLossRateUpdate(float packet_loss_rate)
{
	this->wrappedEncoder->OnPacketLossRateUpdate(packet_loss_rate);
}

void VP9WrapperEncoder::OnRttUpdate(int64_t rtt_ms)
{
	this->wrappedEncoder->OnRttUpdate(rtt_ms);
}

void VP9WrapperEncoder::OnLossNotification(const LossNotification& loss_notification)
{
	this->wrappedEncoder->OnLossNotification(loss_notification);
}

VideoEncoder::EncoderInfo VP9WrapperEncoder::GetEncoderInfo() const
{
	return this->wrappedEncoder->GetEncoderInfo();
}

