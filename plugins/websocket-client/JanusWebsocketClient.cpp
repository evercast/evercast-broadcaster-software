#include "JanusWebsocketClient.h"

using json = nlohmann::json;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

JanusWebsocketClient::JanusWebsocketClient()
{
    // Set logging to be pretty verbose (everything except message payloads)
    client.set_access_channels(websocketpp::log::alevel::all);
    client.clear_access_channels(websocketpp::log::alevel::frame_payload);
    client.set_error_channels(websocketpp::log::elevel::all);
    // Initialize ASIO
    client.init_asio();
}

JanusWebsocketClient::~JanusWebsocketClient()
{
}

bool JanusWebsocketClient::connect(
	const std::string& url,
	const std::string& room,
	const std::string& username,
	const std::string& token,
	WebsocketClient::Listener* listener)
{
}

bool JanusWebsocketClient::open(
    const std::string & sdp,
    const std::string & video_codec,
    const std::string & audio_codec,
    const std::string & /* Id */)
{
}

bool JanusWebsocketClient::trickle(
    const std::string & mid,
    const int index,
    const std::string & candidate,
    const bool last)
{
}

bool JanusWebsocketClient::disconnect(const bool wait)
{
}


