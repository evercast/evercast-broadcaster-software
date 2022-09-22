#include "WebRTCStream.h"
#include "SDPModif.h"

#include "media-io/video-io.h"

#include "opus_audio_encoder_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "api/video/i420_buffer.h"
#include "api/video/i444_buffer.h"
#include "api/peer_connection_interface.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "pc/rtc_stats_collector.h"
#include "rtc_base/checks.h"
#include <libyuv.h>
#include "Evercast.h"
#include "EvercastSessionData.h"

#include <algorithm>
#include <chrono>
#include <iterator>
#include <memory>
#include <mutex>
#include <thread>
#include <algorithm>
#include <locale>

#define debug(format, ...) blog(LOG_DEBUG, format, ##__VA_ARGS__)
#define info(format, ...) blog(LOG_INFO, format, ##__VA_ARGS__)
#define warn(format, ...) blog(LOG_WARNING, format, ##__VA_ARGS__)
#define error(format, ...) blog(LOG_ERROR, format, ##__VA_ARGS__)

class StatsCallback : public webrtc::RTCStatsCollectorCallback {
public:
    rtc::scoped_refptr<const webrtc::RTCStatsReport> report() { return report_; }
    bool called() const { return called_; }
protected:
    void OnStatsDelivered(
            const rtc::scoped_refptr<const webrtc::RTCStatsReport> &report) override
    {
        report_ = report;
        called_ = true;
    }
private:
    bool called_ = false;
    rtc::scoped_refptr<const webrtc::RTCStatsReport> report_;
};

class CustomLogger : public rtc::LogSink {
public:
    void OnLogMessage(const std::string &message) override
    {
        info("%s", message.c_str());
    }
};

CustomLogger logger;

WebRTCStream::WebRTCStream(obs_output_t *output)
{
    rtc::LogMessage::RemoveLogToStream(&logger);
    rtc::LogMessage::AddLogToStream(&logger, rtc::LoggingSeverity::LS_VERBOSE);

    frame_id = 0;
    pli_received = 0;
    audio_bytes_sent = 0;
    video_bytes_sent = 0;
    total_bytes_sent = 0;

    audio_bitrate = 128;
    video_bitrate = 2500;

    // Store output
    this->output = output;
    this->client = nullptr;
    this->connection_invalidated = false;

    struct obs_video_info ovi;
    obs_get_video_info(&ovi);
    videoFormat = ovi.output_format;

    // Create audio device module
    adm = new rtc::RefCountedObject<AudioDeviceModuleWrapper>();

    // Network thread
    network = rtc::Thread::CreateWithSocketServer();
    network->SetName("network", nullptr);
    network->Start();

    // Worker thread
    worker = rtc::Thread::Create();
    worker->SetName("worker", nullptr);
    worker->Start();

    // Signaling thread
    signaling = rtc::Thread::Create();
    signaling->SetName("signaling", nullptr);
    signaling->Start();

    factory = webrtc::CreatePeerConnectionFactory(
            network.get(),
            worker.get(),
            signaling.get(),
            adm,
            webrtc::CreateOpusAudioEncoderFactory(),
            webrtc::CreateBuiltinAudioDecoderFactory(),
            webrtc::CreateBuiltinVideoEncoderFactory(),
            webrtc::CreateBuiltinVideoDecoderFactory(),
            nullptr,
            nullptr);

    // Create video capture module
    videoCapturer = new rtc::RefCountedObject<VideoCapturer>();
}

WebRTCStream::~WebRTCStream()
{
    rtc::LogMessage::RemoveLogToStream(&logger);

    // Shutdown websocket connection and close Peer Connection
    close(false);

    // Free factories
    adm = nullptr;
    pc = nullptr;
    factory = nullptr;
    videoCapturer = nullptr;

    // Stop all threads
    if (!network->IsCurrent())
        network->Stop();
    if (!worker->IsCurrent())
        worker->Stop();
    if (!signaling->IsCurrent())
        signaling->Stop();

    network.release();
    worker.release();
    signaling.release();
}

