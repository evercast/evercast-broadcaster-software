#include "VideoRoomMessageProcessor.h"
#include "WebsocketClient.h"
#include <util/base.h>

//Use http://think-async.com/ instead of boost
#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_STL_
#define _WEBSOCKETPP_CPP11_THREAD_
#define _WEBSOCKETPP_CPP11_FUNCTIONAL_
#define _WEBSOCKETPP_CPP11_SYSTEM_ERROR_
#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_CPP11_MEMORY_

#include "websocketpp/client.hpp"

#define warn(format, ...)  blog(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...)  blog(LOG_INFO,    format, ##__VA_ARGS__)
#define debug(format, ...) blog(LOG_DEBUG,   format, ##__VA_ARGS__)
#define error(format, ...) blog(LOG_ERROR,   format, ##__VA_ARGS__)

using namespace std;

JanusMessageProcessor * VideoRoomMessageProcessor::create(
	const string& url,
	const string& room,
	const string& username,
	const string& token,
	WebsocketSender* sender,
	WebsocketClient::Listener* listener)
{
	return (JanusMessageProcessor*)new VideoRoomMessageProcessor(
			url, room, username, token, sender, listener);
}


VideoRoomMessageProcessor::VideoRoomMessageProcessor(
	const string &url, const string &room,
	const string &username, const string &token,
	WebsocketSender *sender, WebsocketClient::Listener *listener)
{
	currentState = VideoRoomState::Created;
	this->url = url;
	this->room = room;
	this->username = username;
	this->token = token;
	this->sender = sender;
	this->listener = listener;
	is_running.store(false);
}

VideoRoomMessageProcessor::~VideoRoomMessageProcessor() {
	close();
}

void VideoRoomMessageProcessor::close()
{
	bool needsJoin = is_running.load();
	if (needsJoin) {
		beforeStreamEnded();
	}

	is_running.store(false);
	if (needsJoin) {
	    if (keepAliveThread.joinable()) {
		    keepAliveThread.join();
	    }
	}
	stateListener.notify_all();
}

void VideoRoomMessageProcessor::processServerMessage(nlohmann::json &msg)
{
	{
		std::lock_guard<std::mutex> lockGuard(timeoutMutex);
		last_message_recd_time = chrono::system_clock::now();
	}

	if (msg.find("janus") == msg.end())
		return;

	// Check if it is an SDP negotiation message
	if (msg.find("jsep") != msg.end()) {
		string sdp = msg["jsep"]["sdp"];
		// Event
		listener->onOpened(sdp);
		return;
	}

	// Check message type
	string messageType = msg["janus"];

	// These come through frequently
	if (messageType == "ack")
		return;

	// Success response handling
	if (messageType.compare("success") == 0) {
		processSuccessMessage(msg);
	} else if (messageType == "event") {
		// Error and custom event response handling
		processEvent(msg);
	} else {
		processUnhandledMessage(messageType, msg);
	}
}

/*********************** INBOUND MESSAGE PROCESSING ***********************/

void VideoRoomMessageProcessor::processSuccessMessage(json &msg)
{
	if (msg.find("transaction") == msg.end())
		return;
	if (msg.find("data") == msg.end())
		return;
	// Get the data session
	auto data = msg["data"];
	// Server is sending response twice, ignore second one
	if (!logged) {
		// Get response code
		session_id = data["id"];
		sendAttachMessage();
		logged = true;

		assignMinimumState(VideoRoomState::LoggedIn);
	} else { // logged
		handle_id = data["id"];
		sendJoinMessage(room);
		afterStreamStarted();
		assignMinimumState(VideoRoomState::Attached);
	}
}

void VideoRoomMessageProcessor::processEvent(json &msg)
{
	int error_code = parsePluginErrorCode(msg);

	if (error_code == SUCCESS_CODE) {
		processResponseEvent(msg);
	} else {
		processErrorEvent(error_code, msg);
	}
}

void VideoRoomMessageProcessor::processErrorEvent(int errorCode, json &msg)
{
	if (errorCode != SUCCESS_CODE) {
		warn("Unexpected error response:\n%s\n", msg.dump().c_str());
	}
}

void VideoRoomMessageProcessor::processResponseEvent(json &msg)
{
	if (processJoinResponse(msg)) {
		return;
	}
}

bool VideoRoomMessageProcessor::processJoinResponse(json& msg)
{
    if (msg.find("plugindata") == msg.end())
    {
        return false;
    }

    auto plugindata = msg["plugindata"];
    if (plugindata.find("data") == plugindata.end())
    {
        return false;
    }

    auto data = plugindata["data"];

    auto videoroomEvent = data["videoroom"];
    if (videoroomEvent != "joined") {
		return false;
    }

	this->assignMinimumState(VideoRoomState::Joined);
	return true;
}

void VideoRoomMessageProcessor::processUnhandledMessage(string messageType,
						       json &msg)
{
	UNUSED_PARAMETER(msg);

	info("Ignored message of type %s.", messageType.c_str());
}

/*********************** END INBOUND MESSAGE PROCESSING ***********************/

/*********************** MESSAGE CONSTRUCTION ***********************/

bool VideoRoomMessageProcessor::sendKeepAliveMessage()
{
	json keepaliveMsg = {
		{"janus", "keepalive"},
		{"session_id", session_id},
		{"transaction", "keepalive-" + to_string(rand())}
	};

	return sender->sendMessage(keepaliveMsg, "keep-alive");
}

