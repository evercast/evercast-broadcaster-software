#include "VideoRoomMessageProcessor.h"
#include "WebsocketClient.h"
#include "Evercast.h"
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
	const std::string& url,
	const std::string& room,
	const std::string& username,
	const std::string& token,
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
	is_closed.store(false);
}

VideoRoomMessageProcessor::~VideoRoomMessageProcessor() {
	close();
}

void VideoRoomMessageProcessor::close()
{
	// TODO: Make sure calling this multiple times doesn't hurt anything
	is_running.store(false);
	if (keepAliveThread.joinable()) {
		keepAliveThread.join();
	}
	is_closed.store(true);
	stateListener.notify_all();
}

void VideoRoomMessageProcessor::processServerMessage(nlohmann::json &msg)
{
	last_message_recd_time = chrono::system_clock::now();

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

		// Keep the connection alive
		is_running.store(true);

		keepAliveThread = thread(&VideoRoomMessageProcessor::keepConnectionAlive, this);
		assignMinimumState(VideoRoomState::LoggedIn);
	} else { // logged
		handle_id = data["id"];
		sendJoinMessage(room);
		assignMinimumState(VideoRoomState::Attached);
	}
}

void VideoRoomMessageProcessor::processEvent(json &msg)
{
	int error_code = parsePluginErrorCode(msg);

	if (error_code == EVERCAST_SUCCESS) {
		processResponseEvent(msg);
	} else {
		processErrorEvent(error_code, msg);
	}
}

void VideoRoomMessageProcessor::processErrorEvent(int errorCode, json &msg)
{
	if (errorCode == EVERCAST_ERR_DUPLICATE_USER) {
		// Someone is already using that ID, probably a previous version of us.  Log in with a fresh ID.
		logged = false;
		session_id = 0;
		handle_id = 0;

		// Keepalive stuff to prevent doubling-up on keepalive threads
		is_running.store(false);
		if (keepAliveThread.joinable()) {
			keepAliveThread.join();
		}

		sendLoginMessage(username, token, room);

		// Launch logged event
		listener->onLogged(session_id);
		return;
	} else if (errorCode == EVERCAST_ERR_UNSUPPORTED_AUDIO_CODEC) {
		error("Janus room does not support the audio codec specified.");
		// TODO: Try this out
		listener->onLoggedError(-EVERCAST_ERR_UNSUPPORTED_AUDIO_CODEC);
		return;
	} else if (errorCode != EVERCAST_SUCCESS) {
		warn("Unexpected VideoRoom error response:\n%s\n", msg.dump());
		return;
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
    if (videoroomEvent == "joined") {
	    this->assignMinimumState(VideoRoomState::Joined);
    }
}

void VideoRoomMessageProcessor::processUnhandledMessage(string messageType,
						       json &msg)
{
	info("Ignored message of type %s.", messageType);
}

/*********************** END INBOUND MESSAGE PROCESSING ***********************/

/*********************** MESSAGE CONSTRUCTION ***********************/

bool VideoRoomMessageProcessor::sendKeepAliveMessage()
{
	json keepaliveMsg = {
		{"janus", "keepalive"},
		{"session_id", session_id},
		{"transaction", "keepalive-" + std::to_string(rand())}
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
			   {"transaction", "trickle" + std::to_string(rand())},
			   {"candidate",
			    {{"sdpMid", mid},
			     {"sdpMLineIndex", index},
			     {"candidate", candidate}}}};
	} else {
		trickle = {{"janus", "trickle"},
			   {"handle_id", handle_id},
			   {"session_id", session_id},
			   {"transaction", "trickle" + std::to_string(rand())},
			   {"candidate", {{"completed", true}}}};
	}

	return sender->sendMessage(trickle, "trickle");
}

bool VideoRoomMessageProcessor::sendOpenMessage(
	const string &sdp, const string &video_codec,
	const string &audio_codec)
{
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
            { "muted", false },
            { "video", true },
            { "audio", true }
	};
	// Send offer
	json open = {
            { "janus", "message" },
            { "session_id", session_id },
            { "handle_id", handle_id },
            { "transaction", std::to_string(rand()) },
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

bool VideoRoomMessageProcessor::sendLoginMessage(string username, string token, string room)
{
    // Login command
    json login = {
            { "janus", "create" },
            { "transaction", std::to_string(rand()) },
    };
    return sender->sendMessage(login, "login");
}

bool VideoRoomMessageProcessor::sendAttachMessage()
{
    // Create handle command
    json attachPlugin = {
        { "janus", "attach" },
        { "plugin", "janus.plugin.videoroom" },
        { "opaque_id", "videoroomtest-" + std::to_string(rand()) }, // TODO: random string should be 12 length?
        { "transaction", std::to_string(rand()) },
        { "session_id", session_id }
    };

    return sender->sendMessage(attachPlugin, "attach");
}

bool VideoRoomMessageProcessor::sendJoinMessage(string room)
{
    json joinRoom = {
            { "janus", "message" },
            { "transaction", std::to_string(rand()) },
            { "session_id", session_id },
            { "handle_id", handle_id },
            { "body",
                    {
                            { "room", std::stoi(room) },
                            { "display", "EBS" },
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
		{"transaction", std::to_string(rand())}
	};

	return sender->sendMessage(destroy, "destroy");
	// TODO: On completion, set state to closed
}

/*********************** END MESSAGE CONSTRUCTION ***********************/

int VideoRoomMessageProcessor::parsePluginErrorCode(json &msg)
{
	if (msg.find("plugindata") == msg.end()) {
		return EVERCAST_SUCCESS;
	}

	auto plugindata = msg["plugindata"];
	if (plugindata.find("data") == plugindata.end()) {
		return EVERCAST_SUCCESS;
	}

	auto data = plugindata["data"];

	if (data.find("error_code") == data.end()) {
		return EVERCAST_SUCCESS;
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
			listener->onDisconnected();
			break;
		}

		try {
			sendKeepAliveMessage();
		} catch (const websocketpp::exception &e) {
			warn("keepConnectionAlive exception: %s", e.what());
		}
		std::this_thread::sleep_for(std::chrono::seconds(2));
	}
}

bool VideoRoomMessageProcessor::hasTimedOut()
{
    auto current_time = std::chrono::system_clock::now();
    std::chrono::duration<double> gap = current_time - last_message_recd_time;
    return gap.count() > EVERCAST_MESSAGE_TIMEOUT;
}

bool VideoRoomMessageProcessor::awaitState(VideoRoomState state, int timeoutSeconds)
{
	info("Awaiting state %d in state %d...\n", state, currentState);
	auto start_time = std::chrono::system_clock::now();
	auto duration = std::chrono::seconds(timeoutSeconds);

	info("-1\n");
	unique_lock<mutex> lock(stateMutex);
	info("0\n");
	while (currentState < state) {
		info("1\n");
		// auto current_time = std::chrono::system_clock::now();
		// auto remainder = duration - (current_time - start_time);
		if (is_closed.load()/* || remainder < std::chrono::seconds(0)*/) {
			info("2a\n");
			return false;
		}
		info("2b\n");
		info("Waiting for state %d...\n", state);
		stateListener.wait_for(
			lock,
			std::chrono::seconds(timeoutSeconds),
			[=] { return is_closed.load() || currentState >= state; });
		info("Waiting for state %d complete.\n", state);
		lock.unlock();
	}
	info("Awaited state %d and got state %d\n", state, currentState);

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

