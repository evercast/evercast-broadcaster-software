#include "WebsocketClient.h"

class JanusWebsocketClient : public WebsocketClient {
public:
    JanusWebsocketClient ();
    ~JanusWebsocketClient();

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
};