bool WebRTCStream::start(WebRTCStream::Type type)
{
    info("WebRTCStream::start");
    this->type = type;

    obs_service_t *service = obs_output_get_service(output);
    if (!service) {
        obs_output_set_last_error(output, "An unexpected error occurred during stream startup.  Please ensure you have selected an Evercast stream.");
        obs_output_signal_stop(output, OBS_OUTPUT_CONNECT_FAILED);
        return false;
    }

    url = obs_service_get_url(service) ? obs_service_get_url(service) : "";
    room = obs_service_get_room(service) ? obs_service_get_room(service) : "";
    username = obs_service_get_username(service) ? obs_service_get_username(service) : "";
    password = obs_service_get_password(service) ? obs_service_get_password(service) : "";
    video_codec = obs_service_get_codec(service) ? obs_service_get_codec(service) : "";
    protocol = obs_service_get_protocol(service) ? obs_service_get_protocol(service) : "";

    // Stream settings sanity check
    // NOTE: Username checks out of scope for Evercast and not implemented here
    bool isServiceValid = true;
    if (type != WebRTCStream::Type::Millicast) {
        if (url.empty()) {
            warn("Invalid url");
            isServiceValid = false;
        }

        if (room.empty()) {
            warn("Missing room ID");
            isServiceValid = false;
        }
    }

    if (type != WebRTCStream::Type::Wowza) {
        if (password.empty()) {
            warn("Missing room key");
            isServiceValid = false;
        }
    }

    if (!isServiceValid) {
        obs_output_set_last_error(
            output,
            "You have not added any EBS keys. For more information please visit "
            "<a href=\"https://guides.evercast.us/troubleshooting/evercast/adding-updating-ebs-stream-settings\">this link</a>.");
        obs_output_signal_stop(output, OBS_OUTPUT_CONNECT_FAILED);
        return false;
    }

    obs_output_t *context = output;

    obs_encoder_t *aencoder = obs_output_get_audio_encoder(context, 0);
    obs_data_t *asettings = obs_encoder_get_settings(aencoder);
    audio_bitrate = (int)obs_data_get_int(asettings, "bitrate");
    obs_data_release(asettings);

    obs_encoder_t *vencoder = obs_output_get_video_encoder(context);
    obs_data_t *vsettings = obs_encoder_get_settings(vencoder);
    video_bitrate = (int)obs_data_get_int(vsettings, "bitrate");
    obs_data_release(vsettings);

    struct obs_audio_info audio_info;
    if (!obs_get_audio_info(&audio_info)) {
	warn("Failed to load audio settings.  Defaulting to opus.");
        audio_codec = "opus";
    } else {
        channel_count = (int)(audio_info.speakers);
        audio_codec = channel_count <= 2 ? "opus" : "multiopus";
    }

    // Shutdown websocket connection and close Peer Connection (just in case)
    if (close(false))
        obs_output_signal_stop(output, OBS_OUTPUT_ERROR);

    if (!startWebSocket(type))
        return false;

    if (!startPeerConnection())
        return false;

    return true;
}

void WebRTCStream::recordConnectionError(std::string message)
{
	warn("Error connecting to server");

	// Shutdown websocket connection and close Peer Connection
	close(false);
	obs_output_set_last_error(output, message.c_str());
		
	// Disconnect, this will call stop on main thread
	obs_output_signal_stop(output, OBS_OUTPUT_CONNECT_FAILED);
}

bool WebRTCStream::startWebSocket(WebRTCStream::Type type)
{
	client = createWebsocketClient(type);
	if (!client) {
		warn("Error creating Websocket client");
		// Close Peer Connection
		close(false);
		// NOTE: Placeholder message
		obs_output_set_last_error(
			output,
			"There was a problem connecting to Evercast.  Are you behind a firewall?");
		// Disconnect, this will call stop on main thread
		obs_output_signal_stop(output, OBS_OUTPUT_CONNECT_FAILED);
		return false;
	}

	info("Video codec:      %s",
	     video_codec.empty() ? "Automatic" : video_codec.c_str());
	info("Protocol:         %s",
	     protocol.empty() ? "Automatic" : protocol.c_str());

	if (type == WebRTCStream::Type::Janus) {
		info("Server Room:      %s\nStream Key:       %s\n",
		     room.c_str(), password.c_str());
	} else if (type == WebRTCStream::Type::Wowza) {
		info("Application Name: %s\nStream Name:      %s\n",
		     room.c_str(), username.c_str());
	} else if (type == WebRTCStream::Type::Millicast) {
		info("Stream Name:      %s\nPublishing Token: %s\n",
		     username.c_str(), password.c_str());
		url = "Millicast";
	} else if (type == WebRTCStream::Type::Evercast) {
		info("Server Room:      %s\nStream Key:       %s\n",
		     room.c_str(), password.c_str());
	}
	info("CONNECTING TO %s", url.c_str());

	// Connect to server
    this->connection_invalidated = false;
	if (!client->connect(url, room, username, password, this)) {
		recordConnectionError("There was a problem connecting to your Evercast room.  Have you double-checked your room settings?");
		return false;
	}

	if (type == WebRTCStream::Type::Evercast) {
		// Wait for ICE servers to come back from server.  If nothing comes back
		// after the specified timeout has elapsed, continue with a default value.
		EvercastSessionData *session_data =
			EvercastSessionData::findOrCreateSession(
				(long long)client);

		session_data->registerEventHandler(this);

		bool successfullyJoined = session_data->awaitJoinComplete(5);
		if (!successfullyJoined) {
            webrtc::MutexLock lock(&mutex_);
            if (!this->connection_invalidated) {
                this->connection_invalidated = true;
                recordConnectionError("Please make sure there is at least one participant in your Evercast virtual room in order to connect.");
            }
			return false;
		}

		std::vector<IceServerDefinition> servers =
			session_data->getIceServers();
		for (std::vector<IceServerDefinition>::iterator it =
			     servers.begin();
		     it != servers.end(); it++) {
			webrtc::PeerConnectionInterface::IceServer server;
			server.urls = {it->urls};
			server.username = it->username;
			server.password = it->password;
			this->ice_servers.push_back(server);
		}

		if (servers.empty()) {
			return false;
		}
	}

	return true;
}

