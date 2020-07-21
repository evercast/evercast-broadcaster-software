#ifndef __VIDEO_ROOM_MESSAGE_PROCESSOR_H__
#define __VIDEO_ROOM_MESSAGE_PROCESSOR_H__

#include "JanusMessageProcessor.h"
#include "WebsocketClient.h"
#include "WebsocketSender.h"
#include "nlohmann/json.hpp"

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

#define EVERCAST_MESSAGE_TIMEOUT 5.0

class VideoRoomMessageProcessor : public JanusMessageProcessor {
public:
	static JanusMessageProcessor * create(
		const std::string &url,
		const std::string &room,
		const std::string &username,
		const std::string &token,
		WebsocketSender *sender,
		WebsocketClient::Listener* listener);

	VideoRoomMessageProcessor(
		const std::string& url,
		const std::string& room,
		const std::string& username,
		const std::string& token,
		WebsocketSender *sender,
		WebsocketClient::Listener* listener);

	~VideoRoomMessageProcessor();

	void processServerMessage(json &msg) override;
	bool sendKeepAliveMessage() override;
	bool sendTrickleMessage(const std::string &mid, int index, const std::string &candidate, bool last) override;
	bool sendOpenMessage(const std::string &sdp, const std::string &video_codec, const std::string &audio_codec) override;
	bool sendLoginMessage(std::string username, std::string token, std::string room) override;
	bool sendAttachMessage() override;
	bool sendJoinMessage(std::string room) override;
	bool sendDestroyMessage() override;
	void close() override;

protected:
	enum VideoRoomState {
		Created = 0,
		LoggedIn,
		Attached,
		Joined,
		Open,
		Closed
	};

	bool logged = false;
	long long session_id;
	long long handle_id;
	WebsocketSender *sender;
	WebsocketClient::Listener *listener;

	virtual void processSuccessMessage(json &msg);
	virtual void processUnhandledMessage(std::string messageType, json &msg);
	void processEvent(json &msg);
	virtual void processErrorEvent(int errorCode, json &msg);
	virtual void processResponseEvent(json &msg);
	bool awaitState(VideoRoomState state, int timeoutSeconds);
	void assignMinimumState(VideoRoomState state);

private:
	std::string url;
	std::string room;
	std::string username;
	std::string token;
	std::atomic<bool> is_running;
	std::atomic<bool> is_closed;
	std::thread keepAliveThread;
	std::chrono::time_point<std::chrono::system_clock> last_message_recd_time;
	std::mutex stateMutex;
	VideoRoomState currentState;
	std::condition_variable stateListener;

	bool processJoinResponse(json& msg);
	void keepConnectionAlive();
	bool hasTimedOut();
	int parsePluginErrorCode(json &msg);
};

#endif

