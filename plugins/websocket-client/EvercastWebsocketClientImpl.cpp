#include "EvercastWebsocketClientImpl.h"
#include "EvercastSessionData.h"

#include <iostream>
#include <string>

using json = nlohmann::json;

EvercastWebsocketClientImpl::~EvercastWebsocketClientImpl()
{
    EvercastSessionData::terminateSession((long long)this);
}

/*********************** EVERCAST CUSTOM MESSAGING ***********************/

bool EvercastWebsocketClientImpl::processJoinResponse(json& msg)
{
    bool result = VideoRoomWebsocketClientImpl::processJoinResponse(msg);

    if (!result) {
        return false;
    }

    // TODO: Get attendees, etc. as well
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
    std::vector<IceServerDefinition> ice_servers;
    if (data.find("iceServers") == data.end())
    {
        defineIceServers(ice_servers);
        return false;
    }

    // Parse servers from JSON
    auto servers = data["iceServers"];
    for (auto& element : servers.items()) {
        IceServerDefinition serv;
        auto value = element.value();
        serv.urls = value["urls"].get<std::string>();
        if (!value.at("username").is_null())
            serv.username = value["username"].get<std::string>();
        if (!value.at("credential").is_null())
            serv.password = value["credential"].get<std::string>();
        ice_servers.push_back(serv);
    }

    defineIceServers(ice_servers);

    return true;
}

/*********************** SPECIALIZED EVERCAST MESSAGES CONSTRUCTION AND TRANSMISSION ***********************/

void EvercastWebsocketClientImpl::sendLoginMessage(std::string username, std::string token, std::string room)
{
    // Login command
    json login = {
            { "janus", "create" },
            { "transaction", std::to_string(rand()) },
            { "payload",
                    {
                            { "username", username },
                            { "token", token },
                            { "room", room }
                    }
            }
    };
    sendMessage(login, "login");
}

void EvercastWebsocketClientImpl::sendAttachMessage()
{
    // Create handle command
    json attachPlugin = {
            { "janus", "attach" },
            { "transaction", std::to_string(rand()) },
            { "session_id", session_id },
            { "plugin", "janus.plugin.lua" }
    };
    sendMessage(attachPlugin, "attach");
}

void EvercastWebsocketClientImpl::sendJoinMessage(std::string room)
{
    json joinRoom = {
            { "janus", "message" },
            { "transaction", std::to_string(rand()) },
            { "session_id", session_id },
            { "handle_id", handle_id },
            { "body",
                    {
                            { "room", room },
                            { "display", "EBS" },
                            { "ptype", "publisher" },
                            { "request", "join" }
                    }
            }
    };
    sendMessage(joinRoom, "join");
}

void EvercastWebsocketClientImpl::sendDestroyMessage()
{
    json destroyMsg = {
            { "janus", "destroy" },
            { "session_id", session_id },
            { "transaction", std::to_string(rand()) }
    };
    sendMessage(destroyMsg, "destroy");
}

/*********************** UTILITY FUNCTIONS ***********************/

void EvercastWebsocketClientImpl::defineIceServers(std::vector<IceServerDefinition> &ice_servers)
{
    // Fall back to default values.
    if (ice_servers.empty()) {
        IceServerDefinition server;
        server.urls = "stun:stun.l.google.com:19302";
        ice_servers.push_back(server);
    }

    // Store servers in sesion object shared with owner
    EvercastSessionData *session = EvercastSessionData::findOrCreateSession((long long)this);
    session->storeIceServers(ice_servers);
}