bool WebRTCStream::startPeerConnection()
{
    webrtc::PeerConnectionInterface::RTCConfiguration config;
    config.servers = this->ice_servers;
    // config.bundle_policy = webrtc::PeerConnectionInterface::kBundlePolicyMaxBundle;
    // config.disable_ipv6 = true;
    // config.rtcp_mux_policy = webrtc::PeerConnectionInterface::kRtcpMuxPolicyRequire;
    // config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
    // config.set_cpu_adaptation(false);
    // config.set_suspend_below_min_bitrate(false);

    webrtc::PeerConnectionDependencies dependencies(this);

    pc = factory->CreatePeerConnection(config, std::move(dependencies));

    if (!pc.get()) {
        error("Error creating Peer Connection");
        // NOTE: Placeholder message
        obs_output_set_last_error(output, "There was an error connecting to the server.  Are you connected to the internet?");
        obs_output_signal_stop(output, OBS_OUTPUT_CONNECT_FAILED);
        return false;
    } else {
        info("PEER CONNECTION CREATED\n");
    }

    cricket::AudioOptions options;
    options.echo_cancellation.emplace(false); // default: true
    options.auto_gain_control.emplace(false); // default: true
    options.noise_suppression.emplace(false); // default: true
    options.highpass_filter.emplace(false); // default: true
    options.stereo_swapping.emplace(false);
    options.typing_detection.emplace(false); // default: true
    options.experimental_agc.emplace(false);
    options.experimental_ns.emplace(false);
    options.residual_echo_detector.emplace(false); // default: true
    // options.tx_agc_limiter.emplace(false);

    stream = factory->CreateLocalMediaStream("obs");

    audio_source = EvercastAudioSource::Create(&options);
    audio_track = factory->CreateAudioTrack("audio", audio_source);
    // pc->AddTrack(audio_track, {"obs"});
    stream->AddTrack(audio_track);
    
    video_track = factory->CreateVideoTrack("video", videoCapturer);
    // pc->AddTrack(video_track, {"obs"});
    stream->AddTrack(video_track);

    // Add the stream to the peer connection
    if (!pc->AddStream(stream)) {
        warn("Adding stream to PeerConnection failed");
        // Close Peer Connection
        close(false);
        // Disconnect, this will call stop on main thread
        obs_output_set_last_error(output, "There was a problem connecting your output to the stream.  Do your sources appear to be working correctly?");
        obs_output_signal_stop(output, OBS_OUTPUT_CONNECT_FAILED);
        return false;
    }

    if (type == WebRTCStream::Type::Evercast) {
        // This is done after setup, when ICE servers have been returned from the websocket
        createOffer();
    }

    return true;
}

void WebRTCStream::onConnected()
{
    info("WebRTCStream::onConnected");
}

void WebRTCStream::onLogged(int /* code */)
{
    if (type != WebRTCStream::Type::Evercast) {
        createOffer();
    }
}

void WebRTCStream::createOffer()
{
	info("WebRTCStream::onLogged\nCreating offer...");
	webrtc::PeerConnectionInterface::RTCOfferAnswerOptions offer_options;
	offer_options.voice_activity_detection = false;
	pc->CreateOffer(this, offer_options);
}

void WebRTCStream::handleEmptyRoom()
{
	info("Room is empty.  Shutting down stream...");
	this->close(false);
	obs_output_set_last_error(output, "Everyone has left your room.  Shutting down stream.");
	obs_output_signal_stop(output, OBS_OUTPUT_ERROR);
}

