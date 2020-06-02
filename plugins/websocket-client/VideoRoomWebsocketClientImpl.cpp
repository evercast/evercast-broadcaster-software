#include "VideoRoomWebsocketClientImpl.h"
#include "nlohmann/json.hpp"
#include "Evercast.h"

using json = nlohmann::json;

/*********************** MESSAGE CONSTRUCTION AND TRANSMISSION ***********************/

void VideoRoomWebsocketClientImpl::sendLoginMessage(std::string username, std::string token, std::string room)
{
    // Login command
    json login = {
            { "janus", "create" },
            { "transaction", std::to_string(rand()) },
    };
    sendMessage(login, "login");
}

void VideoRoomWebsocketClientImpl::sendAttachMessage()
{
    // Create handle command
    json attachPlugin = {
        { "janus", "attach" },
        { "plugin", "janus.plugin.videoroom" },
        { "opaque_id", "videoroomtest-" + std::to_string(rand()) }, // TODO: random string should be 12 length?
        { "transaction", std::to_string(rand()) },
        { "session_id", session_id }
    };

    sendMessage(attachPlugin, "attach");
}

void VideoRoomWebsocketClientImpl::sendJoinMessage(std::string room)
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
    sendMessage(joinRoom, "join");
}

void VideoRoomWebsocketClientImpl::sendDestroyMessage()
{
    json destroyMsg = {
            { "janus", "destroy" },
            { "session_id", session_id },
            { "transaction", std::to_string(rand()) }
    };
    sendMessage(destroyMsg, "destroy");
}
