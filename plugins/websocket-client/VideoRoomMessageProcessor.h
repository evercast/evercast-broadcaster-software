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

#define MESSAGE_TIMEOUT 5
#define SUCCESS_CODE 0

using namespace std;

class VideoRoomMessageProcessor : public JanusMessageProcessor {
public:
	static JanusMessageProcessor * create(
		const string &url,
		const string &room,
		const string &username,
		const string &token,
		WebsocketSender *sender,
		WebsocketClient::Listener* listener);

	VideoRoomMessageProcessor(
		const string& url,
		const string& room,
		const string& username,
		const string& token,
		WebsocketSender *sender,
		WebsocketClient::Listener* listener);

	~VideoRoomMessageProcessor();

	void processServerMessage(json &msg) override;
	bool sendKeepAliveMessage() override;
	bool sendTrickleMessage(const string &mid, int index, const string &candidate, bool last) override;
	bool sendOpenMessage(const string &sdp, const string &video_codec, const string &audio_codec, int video_profile) override;
	bool onWebsocketOpened() override;
	bool sendLoginMessage() override;
	bool sendAttachMessage() override;
	bool sendJoinMessage(string room) override;
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
	string url;
	string room;
	string username;
	string token;

	virtual void processSuccessMessage(json &msg);
	virtual void processUnhandledMessage(string messageType, json &msg);
	void processEvent(json &msg);
	virtual void processErrorEvent(int errorCode, json &msg);
	virtual void processResponseEvent(json &msg);
	bool awaitState(VideoRoomState state, int timeoutSeconds);
	void assignMinimumState(VideoRoomState state);

	virtual void afterStreamStarted();
	virtual void beforeStreamEnded();

private:
	atomic<bool> is_running;
	thread keepAliveThread;
	chrono::time_point<chrono::system_clock> last_message_recd_time;
	mutex timeoutMutex;
	mutex stateMutex;
	VideoRoomState currentState;
	condition_variable stateListener;

	bool processJoinResponse(json& msg);
	void keepConnectionAlive();
	bool hasTimedOut();
	int parsePluginErrorCode(json &msg);
};

#endif

