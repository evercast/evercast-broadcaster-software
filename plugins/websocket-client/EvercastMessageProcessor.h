#ifndef __EVERCAST_MESSAGE_PROCESSOR_H__
#define __EVERCAST_MESSAGE_PROCESSOR_H__

#include "VideoRoomMessageProcessor.h"
#include "WebsocketClient.h"
#include "WebsocketSender.h"
#include "EvercastSessionData.h"

class EvercastMessageProcessor : public VideoRoomMessageProcessor {
public:
	static JanusMessageProcessor * create(
		const std::string &url,
		const std::string &room,
		const std::string &username,
		const std::string &token,
		WebsocketSender *sender,
		WebsocketClient::Listener* listener);

	EvercastMessageProcessor(
		const std::string &url,
		const std::string &room,
		const std::string &username,
		const std::string &token,
		WebsocketSender *sender,
		WebsocketClient::Listener* listener) :
		VideoRoomMessageProcessor(url, room, username, token, sender, listener) {};

	bool sendLoginMessage(std::string username, std::string token, std::string room) override;
	bool sendAttachMessage() override;
	bool sendJoinMessage(std::string room) override;
	bool sendDestroyMessage() override;

protected:
	void processResponseEvent(json &msg) override;

private:
	bool processPluginData(json& msg);
	bool processJoinResponse(json& data);
	void parseAttendees(json& data);
	void defineAttendees(std::vector<AttendeeIdentifier>& attendees);
	void parseIceServers(json &data);
	void defineIceServers(std::vector<IceServerDefinition> &ice_servers);
};

#endif