void WebRTCStream::OnSuccess(webrtc::SessionDescriptionInterface *desc)
{
    info("WebRTCStream::OnSuccess\n");
    std::string sdp;
    desc->ToString(&sdp);

    info("Audio codec:      %s", audio_codec.c_str());
    info("Audio bitrate:    %d\n", audio_bitrate);
    info("Video codec:      %s", video_codec.empty() ? "Automatic" : video_codec.c_str());
    info("Video bitrate:    %d\n", video_bitrate);
    info("OFFER:\n\n%s\n", sdp.c_str());

    std::string sdpCopy = sdp;
    if (type == WebRTCStream::Type::Wowza) {
        std::vector<int> audio_payloads;
        std::vector<int> video_payloads;
        // If codec setting is Automatic
        if (video_codec.empty()) {
            audio_codec = "";
            video_codec = "";
        }
        // Force specific video/audio payload
        SDPModif::forcePayload(sdpCopy, audio_payloads, video_payloads,
                audio_codec, video_codec, 0, "42e01f", 0);
        // Constrain video bitrate
        SDPModif::bitrateMaxMinSDP(sdpCopy, video_bitrate, video_payloads);
        // Enable stereo & constrain audio bitrate
        SDPModif::stereoSDP(sdpCopy, audio_bitrate);
    } else if (type == WebRTCStream::Type::Evercast) {
        if (audio_codec == "multiopus") {
            // Modify offer to accept multiopus
            SDPModif::surroundSDP(sdpCopy, channel_count);
        }
    }

    int video_profile = (videoFormat == VIDEO_FORMAT_I444) ? 1 : 0;

    info("SETTING LOCAL DESCRIPTION\n\n");
    pc->SetLocalDescription(this, desc);

    info("Sending OFFER (SDP) to remote peer:\n\n%s", sdpCopy.c_str());
    client->open(sdpCopy, video_codec, audio_codec, video_profile, username);
}

void WebRTCStream::OnSuccess()
{
    info("Local Description set\n");
}

void WebRTCStream::OnFailure(webrtc::RTCError error)
{
    if (error.ok()) {
        return;
    }
    warn("WebRTCStream::OnFailure [%s]", error.message());
    // Shutdown websocket connection and close Peer Connection
    close(false);
    // Disconnect, this will call stop on main thread
    obs_output_signal_stop(output, OBS_OUTPUT_ERROR);
}

void WebRTCStream::OnIceCandidate(const webrtc::IceCandidateInterface *candidate)
{
    std::string str;
    candidate->ToString(&str);
    // Send candidate to remote peer
    client->trickle(candidate->sdp_mid(), candidate->sdp_mline_index(), str, false);
}

void WebRTCStream::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState state /* new_state */)
{
    using namespace webrtc;
    info("WebRTCStream::OnIceConnectionChange [%u]", state);

    switch (state) {
        case PeerConnectionInterface::IceConnectionState::kIceConnectionFailed:
            // Close must be carried out on a separate thread in order to avoid deadlock
            thread_closeAsync = std::thread([&] () {
                close(false);

                obs_output_set_last_error(
                        output,
                        "Evercast found your room, but streaming failed.  Are you behind a firewall?\n\n"
                        "Visit Evercast's <a href=\"https://guides.evercast.us/troubleshooting/security-and-data/security-whitelisting\">security whitelisting page</a> for firewall configuration help.");

                // Disconnect, this will call stop on main thread
                obs_output_signal_stop(output, OBS_OUTPUT_ERROR);
            });
            break;
        default:
            break;
    }
}

void WebRTCStream::onRemoteIceCandidate(const std::string &sdpData)
{
    if (sdpData.empty()) {
        info("ICE COMPLETE\n");
        pc->AddIceCandidate(nullptr);
    } else {
        std::string s = sdpData;
        s.erase(remove(s.begin(), s.end(), '\"'), s.end());
        if (protocol.empty() || SDPModif::filterIceCandidates(s, protocol)) {
            const std::string candidate = s;
            info("Remote %s\n", candidate.c_str());
            const std::string sdpMid = "";
            int sdpMLineIndex = 0;
            webrtc::SdpParseError error;
            const webrtc::IceCandidateInterface *newCandidate =
                    webrtc::CreateIceCandidate(sdpMid, sdpMLineIndex,
                            candidate, &error);
            pc->AddIceCandidate(newCandidate);
        } else {
            info("Ignoring remote %s\n", s.c_str());
        }
    }
}

