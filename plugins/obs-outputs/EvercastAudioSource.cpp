#include "EvercastAudioSource.h"
#include <obs.h>

rtc::scoped_refptr<EvercastAudioSource> EvercastAudioSource::Create(cricket::AudioOptions *options)
{
	audio_t *audio = obs_get_audio();

	if (nullptr == audio) {
		blog(LOG_ERROR, "Could not retrieve OBS audio source.");
		return nullptr;
	}

	rtc::scoped_refptr<EvercastAudioSource> source(
		new rtc::RefCountedObject<EvercastAudioSource>());
	source->Initialize(audio, options);
	return source;
}

void EvercastAudioSource::AddSink(AudioTrackSinkInterface *sink) {
	if (nullptr != sink_) {
		blog(LOG_WARNING, "Replacing audio sink...");
	}

	sink_ = sink;
}

void EvercastAudioSource::RemoveSink(AudioTrackSinkInterface *sink) {
	if (sink_ != sink) {
		blog(LOG_WARNING, "Attempting to remove unassigned sink...");
		return;
	}

	sink_ = nullptr;
}

void EvercastAudioSource::OnAudioData(audio_data *frame)
{
	AudioTrackSinkInterface *sink = this->sink_;
	if (nullptr == sink) {
		return;
	}

	uint8_t *data = frame->data[0];
	size_t num_channels = audio_output_get_channels(audio_);
	uint32_t sample_rate = 48000;
	size_t chunk = (sample_rate / 100);
	size_t sample_size = 2;
	size_t i = 0;
	uint8_t *position;

	if (pending_remainder) {
		// Copy missing chunks
		i = chunk - pending_remainder;
		memcpy(pending + pending_remainder * sample_size * num_channels, data,
		       i * sample_size * num_channels);

		// Send
		sink->OnData(pending, 16, sample_rate, num_channels, chunk);

		// No pending chunks
		pending_remainder = 0;
	}

	while (i + chunk < frame->frames) {
		position = data + i * sample_size * num_channels;
		sink->OnData(position, 16, sample_rate, num_channels, chunk);
		i += chunk;
	}

	if (i != frame->frames) {
		pending_remainder = frame->frames - i;
		memcpy(pending, data + i * sample_size * num_channels,
		       pending_remainder * sample_size * num_channels);
	}
}

EvercastAudioSource::EvercastAudioSource()
{
	sink_ = nullptr;
}

EvercastAudioSource::~EvercastAudioSource()
{
	free(pending);
}

void EvercastAudioSource::Initialize(audio_t *audio,
				     cricket::AudioOptions *options)
{
	// TODO: Null-check audio
	audio_ = audio;
	options_ = *options;

	size_t num_channels = audio_output_get_channels(audio_);
	size_t pending_len = num_channels * 2 * 640;
	pending = (uint8_t *)malloc(pending_len);
	pending_remainder = 0;
}
