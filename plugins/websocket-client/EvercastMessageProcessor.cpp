#include "EvercastMessageProcessor.h"
#include "Evercast.h"
#include "EvercastStreamInfo.h"
#include <util/base.h>
#include <util/stream-config.h>

#ifdef __APPLE__
#include <util/apple/osx-version.h>
#elif _WIN32
#include <util/windows/win-version.h>
#include <sstream>
#endif

#ifdef __APPLE__

static string getOperatingSystemVersion()
{
	return "OSX " + string(getOSXVersion());
}

#elif _WIN32

static string getOperatingSystemVersion()
{
	stringstream ss;
	struct win_version_info versionInfo;

	get_win_ver(&versionInfo);
	ss << "Windows " << versionInfo.major << "." << versionInfo.minor << "."
	   << versionInfo.build << "." << versionInfo.revis;
	return ss.str();
}

#else

static string getOperatingSystemVersion()
{
	return "Unknown";
}

#endif

JanusMessageProcessor *
EvercastMessageProcessor::create(const string &url, const string &room,
				 const string &username, const string &token,
				 WebsocketSender *sender,
				 WebsocketClient::Listener *listener)
{
	return (JanusMessageProcessor *)new EvercastMessageProcessor(
		url, room, username, token, sender, listener);
}

static void handleChange(void *reference)
{
	EvercastStreamInfo::instance()->refreshStreamType();
	EvercastMessageProcessor *processor =
		(EvercastMessageProcessor *)reference;
	processor->sendEndStreamMessage();
	processor->sendStartStreamMessage();
}

stream_config_change_callback changeHandler = handleChange;

bool EvercastMessageProcessor::sendLoginMessage()
{
	// Login command
	json login = {
		{"janus", "create"},
		{"transaction", to_string(rand())},
		{"payload",
		 {{"username", username}, {"token", token}, {"room", room}}}};

	return sender->sendMessage(login, "login");
}

bool EvercastMessageProcessor::sendStartStreamMessage()
{
	string os = getOperatingSystemVersion();

	EvercastStreamInfo *streamInfo = EvercastStreamInfo::instance();
	json startStream = {
		{"event", START_NATIVE_DESKTOP_STREAM},
		{"transaction", to_string(rand())},
		{"body",
		 {{"platform", "EBS"},
		  {"streamType", streamInfo->streamType()},
		  {"os", os},
		  {"userId", streamInfo->userId()},
		  {"roomId", streamInfo->roomId()},
		  {"resolution", streamInfo->resolution()},
		  {"framerate", streamInfo->framerate()},
		  {"colorSpace", streamInfo->colorSpace()},
		  {"streamId", streamInfo->streamId()}}},
	};

	// TODO: Send instead of logging (is logged during send)
	blog(LOG_ERROR, "Sending %s message...", START_NATIVE_DESKTOP_STREAM);
	blog(LOG_ERROR, "MESSAGE: %s\n", startStream.dump().c_str());
	// return sender->sendMessage(startStream, START_NATIVE_DESKTOP_STREAM);
}

bool EvercastMessageProcessor::sendEndStreamMessage()
{
	EvercastStreamInfo *streamInfo = EvercastStreamInfo::instance();

	json endStream = {{"event", END_NATIVE_DESKTOP_STREAM},
			  {"transaction", to_string(rand())},
			  {"body", {{"streamId", streamInfo->streamId()}}}};

	// TODO: Send instead of logging (is logged during send)
	blog(LOG_ERROR, "Sending %s message...", END_NATIVE_DESKTOP_STREAM);
	blog(LOG_ERROR, "MESSAGE: %s\n", endStream.dump().c_str());
	// return sender->sendMessage(endStream, END_NATIVE_DESKTOP_STREAM);
}

bool EvercastMessageProcessor::sendAttachMessage()
{
	// Create handle command
	json attachPlugin = {{"janus", "attach"},
			     {"transaction", to_string(rand())},
			     {"session_id", session_id},
			     {"plugin", "janus.plugin.lua"}};

	return sender->sendMessage(attachPlugin, "attach");
}

bool EvercastMessageProcessor::sendJoinMessage(string room)
{
	json joinRoom = {{"janus", "message"},
			 {"transaction", to_string(rand())},
			 {"session_id", session_id},
			 {"handle_id", handle_id},
			 {"body",
			  {{"room", room},
			   {"display", "EBS"},
			   {"ptype", "publisher"},
			   {"request", "join"}}}};

	return sender->sendMessage(joinRoom, "join");
}

bool EvercastMessageProcessor::sendDestroyMessage()
{
	json destroy = {{"janus", "destroy"},
			{"session_id", session_id},
			{"transaction", to_string(rand())}};

	return sender->sendMessage(destroy, "destroy");
}

void EvercastMessageProcessor::processResponseEvent(json &msg)
{
	VideoRoomMessageProcessor::processResponseEvent(msg);

	// See if the inbound message is plugin data
	if (processPluginData(msg)) {
		return;
	}
}

