#ifndef __JANUS_MESSAGE_PROCESSOR_H__
#define __JANUS_MESSAGE_PROCESSOR_H__

#include "WebsocketClient.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

class JanusMessageProcessor {
public:
	virtual void processServerMessage(json &msg) = 0;
	virtual bool sendKeepAliveMessage() = 0;
	virtual bool sendTrickleMessage(const std::string &mid, int index, const std::string &candidate, bool last) = 0;
	virtual bool sendOpenMessage(const std::string &sdp, const std::string &video_codec, const std::string &audio_codec) = 0;
	virtual bool onOpened(std::string username, std::string token, std::string room) = 0;
	virtual bool sendLoginMessage(std::string username, std::string token, std::string room) = 0;
	virtual bool sendAttachMessage() = 0;
	virtual bool sendJoinMessage(std::string room) = 0;
	virtual bool sendDestroyMessage() = 0;
	virtual void close() = 0;
};

#endif