bool VideoRoomMessageProcessor::sendTrickleMessage(
	const string &mid, int index, const string &candidate,
	bool last)
{
	json trickle;
	if (!last) {
		trickle = {{"janus", "trickle"},
			   {"handle_id", handle_id},
			   {"session_id", session_id},
			   {"transaction", "trickle" + to_string(rand())},
			   {"candidate",
			    {{"sdpMid", mid},
			     {"sdpMLineIndex", index},
			     {"candidate", candidate}}}};
	} else {
		trickle = {{"janus", "trickle"},
			   {"handle_id", handle_id},
			   {"session_id", session_id},
			   {"transaction", "trickle" + to_string(rand())},
			   {"candidate", {{"completed", true}}}};
	}

	return sender->sendMessage(trickle, "trickle");
}

bool VideoRoomMessageProcessor::sendOpenMessage(
	const string &sdp, const string &video_codec,
	const string &audio_codec, int video_profile)
{
	// This can get kicked off before the login/join process has completed, so synchronize
	if (!awaitState(VideoRoomState::Joined, 5)) {
		return false;
	}

	json body_no_codec = {
            { "request", "configure" },
            { "muted", false },
            { "video", true },
            { "audio", true }
	};
	json body_with_codec = {
            { "request", "configure" },
            { "videocodec", video_codec },
            { "audiocodec", audio_codec },
            { "vp9_profile", video_profile },
            { "muted", false },
            { "video", true },
            { "audio", true }
	};
	// Send offer
	json open = {
            { "janus", "message" },
            { "session_id", session_id },
            { "handle_id", handle_id },
            { "transaction", to_string(rand()) },
            { "body", (video_codec.empty() && audio_codec.empty()) ? body_no_codec : body_with_codec },
            { "jsep",
                    {
                            { "type", "offer" },
                            { "sdp", sdp },
                            { "trickle", true }
                    }
            }
	};

	return sender->sendMessage(open, "open");
}

bool VideoRoomMessageProcessor::onWebsocketOpened()
{
	// Keep the connection alive
	is_running.store(true);
	// Initialize time to present
	last_message_recd_time = chrono::system_clock::now();
	keepAliveThread = thread(&VideoRoomMessageProcessor::keepConnectionAlive, this);
	return sendLoginMessage();
}

bool VideoRoomMessageProcessor::sendLoginMessage()
{
    // Login command
    json login = {
            { "janus", "create" },
            { "transaction", to_string(rand()) },
    };
    return sender->sendMessage(login, "login");
}

bool VideoRoomMessageProcessor::sendAttachMessage()
{
    // Create handle command
    json attachPlugin = {
        { "janus", "attach" },
        { "plugin", "janus.plugin.videoroom" },
        { "opaque_id", "videoroomtest-" + to_string(rand()) }, // TODO: random string should be 12 length?
        { "transaction", to_string(rand()) },
        { "session_id", session_id }
    };

    return sender->sendMessage(attachPlugin, "attach");
}

bool VideoRoomMessageProcessor::sendJoinMessage(string room)
{
    json joinRoom = {
            { "janus", "message" },
            { "transaction", to_string(rand()) },
            { "session_id", session_id },
            { "handle_id", handle_id },
            { "body",
                    {
                            { "room", stoll(room) },
                            { "display", "OBS" },
                            { "ptype", "publisher" },
                            { "request", "join" }
                    }
            }
    };
    return sender->sendMessage(joinRoom, "join");
}

bool VideoRoomMessageProcessor::sendDestroyMessage()
{
	json destroy = {
		{"janus", "destroy"},
		{"session_id", session_id},
		{"transaction", to_string(rand())}
	};

	return sender->sendMessage(destroy, "destroy");
}

/*********************** END MESSAGE CONSTRUCTION ***********************/

int VideoRoomMessageProcessor::parsePluginErrorCode(json &msg)
{
	if (msg.find("plugindata") == msg.end()) {
		return SUCCESS_CODE;
	}

	auto plugindata = msg["plugindata"];
	if (plugindata.find("data") == plugindata.end()) {
		return SUCCESS_CODE;
	}

	auto data = plugindata["data"];

	if (data.find("error_code") == data.end()) {
		return SUCCESS_CODE;
	}

	int error_code = data["error_code"];
	return error_code;
}

void VideoRoomMessageProcessor::keepConnectionAlive()
{
	while (is_running.load()) {
		// Check how long it's been since we last heard from the server
		if (hasTimedOut()) {
			warn("Connection lost - no messages received");
			is_running.store(false);
			sender->onTimeout();
			break;
		}

		try {
			sendKeepAliveMessage();
		} catch (const websocketpp::exception &e) {
			warn("keepConnectionAlive exception: %s", e.what());
		}
		this_thread::sleep_for(chrono::seconds(2));
	}
}

bool VideoRoomMessageProcessor::hasTimedOut()
{
    std::lock_guard<std::mutex> lockGuard(timeoutMutex);
    auto current_time = chrono::system_clock::now();
    chrono::seconds gap = chrono::duration_cast<std::chrono::seconds>(current_time - last_message_recd_time);
    info("KeepAlive timeout gap.count()=%d", gap.count());
    return gap.count() > MESSAGE_TIMEOUT;
}

bool VideoRoomMessageProcessor::awaitState(VideoRoomState state, int timeoutSeconds)
{
	unique_lock<mutex> lock(stateMutex);
	while (currentState < state) {
		if (!is_running.load()) {
			return false;
		}
		stateListener.wait_for(
			lock,
			chrono::seconds(timeoutSeconds),
			[=] { return !is_running.load() || currentState >= state; });
		lock.unlock();
	}

	return is_running.load();
}

void VideoRoomMessageProcessor::assignMinimumState(VideoRoomState state)
{
	lock_guard<mutex> lock(stateMutex);

	if (currentState < state) {
		currentState = state;
	}

	stateListener.notify_all();
}

void VideoRoomMessageProcessor::afterStreamStarted() {
}

void VideoRoomMessageProcessor::beforeStreamEnded() {
}

