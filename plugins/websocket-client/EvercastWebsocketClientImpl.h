#ifndef __EVERCAST_WEBSOCKET_CLIENT_IMPL_H__
#define __EVERCAST_WEBSOCKET_CLIENT_IMPL_H__

#include "VideoRoomWebsocketClientImpl.h"
#include <EvercastSessionData.h>

class EvercastWebsocketClientImpl : public VideoRoomWebsocketClientImpl {
public:
    ~EvercastWebsocketClientImpl();

protected:
    bool processJoinResponse(nlohmann::json &msg) override;
    void sendLoginMessage(std::string username, std::string token, std::string room) override;
    void sendAttachMessage() override;
    void sendJoinMessage(std::string room) override;
    void sendDestroyMessage() override;
private:
    void defineIceServers(std::vector<IceServerDefinition> &ice_servers);
};

#endif // __EVERCAST_WEBSOCKET_CLIENT_IMPL_H__
