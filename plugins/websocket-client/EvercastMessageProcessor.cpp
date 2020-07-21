#include "EvercastMessageProcessor.h"

JanusMessageProcessor * EvercastMessageProcessor::create(
	const std::string& url,
	const std::string& room,
	const std::string& username,
	const std::string& token,
	WebsocketSender* sender,
	WebsocketClient::Listener* listener)
{
	return (JanusMessageProcessor*)new EvercastMessageProcessor(
			url, room, username, token, sender, listener);
}


bool EvercastMessageProcessor::sendLoginMessage(std::string username, std::string token, std::string room)
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

	return sender->sendMessage(login, "login");
}

bool EvercastMessageProcessor::sendAttachMessage()
{
	// Create handle command
	json attachPlugin = {
            { "janus", "attach" },
            { "transaction", std::to_string(rand()) },
            { "session_id", session_id },
            { "plugin", "janus.plugin.lua" }
	};

	return sender->sendMessage(attachPlugin, "attach");
}

bool EvercastMessageProcessor::sendJoinMessage(std::string room)
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

	return sender->sendMessage(joinRoom, "join");
}

bool EvercastMessageProcessor::sendDestroyMessage()
{
	json destroy = {
            { "janus", "destroy" },
            { "session_id", session_id },
            { "transaction", std::to_string(rand()) }
	};

	return sender->sendMessage(destroy, "destroy");
}

void EvercastMessageProcessor::processResponseEvent(json &msg)
{
	VideoRoomMessageProcessor::processResponseEvent(msg);

	// See if the inbound message is plugin data
	if (processPluginData(msg)) {
		return;
	}
}

bool EvercastMessageProcessor::processPluginData(json& msg)
{
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

    if (data.find("videoroom") == data.end()) {
        return false;
    }

    if (processJoinResponse(data)) {
	return true;
    }

    if (processArriveResponse(data)) {
	return true;
    }

    if (processLeaveResponse(data)) {
	return true;
    }

    return false;
}

bool EvercastMessageProcessor::processJoinResponse(json& data)
{
    auto videoroomEvent = data["videoroom"];
    if (videoroomEvent != "joined") {
	    return false;
    }

    parseAttendees(data);
    parseIceServers(data);
}

bool EvercastMessageProcessor::processArriveResponse(json& data)
{
	if (data.find("event") == data.end()) {
		return false;
	}

	auto joiningEvent = data["event"];
	if (joiningEvent != "joining") {
		return false;
	}

	AttendeeIdentifier identifier;
	identifier.display = data["display"].get<std::string>();
	identifier.id = data["id"].get<std::string>();

	EvercastSessionData *session = getSession();
	session->attendeeArrived(identifier);

	return false;
}

bool EvercastMessageProcessor::processLeaveResponse(json& data)
{
	if (data.find("unpublished") == data.end()) {
		return false;
	}

	auto leaving = data["unpublished"];

	EvercastSessionData *session = getSession();
	session->attendeeLeft(leaving);

	return true;
}

void EvercastMessageProcessor::parseAttendees(json& data)
{
    std::vector<AttendeeIdentifier> attendees;
    if (data.find("attendees") == data.end())
    {
	defineAttendees(attendees);
    }

    auto raw = data["attendees"];
    for (auto& element : raw.items()) {
	AttendeeIdentifier identifier;
	auto value = element.value();
	identifier.id = value["id"].get<std::string>();
	identifier.display = value["display"].get<std::string>();
	attendees.push_back(identifier);
    }

    defineAttendees(attendees);
}

void EvercastMessageProcessor::defineAttendees(std::vector<AttendeeIdentifier>& attendees)
{
    // Store attendees in sesion object shared with owner
    EvercastSessionData *session = getSession();
    session->storeAttendees(attendees);
}

void EvercastMessageProcessor::parseIceServers(json &data) {
    // Ice servers
    std::vector<IceServerDefinition> ice_servers;
    if (data.find("iceServers") == data.end())
    {
        defineIceServers(ice_servers);
	return;
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
}

void EvercastMessageProcessor::defineIceServers(std::vector<IceServerDefinition> &ice_servers)
{
    // Fall back to default values.
    if (ice_servers.empty()) {
        IceServerDefinition server;
        server.urls = "stun:stun.l.google.com:19302";
        ice_servers.push_back(server);
    }

    // Store servers in sesion object shared with owner
    EvercastSessionData *session = getSession();
    session->storeIceServers(ice_servers);
}

EvercastSessionData *EvercastMessageProcessor::getSession()
{
	return EvercastSessionData::findOrCreateSession((long long)sender->getId());
}