void WebRTCStream::onOpened(const std::string &sdp)
{
    info("ANSWER:\n\n%s\n", sdp.c_str());

    std::string sdpCopy = sdp;

    if (type != WebRTCStream::Type::Wowza) {
        // Constrain video bitrate
        SDPModif::bitrateSDP(sdpCopy, video_bitrate);
        // Enable stereo & constrain audio bitrate
        SDPModif::stereoSDP(sdpCopy, audio_bitrate);
    }

    // SetRemoteDescription observer
    srd_observer = make_scoped_refptr(this);

    // Create ANSWER from sdpCopy
    webrtc::SdpParseError error;
    std::unique_ptr<webrtc::SessionDescriptionInterface> answer =
            webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, sdpCopy, &error);

    info("SETTING REMOTE DESCRIPTION\n\n%s", sdpCopy.c_str());
    pc->SetRemoteDescription(std::move(answer), srd_observer);

    // Set audio conversion info
    audio_convert_info conversion;
    conversion.format = AUDIO_FORMAT_16BIT;
    conversion.samples_per_sec = 48000;
    conversion.speakers = (speaker_layout)channel_count;
    obs_output_set_audio_conversion(output, &conversion);

    info("Begin data capture...");
    obs_output_begin_data_capture(output, 0);
}

void WebRTCStream::OnSetRemoteDescriptionComplete(webrtc::RTCError error)
{
    if (error.ok())
        info("Remote Description set\n");
    else
        warn("Error setting Remote Description: %s\n", error.message());
}

bool WebRTCStream::close(bool wait)
{
    if (!pc.get())
        return false;
    // Get pointer
    auto old = pc.release();
    // Close Peer Connection
    old->Close();
    // Shutdown websocket connection
    if (client) {
        client->disconnect(wait);
    }
    return true;
}

bool WebRTCStream::stop()
{
    info("WebRTCStream::stop");
    // Shutdown websocket connection and close Peer Connection
    close(true);
    // Disconnect, this will call stop on main thread
    obs_output_end_data_capture(output);
    return true;
}

void WebRTCStream::onDisconnected()
{
    info("WebRTCStream::onDisconnected");
    // Make sure any other disconnect attempts have finished before proceeding
    if (thread_closeAsync.joinable()) {
        thread_closeAsync.join();
    }

    // Shutdown websocket connection and close Peer Connection
    close(false);
    // Disconnect, this will call stop on main thread
    obs_output_signal_stop(output, OBS_OUTPUT_DISCONNECTED);
}

void WebRTCStream::onLoggedError(int code)
{
    info("WebRTCStream::onLoggedError [code: %d]", code);
    // We have already given up on the connection.	
    webrtc::MutexLock lock(&mutex_);
    if (this->connection_invalidated) {	
        return;	
    }	

    this->connection_invalidated = true;
    if (client) {	
        client->disconnect(false);	
    }	

    // Close Peer Connection
    const char *error;
    switch (code)
    {
    case -EVERCAST_ERR_UNSUPPORTED_AUDIO_CODEC:
        error = "Your Evercast room does not support surround sound.  Try setting output to stereo, then reconnect.";
        break;
    case -EVERCAST_ERR_DUPLICATE_USER:
        error = "Another instance of EBS with the same user account is already broadcasting to this room.";
        break;
    default:
        error = "Evercast is having trouble connecting to your room.  Are you behind a firewall?\n\n"
        "Visit Evercast's <a href=\"https://guides.evercast.us/troubleshooting/security-and-data/security-whitelisting\">security whitelisting page</a> for firewall configuration help.";
    }

    obs_output_set_last_error(output, error);

    // Disconnect, this will call stop on main thread
    obs_output_signal_stop(output, OBS_OUTPUT_ERROR);
}

void WebRTCStream::onOpenedError(int code)
{
    info("WebRTCStream::onOpenedError [code: %d]", code);
    // Shutdown websocket connection and close Peer Connection
    close(false);
    // Disconnect, this will call stop on main thread
    obs_output_signal_stop(output, OBS_OUTPUT_ERROR);
}

void WebRTCStream::onAudioFrame(audio_data *frame)
{
    if (!frame)
        return;
    // Push it to the device
    audio_source->OnAudioData(frame);
}

