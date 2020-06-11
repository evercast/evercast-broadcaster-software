#ifndef __EVERCAST_WEBSOCKET_CLIENT_IMPL_H__
#define __EVERCAST_WEBSOCKET_CLIENT_IMPL_H__

#include "WebsocketClient.h"
#include "EvercastSessionData.h"

//Use http://think-async.com/ instead of boost
#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_STL_
#define _WEBSOCKETPP_CPP11_THREAD_
#define _WEBSOCKETPP_CPP11_FUNCTIONAL_
#define _WEBSOCKETPP_CPP11_SYSTEM_ERROR_
#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_CPP11_MEMORY_
#define EVERCAST_MESSAGE_TIMEOUT 5.0

#include <websocketpp/common/connection_hdl.hpp>
#include "websocketpp/config/asio_client.hpp"
#include "websocketpp/client.hpp"
#include "nlohmann/json.hpp"

typedef websocketpp::client<websocketpp::config::asio_tls_client> Client;

class EvercastWebsocketClientImpl : public WebsocketClient {
public:
    EvercastWebsocketClientImpl();
    ~EvercastWebsocketClientImpl();

    // WebsocketClient::Listener implementation
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

    void keepConnectionAlive(WebsocketClient::Listener * listener);
    void destroy();

protected:
    long long session_id;
    long long handle_id;

    virtual void sendKeepAliveMessage();
    virtual bool sendTrickleMessage(const std::string &mid, int index, const std::string &candidate, bool last);
    virtual bool sendOpenMessage(const std::string &sdp, const std::string &video_codec, const std::string &audio_codec);
    virtual void sendLoginMessage(std::string username, std::string token, std::string room);
    virtual void sendAttachMessage();
    virtual void sendJoinMessage(std::string room);
    virtual void sendDestroyMessage();
    virtual bool sendMessage(nlohmann::json msg, const char *name);

 private:
    bool logged;

    Client client;
    Client::connection_ptr connection;
    std::thread thread;
    std::thread thread_keepAlive;
    std::atomic<bool> is_running;
    std::chrono::time_point<std::chrono::system_clock> last_message_recd_time;

    std::string sanitizeString(const std::string & s);
    void handleDisconnect(websocketpp::connection_hdl connectionHdl,
                          WebsocketClient::Listener * listener);
    void handleFail(websocketpp::connection_hdl connectionHdl,
                          WebsocketClient::Listener * listener);
    int parsePluginErrorCode(nlohmann::json &msg);
    bool hasTimedOut();
    bool processJoinResponse(nlohmann::json &msg);
    void defineIceServers(std::vector<IceServerDefinition> &ice_servers);
};

#endif // __EVERCAST_WEBSOCKET_CLIENT_IMPL_H__
