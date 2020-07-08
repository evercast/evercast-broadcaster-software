#ifndef _OBS_OUTPUTS_EVERCAST_AUDIO_SOURCE_H_
#define _OBS_OUTPUTS_EVERCAST_AUDIO_SOURCE_H_

#include "api/scoped_refptr.h"
#include "api/notifier.h"
#include "api/peer_connection_interface.h"
#include "api/media_stream_interface.h"
#include "media-io/audio-io.h"
#include "rtc_base/ref_counted_object.h"

using namespace webrtc;

// Takes audio data from OBS, juggles the frames into a shape webrtc likes, and sends it to sink_, which is provided by webrtc
class EvercastAudioSource : public Notifier<AudioSourceInterface> {
public:
	static rtc::scoped_refptr<EvercastAudioSource> Create(cricket::AudioOptions *options);

	SourceState state() const override { return kLive; }
	bool remote() const override { return false; }

	const cricket::AudioOptions options() const override
	{
		return options_;
	}

	~EvercastAudioSource();

	void AddSink(AudioTrackSinkInterface *sink) override;
	void RemoveSink(AudioTrackSinkInterface *sink) override;
	void OnAudioData(audio_data *frame);

protected:
	audio_t *audio_;
	uint16_t pending_remainder;
	uint8_t *pending;
	cricket::AudioOptions options_;
	AudioTrackSinkInterface *sink_;
	std::vector<std::vector<float>> conversion;


	EvercastAudioSource();
	void Initialize(audio_t *audio, cricket::AudioOptions *options);
	void TranscodeAudio(audio_data *frame, int input_channels, int output_channels, uint8_t *output, int output_len);
};

#endif
