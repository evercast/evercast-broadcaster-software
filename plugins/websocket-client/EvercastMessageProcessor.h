#ifndef __EVERCAST_MESSAGE_PROCESSOR_H__
#define __EVERCAST_MESSAGE_PROCESSOR_H__

#include "VideoRoomMessageProcessor.h"
#include "WebsocketClient.h"
#include "WebsocketSender.h"
#include "EvercastSessionData.h"

using namespace std;

class EvercastMessageProcessor : public VideoRoomMessageProcessor {
public:
	static JanusMessageProcessor * create(
		const string &url,
		const string &room,
		const string &username,
		const string &token,
		WebsocketSender *sender,
		WebsocketClient::Listener* listener);

	EvercastMessageProcessor(
		const string &url,
		const string &room,
		const string &username,
		const string &token,
		WebsocketSender *sender,
		WebsocketClient::Listener* listener) :
		VideoRoomMessageProcessor(url, room, username, token, sender, listener) {};

	bool sendLoginMessage() override;
	bool sendAttachMessage() override;
	bool sendJoinMessage(string room) override;
	bool sendDestroyMessage() override;

protected:
	void processErrorEvent(int errorCode, json &msg) override;
	void processResponseEvent(json &msg) override;

private:
	bool processPluginData(json& msg);
	bool processJoinResponse(json& data);
	bool processArriveResponse(json& data);
	bool processLeaveResponse(json& data);
	void parseAttendees(json& data);
	void defineAttendees(vector<AttendeeIdentifier>& attendees);
	void parseIceServers(json &data);
	void defineIceServers(vector<IceServerDefinition> &ice_servers);
	EvercastSessionData* getSession();
};

#endif