webrtc::VideoFrame WebRTCStream::constructOutputFrame(video_data* frame)
{
    // Calculate size
    int outputWidth = obs_output_get_width(output);
    int outputHeight = obs_output_get_height(output);
    int target_width = abs(outputWidth);
    int target_height = abs(outputHeight);
    libyuv::RotationMode rotation_mode = libyuv::kRotate0;

    webrtc::VideoType videoType;
    rtc::scoped_refptr<VideoFrameBuffer> buffer;
    if (videoFormat == VIDEO_FORMAT_I444) {
        int stride = outputWidth;
        // Convert frame
        rtc::scoped_refptr<webrtc::I444Buffer> i444Buffer = webrtc::I444Buffer::Create(target_width, target_height, stride);
	memcpy(i444Buffer.get()->MutableDataY(), frame->data[0], target_width * target_height);
	memcpy(i444Buffer.get()->MutableDataU(), frame->data[1], target_width * target_height);
	memcpy(i444Buffer.get()->MutableDataV(), frame->data[2], target_width * target_height);

	buffer = i444Buffer;
    } else {

	uint8_t* frameData = frame->data[0];
	std::unique_ptr<uint8_t[]> convertedFrameData;

	switch (videoFormat) {
	case VIDEO_FORMAT_NV12:
	    videoType = webrtc::VideoType::kNV12;
	    break;

	case VIDEO_FORMAT_I420:
	    videoType = webrtc::VideoType::kI420;
	    break;
	case VIDEO_FORMAT_RGBA:
	    videoType = webrtc::VideoType::kARGB;
	    unsigned int frameSizeBytes = frame->linesize[0] * outputHeight * 4;
	    convertedFrameData.reset(new uint8_t[frameSizeBytes]);
	    libyuv::ARGBToABGR(frameData, frame->linesize[0], convertedFrameData.get(), frame->linesize[0], outputWidth, outputHeight);
	    frameData = convertedFrameData.get();
	    break;
	}

	uint32_t size = outputWidth * outputHeight * 3 / 2;
	int stride_y = outputWidth;
	int stride_uv = (outputWidth + 1) / 2;

        // Convert frame
        rtc::scoped_refptr<webrtc::I420Buffer> i420Buffer = webrtc::I420Buffer::Create(target_width, target_height, stride_y, stride_uv, stride_uv);

        const int conversionResult = libyuv::ConvertToI420(
	    frameData, size,
            i420Buffer.get()->MutableDataY(), i420Buffer.get()->StrideY(),
            i420Buffer.get()->MutableDataU(), i420Buffer.get()->StrideU(),
            i420Buffer.get()->MutableDataV(), i420Buffer.get()->StrideV(), 0, 0,
            outputWidth, outputHeight, target_width, target_height,
            rotation_mode, ConvertVideoType(videoType));

        // not using the result yet, silence compiler
        (void)conversionResult;

	buffer = i420Buffer;
    }

    const int64_t obs_timestamp_us =
            (int64_t)frame->timestamp / rtc::kNumNanosecsPerMicrosec;

    // Align timestamps from OBS capturer with rtc::TimeMicros timebase
    const int64_t aligned_timestamp_us =
            timestamp_aligner_.TranslateTimestamp(obs_timestamp_us, rtc::TimeMicros());

    // Create a webrtc::VideoFrame to pass to the capturer
    return webrtc::VideoFrame::Builder()
            .set_video_frame_buffer(buffer)
            .set_rotation(webrtc::kVideoRotation_0)
            .set_timestamp_us(aligned_timestamp_us)
            .set_id(++frame_id)
            .build();
}

void WebRTCStream::onVideoFrame(video_data *frame)
{
    if (!frame)
        return;
    if (!videoCapturer)
        return;

    if (std::chrono::system_clock::time_point(std::chrono::duration<int>(0)) == previous_time)
      // First frame sent: Initialize previous_time
      previous_time = std::chrono::system_clock::now();

    webrtc::VideoFrame video_frame = constructOutputFrame(frame);

    // Send frame to video capturer
    videoCapturer->OnFrameCaptured(video_frame);
}

