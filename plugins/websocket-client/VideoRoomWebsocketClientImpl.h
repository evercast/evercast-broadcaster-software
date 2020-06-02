#include "EvercastWebsocketClientImpl.h"

class VideoRoomWebsocketClientImpl : public EvercastWebsocketClientImpl {
protected:
    void sendLoginMessage(std::string username, std::string token, std::string room) override;
    void sendAttachMessage() override;
    void sendJoinMessage(std::string room) override;
    void sendDestroyMessage() override;
};


