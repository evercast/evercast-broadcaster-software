
#include <util/base.h>

//Use http://think-async.com/ instead of boost
#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_STL_
#define _WEBSOCKETPP_CPP11_THREAD_
#define _WEBSOCKETPP_CPP11_FUNCTIONAL_
#define _WEBSOCKETPP_CPP11_SYSTEM_ERROR_
#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_CPP11_MEMORY_

#include <websocketpp/common/connection_hdl.hpp>
#include "websocketpp/config/asio_client.hpp"
#include "websocketpp/client.hpp"
#include "nlohmann/json.hpp"

#include "WebsocketClient.h"
#include "WebsocketSender.h"
#include "JanusMessageProcessor.h"
#include "ThreadSafeQueue.h"

#include <thread>

typedef websocketpp::client<websocketpp::config::asio_tls_client> Client;

#define warn(format, ...)  blog(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...)  blog(LOG_INFO,    format, ##__VA_ARGS__)
#define debug(format, ...) blog(LOG_DEBUG,   format, ##__VA_ARGS__)
#define error(format, ...) blog(LOG_ERROR,   format, ##__VA_ARGS__)

typedef JanusMessageProcessor * (*messageProcessorFactory)(
            const std::string & url,
            const std::string & room,
            const std::string & username,
            const std::string & token,
	    WebsocketSender *sender,
            WebsocketClient::Listener *listener);

class JanusWebsocketManager : public WebsocketClient, public WebsocketSender {
public:
    JanusWebsocketManager(messageProcessorFactory factory);
    ~JanusWebsocketManager();

    // WebsocketClient implementation
    bool connect(
            const std::string & url,
            const std::string & room,
            const std::string & username,
            const std::string & token,
            WebsocketClient::Listener * listener) override;
    bool open(
            const std::string & sdp,
            const std::string & video_codec,
            const std::string & audio_codec,
            const std::string & /* Id */) override;
    bool trickle(
            const std::string & mid,
            const int index,
            const std::string & candidate,
            const bool last) override;
    bool disconnect(const bool wait) override;

    void destroy();

    // WebsocketSender implementation
    bool sendMessage(json &msg, const char *name) override;
    long long getId() override;

private:
	messageProcessorFactory processorFactory;
	Client client;
	Client::connection_ptr connection;
	std::unique_ptr<JanusMessageProcessor> messageProcessor;
	ThreadSafeQueue<json> messageQueue;
	std::atomic<bool> is_running;

	std::thread processingThread;
	std::thread ioThread;

	void initializeMessageProcessor(
            const std::string & url,
            const std::string & room,
            const std::string & username,
            const std::string & token,
            WebsocketClient::Listener * listener);

	void processWorker();
	std::string sanitizeString(const std::string & s);

	void handleDisconnect(websocketpp::connection_hdl connectionHdl,
			      WebsocketClient::Listener *listener);

	void handleFail(websocketpp::connection_hdl connectionHdl,
			WebsocketClient::Listener *listener);
};