// NOTE LUDO: #80 add getStats
void WebRTCStream::getStats()
{
  rtc::scoped_refptr<const webrtc::RTCStatsReport> report = NewGetStats();
  if (nullptr == report)
  {
    return;
  }

  stats_list = "";

  std::vector<const webrtc::RTCOutboundRTPStreamStats*> send_stream_stats =
          report->GetStatsOfType<webrtc::RTCOutboundRTPStreamStats>();
  for (const auto& stat : send_stream_stats) {
    if (stat->kind.ValueToString() == "audio") {
      audio_bytes_sent = std::stoll(stat->bytes_sent.ValueToJson());
    }
    if (stat->kind.ValueToString() == "video") {
      video_bytes_sent = std::stoll(stat->bytes_sent.ValueToJson());
      pli_received = std::stoi(stat->pli_count.ValueToJson());
    }
  }
  total_bytes_sent = audio_bytes_sent + video_bytes_sent;

  // RTCDataChannelStats
  std::vector<const webrtc::RTCDataChannelStats*> data_channel_stats =
          report->GetStatsOfType<webrtc::RTCDataChannelStats>();
  for (const auto& stat : data_channel_stats) {
    stats_list += "data_messages_sent:"     + stat->messages_sent.ValueToJson() + "\n";
    stats_list += "data_bytes_sent:"        + stat->bytes_sent.ValueToJson() + "\n";
    stats_list += "data_messages_received:" + stat->messages_received.ValueToJson() + "\n";
    stats_list += "data_bytes_received:"    + stat->bytes_received.ValueToJson() + "\n";
  }

  // RTCMediaStreamTrackStats
  // double audio_track_jitter_buffer_delay = 0.0;
  // double video_track_jitter_buffer_delay = 0.0;
  // uint64_t audio_track_jitter_buffer_emitted_count = 0;
  // uint64_t video_track_jitter_buffer_emitted_count = 0;
  std::vector<const webrtc::RTCMediaStreamTrackStats*> media_stream_track_stats =
          report->GetStatsOfType<webrtc::RTCMediaStreamTrackStats>();
  for (const auto& stat : media_stream_track_stats) {
    if (stat->kind.ValueToString() == "audio") {
      // stats_list += "track_audio_level:"            + stat->audio_level.ValueToJson() + "\n";
      // stats_list += "track_total_audio_energy:"     + stat->total_audio_energy.ValueToJson() + "\n";
      // stats_list += "track_echo_return_loss:"   + stat->echo_return_loss.ValueToJson() + "\n";
      // stats_list += "track_echo_return_loss_enhancement:" + stat->echo_return_loss_enhancement.ValueToJson() + "\n";
      // stats_list += "track_total_samples_received:" + stat->total_samples_received.ValueToJson() + "\n";
      // stats_list += "track_total_samples_duration:" + stat->total_samples_duration.ValueToJson() + "\n";
      // stats_list += "track_concealed_samples:" + stat->concealed_samples.ValueToJson() + "\n";
      // stats_list += "track_concealment_events:" + stat->concealment_events.ValueToJson() + "\n";
    }
    if (stat->kind.ValueToString() == "video") {
      // stats_list += "track_jitter_buffer_delay:"    + stat->jitter_buffer_delay.ValueToJson() + "\n";
      // stats_list += "track_jitter_buffer_emitted_count:" + stat->jitter_buffer_emitted_count.ValueToJson() + "\n";
      stats_list += "track_frame_width:"            + stat->frame_width.ValueToJson() + "\n";
      stats_list += "track_frame_height:"           + stat->frame_height.ValueToJson() + "\n";
      stats_list += "track_frames_sent:"            + stat->frames_sent.ValueToJson() + "\n";
      stats_list += "track_huge_frames_sent:"       + stat->huge_frames_sent.ValueToJson() + "\n";
 
      // FPS = frames_sent / (time_now - start_time)
      std::chrono::system_clock::time_point time_now = std::chrono::system_clock::now();
      std::chrono::duration<double> elapsed_seconds = time_now - previous_time;
      previous_time = time_now;
      uint32_t frames_sent = std::stol(stat->frames_sent.ValueToJson());
      stats_list += "track_fps:" + std::to_string((frames_sent - previous_frames_sent) / elapsed_seconds.count()) + "\n";
      previous_frames_sent = frames_sent;

      // stats_list += "track_frames_received:"        + stat->frames_received.ValueToJson() + "\n";
      // stats_list += "track_frames_decoded:"         + stat->frames_decoded.ValueToJson() + "\n";
      // stats_list += "track_frames_dropped:"         + stat->frames_dropped.ValueToJson() + "\n";
      // stats_list += "track_frames_corrupted:"       + stat->frames_corrupted.ValueToJson() + "\n";
      // stats_list += "track_partial_frames_lost:"    + stat->partial_frames_lost.ValueToJson() + "\n";
      // stats_list += "track_full_frames_lost:"       + stat->full_frames_lost.ValueToJson() + "\n";
    }
  }

  // RTCPeerConnectionStats
  // std::vector<const webrtc::RTCPeerConnectionStats*> pc_stats =
  //         report->GetStatsOfType<webrtc::RTCPeerConnectionStats>();
  // for (const auto& stat : pc_stats) {
  //   stats_list += "data_channels_opened:" + stat->data_channels_opened.ValueToJson() + "\n";
  //   stats_list += "data_channels_closed:" + stat->data_channels_closed.ValueToJson() + "\n";
  // }

  // RTCRTPStreamStats
  // std::vector<const webrtc::RTCRTPStreamStats*> rtcrtp_stats =
  //         report->GetStatsOfType<webrtc::RTCRTPStreamStats>();
  // for (const auto& stat : rtcrtp_stats) {
  //   if (stat->kind.ValueToString() == "video") {
  //     stats_list += "rtcrtp_fir_count:" + stat->fir_count.ValueToJson() + "\n";
  //     stats_list += "rtcrtp_pli_count:" + stat->pli_count.ValueToJson() + "\n";
  //     stats_list += "rtcrtp_nack_count:" + stat->nack_count.ValueToJson() + "\n";
  //     stats_list += "rtcrtp_sli_count:" + stat->sli_count.ValueToJson() + "\n";
  //     stats_list += "rtcrtp_qp_sum:" + stat->qp_sum.ValueToJson() + "\n";
  //   }
  // }

  // RTCInboundRTPStreamStats
  // std::vector<const webrtc::RTCInboundRTPStreamStats*> inbound_stream_stats =
  //         report->GetStatsOfType<webrtc::RTCInboundRTPStreamStats>();
  // for (const auto& stat : inbound_stream_stats) {
  //   stats_list += "inbound_stream_packets_received:"        + stat->packets_received.ValueToJson() + "\n";
  //   stats_list += "inbound_stream_bytes_received:"          + stat->bytes_received.ValueToJson() + "\n";
  //   stats_list += "inbound_stream_packets_lost:"            + stat->packets_lost.ValueToJson() + "\n";

  //   if (stat->kind.ValueToString() == "audio") {
  //     stats_list += "inbound_stream_jitter:" + stat->jitter.ValueToJson() + "\n";
  //   }
  // }

  // RTCOutboundRTPStreamStats
  std::vector<const webrtc::RTCOutboundRTPStreamStats*> outbound_stream_stats =
          report->GetStatsOfType<webrtc::RTCOutboundRTPStreamStats>();
  for (const auto& stat : outbound_stream_stats) {
    if (stat->kind.ValueToString() == "audio") {
      /*
      stats_list += "outbound_audio_packets_sent:"   + stat->packets_sent.ValueToJson() + "\n";
      stats_list += "outbound_audio_bytes_sent:"     + stat->bytes_sent.ValueToJson() + "\n";
      // stats_list += "outbound_audio_target_bitrate:" + stat->target_bitrate.ValueToJson() + "\n";
      // stats_list += "outbound_audio_frames_encoded:" + stat->frames_encoded.ValueToJson() + "\n";
      */
    }
    if (stat->kind.ValueToString() == "video") {
      /*
      stats_list += "outbound_video_packets_sent:"   + stat->packets_sent.ValueToJson() + "\n";
      stats_list += "outbound_video_bytes_sent:"     + stat->bytes_sent.ValueToJson() + "\n";
      // stats_list += "outbound_video_target_bitrate:" + stat->target_bitrate.ValueToJson() + "\n";
      // stats_list += "outbound_video_frames_encoded:" + stat->frames_encoded.ValueToJson() + "\n";
      stats_list += "outbound_video_fir_count:"      + stat->fir_count.ValueToJson() + "\n";
      stats_list += "outbound_video_pli_count:"      + stat->pli_count.ValueToJson() + "\n";
      stats_list += "outbound_video_nack_count:"     + stat->nack_count.ValueToJson() + "\n";
      // stats_list += "outbound_video_sli_count:"      + stat->sli_count.ValueToJson() + "\n";
      stats_list += "outbound_video_qp_sum:"         + stat->qp_sum.ValueToJson() + "\n";
      */
    }
  }

  // RTCTransportStats
  std::vector<const webrtc::RTCTransportStats*> transport_stats =
          report->GetStatsOfType<webrtc::RTCTransportStats>();
  for (const auto& stat : transport_stats) {
    stats_list += "transport_bytes_sent:"     + stat->bytes_sent.ValueToJson() + "\n";
    stats_list += "transport_bytes_received:" + stat->bytes_received.ValueToJson() + "\n";
  }

}

rtc::scoped_refptr<const webrtc::RTCStatsReport> WebRTCStream::NewGetStats()
{
    webrtc::MutexLock lock(&mutex_);

    if (nullptr == pc)
    {
        return nullptr;
    }

    rtc::scoped_refptr<StatsCallback> stats_callback =
            new rtc::RefCountedObject<StatsCallback>();

    pc->GetStats(stats_callback);

    while (!stats_callback->called())
        std::this_thread::sleep_for(std::chrono::microseconds(1));

    rtc::scoped_refptr<const webrtc::RTCStatsReport> result = stats_callback->report();
    return result;
}