void EvercastMessageProcessor::processErrorEvent(int errorCode, json &msg)
{
	switch (errorCode) {
	case EVERCAST_ERR_DUPLICATE_USER:
		// Launch logged event: someone else is logged in
		listener->onLoggedError(-EVERCAST_ERR_DUPLICATE_USER);
		break;
	case EVERCAST_ERR_UNSUPPORTED_AUDIO_CODEC:
		blog(LOG_ERROR,
		     "Janus room does not support the audio codec specified.");
		listener->onLoggedError(-EVERCAST_ERR_UNSUPPORTED_AUDIO_CODEC);
		break;
	default:
		VideoRoomMessageProcessor::processErrorEvent(errorCode, msg);
		break;
	}
}

bool EvercastMessageProcessor::processPluginData(json &msg)
{
	if (msg.find("plugindata") == msg.end()) {
		return false;
	}

	auto plugindata = msg["plugindata"];
	if (plugindata.find("data") == plugindata.end()) {
		return false;
	}

	auto data = plugindata["data"];

	if (data.find("videoroom") == data.end()) {
		return false;
	}

	if (processJoinResponse(data)) {
		return true;
	}

	if (processArriveResponse(data)) {
		return true;
	}

	if (processLeaveResponse(data)) {
		return true;
	}

	return false;
}

bool EvercastMessageProcessor::processJoinResponse(json &data)
{
	auto videoroomEvent = data["videoroom"];
	if (videoroomEvent != "joined") {
		return false;
	}

	parseAttendees(data);
	parseIceServers(data);

	return true;
}

bool EvercastMessageProcessor::processArriveResponse(json &data)
{
	if (data.find("event") == data.end()) {
		return false;
	}

	auto joiningEvent = data["event"];
	if (joiningEvent != "joining") {
		return false;
	}

	AttendeeIdentifier identifier;
	identifier.display = data["display"].get<string>();
	identifier.id = data["id"].get<string>();

	EvercastSessionData *session = getSession();
	session->attendeeArrived(identifier);

	return false;
}

bool EvercastMessageProcessor::processLeaveResponse(json &data)
{
	if (data.find("unpublished") == data.end()) {
		return false;
	}

	auto leaving = data["unpublished"];

	EvercastSessionData *session = getSession();
	session->attendeeLeft(leaving);

	return true;
}

void EvercastMessageProcessor::parseAttendees(json &data)
{
	vector<AttendeeIdentifier> attendees;
	if (data.find("attendees") == data.end()) {
		defineAttendees(attendees);
	}

	auto raw = data["attendees"];
	for (auto &element : raw.items()) {
		AttendeeIdentifier identifier;
		auto value = element.value();
		identifier.id = value["id"].get<string>();
		identifier.display = value["display"].get<string>();
		attendees.push_back(identifier);
	}

	defineAttendees(attendees);
}

void EvercastMessageProcessor::defineAttendees(
	vector<AttendeeIdentifier> &attendees)
{
	// Store attendees in sesion object shared with owner
	EvercastSessionData *session = getSession();
	session->storeAttendees(attendees);
}

void EvercastMessageProcessor::parseIceServers(json &data)
{
	// Ice servers
	vector<IceServerDefinition> ice_servers;
	if (data.find("iceServers") == data.end()) {
		defineIceServers(ice_servers);
		return;
	}

	// Parse servers from JSON
	auto servers = data["iceServers"];
	for (auto &element : servers.items()) {
		IceServerDefinition serv;
		auto value = element.value();
		serv.urls = value["urls"].get<string>();
		if (!value.at("username").is_null())
			serv.username = value["username"].get<string>();
		if (!value.at("credential").is_null())
			serv.password = value["credential"].get<string>();
		ice_servers.push_back(serv);
	}

	defineIceServers(ice_servers);
}

void EvercastMessageProcessor::defineIceServers(
	vector<IceServerDefinition> &ice_servers)
{
	// Fall back to default values.
	if (ice_servers.empty()) {
		IceServerDefinition server;
		server.urls = "stun:stun.l.google.com:19302";
		ice_servers.push_back(server);
	}

	// Store servers in sesion object shared with owner
	EvercastSessionData *session = getSession();
	session->storeIceServers(ice_servers);
}

EvercastSessionData *EvercastMessageProcessor::getSession()
{
	return EvercastSessionData::findOrCreateSession(
		(long long)sender->getId());
}

void EvercastMessageProcessor::afterStreamStarted() {
	VideoRoomMessageProcessor::afterStreamStarted();
	sendStartStreamMessage();
	startStreamInfoNotifications();
}

void EvercastMessageProcessor::beforeStreamEnded() {
	endStreamInfoNotifications();
	sendEndStreamMessage();
	VideoRoomMessageProcessor::beforeStreamEnded();
}

void EvercastMessageProcessor::startStreamInfoNotifications()
{
	stream_config_register_change_callback(changeHandler, this);
	EvercastStreamInfo::instance()->refreshStreamType();
}

void EvercastMessageProcessor::endStreamInfoNotifications()
{
	stream_config_unregister_change_callback(changeHandler, this);
}

