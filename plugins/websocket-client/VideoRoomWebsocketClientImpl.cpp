#include "VideoRoomWebsocketClientImpl.h"
#include "nlohmann/json.hpp"
#include "Evercast.h"
#include <util/base.h>

#define warn(format, ...)  blog(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...)  blog(LOG_INFO,    format, ##__VA_ARGS__)
#define debug(format, ...) blog(LOG_DEBUG,   format, ##__VA_ARGS__)
#define error(format, ...) blog(LOG_ERROR,   format, ##__VA_ARGS__)

using json = nlohmann::json;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

VideoRoomWebsocketClientImpl::VideoRoomWebsocketClientImpl()
{
    // Set logging to be pretty verbose (everything except message payloads)
    client.set_access_channels(websocketpp::log::alevel::all);
    client.clear_access_channels(websocketpp::log::alevel::frame_payload);
    client.set_error_channels(websocketpp::log::elevel::all);
    // Initialize ASIO
    client.init_asio();
}

VideoRoomWebsocketClientImpl::~VideoRoomWebsocketClientImpl()
{
    // Disconnect just in case
    disconnect(false);
}

bool VideoRoomWebsocketClientImpl::connect(
        const std::string & raw_url,
        const std::string & raw_room,
        const std::string & raw_username,
        const std::string & raw_token,
        WebsocketClient::Listener * listener)
{
    using namespace std::placeholders;

    websocketpp::lib::error_code ec;
    std::string url = sanitizeString(raw_url);
    std::string room = sanitizeString(raw_room);
    std::string username = sanitizeString(raw_username);
    std::string token = sanitizeString(raw_token);

    // Reset login flag
    logged = false;
    try {
        // --- Message handler
        client.set_message_handler([=](
                websocketpp::connection_hdl /* con */, message_ptr frame) {
            const char* x = frame->get_payload().c_str();
            info("MESSAGE RECEIVED:\n%s\n", x);

            last_message_recd_time = std::chrono::system_clock::now();

            json msg = json::parse(frame->get_payload());

            if (msg.find("janus") == msg.end())
                return;

            // Check if it is an event
            if (msg.find("jsep") != msg.end()) {
                std::string sdp = msg["jsep"]["sdp"];
                // Event
                listener->onOpened(sdp);
                return;
            }

            // Check type
            std::string event = msg["janus"];

            if (event == "ack")
                return;

            // Success response handling
            if (event.compare("success") == 0) {
                if (msg.find("transaction") == msg.end())
                    return;
                if (msg.find("data") == msg.end())
                    return;
                // Get the data session
                auto data = msg["data"];
                // Server is sending response twice, ignore second one
                if (!logged) {
                    // Get response code
                    session_id = data["id"];
                    sendAttachMessage();
                    logged = true;

                    // Keep the connection alive
                    is_running.store(true);

                    thread_keepAlive = std::thread([&]() {
                        keepConnectionAlive(listener);
                    });
                } else { // logged
                    handle_id = data["id"];
                    sendJoinMessage(room);
                }
            }

            // Error response handling
            if (event == "event")
            {
                int error_code = parsePluginErrorCode(msg);

                if (error_code == EVERCAST_ERR_DUPLICATE_USER)
                {
                    // Someone is already using that ID, probably a previous version of us.  Log in with a fresh ID.
                    logged = false;
                    session_id = 0;
                    handle_id = 0;
                    sendLoginMessage(username, token, room);

                    // Keepalive stuff to prevent doubling-up on keepalive threads
                    is_running.store(false);
                    if (thread_keepAlive.joinable())
                    {
                        thread_keepAlive.join();
                    }

                    // Launch logged event
                    listener->onLogged(session_id);
                    return;
                }
                else if (error_code == EVERCAST_ERR_UNSUPPORTED_AUDIO_CODEC)
                {
                    error("Janus room does not support the audio codec specified.");
                    // TODO: Disconnect - needs proper thread management
                    // listener->onLoggedError(-EVERCAST_ERR_UNSUPPORTED_AUDIO_CODEC);
                    return;
                }
                else if (error_code != EVERCAST_SUCCESS)
                {
                    warn("Unexpected Evercast error response:\n%s\n", x);
                    return;
                }

                if (processJoinResponse(msg)) {
                    return;
                }
            }

        });

        // --- Open handler
        client.set_open_handler([=](websocketpp::connection_hdl /* con */) {
            // Launch event
            listener->onConnected();
            sendLoginMessage(username, token, room);
        });

        // --- Close handler
        client.set_close_handler(std::bind(&VideoRoomWebsocketClientImpl::handleDisconnect, this, _1, listener));

        // --- Failure handler
        client.set_fail_handler(std::bind(&VideoRoomWebsocketClientImpl::handleFail, this, _1, listener));

        // --- TLS handler
        client.set_tls_init_handler([&](websocketpp::connection_hdl /* con */) {
            // Create context
            auto ctx = websocketpp::lib::make_shared<asio::ssl::context>(
                    asio::ssl::context::tlsv12_client);
            try {
                // Remove support for undesired TLS versions
                ctx->set_options(
                        asio::ssl::context::default_workarounds |
                        asio::ssl::context::no_sslv2 |
                        asio::ssl::context::single_dh_use);
            } catch (std::exception & e) {
                warn("TLS exception: %s", e.what());
            }
            return ctx;
        });

        // Create websocket url
        std::string wss = url + "/?token=" + token + "&roomId=" + room;
        info("Connection URL:   %s", wss.c_str());

        connection = client.get_connection(wss, ec);
        if (ec) {
            error("Error establishing websocket connection: %s", ec.message().c_str());
            return 0;
        }
        connection->add_subprotocol("janus-protocol");

        // Note that connect here only requests a connection. No network messages
        // exchanged until the event loop starts running in the next line.
        client.connect(connection);
        // Async
        thread = std::thread([&]() {
            // Start ASIO io_service run loop
            // (single connection will be made to the server)
            client.run();  // will exit when this connection is closed
        });
    } catch (websocketpp::exception const & e) {
        warn("connect exception: %s", e.what());
        return false;
    }
    // OK
    return true;
}

bool VideoRoomWebsocketClientImpl::open(
        const std::string & sdp,
        const std::string & video_codec,
        const std::string & audio_codec,
        const std::string & /* Id */)
{
    bool result;
    try {
        result = sendOpenMessage(sdp, video_codec, audio_codec);
    } catch (const websocketpp::exception & e) {
        warn("open exception: %s", e.what());
    }
    return result;
}

bool VideoRoomWebsocketClientImpl::trickle(
        const std::string & mid,
        int index,
        const std::string & candidate,
        bool last)
{
    bool result = false;
    try {
        result = sendTrickleMessage(mid, index, candidate, last);
    } catch (const websocketpp::exception & e) {
        warn("trickle exception: %s", e.what());
    }
    return result;
}

void VideoRoomWebsocketClientImpl::keepConnectionAlive(WebsocketClient::Listener * listener)
{
    while (is_running.load()) {
        if (connection) {
            // Check how long it's been since we last heard from the server
            if (hasTimedOut()) {
                warn("Connection lost - no messages received");
                listener->onDisconnected();

                break;
            }

            try {
                sendKeepAliveMessage();
            } catch (const websocketpp::exception &e) {
                warn("keepConnectionAlive exception: %s", e.what());
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

void VideoRoomWebsocketClientImpl::destroy()
{
    if (connection) {
        try {
            sendDestroyMessage();
        } catch (const websocketpp::exception & e) {
            warn("destroy exception: %s", e.what());
        }
    }
}

bool VideoRoomWebsocketClientImpl::disconnect(bool wait)
{
    destroy();
    if (!connection)
        return true;
    websocketpp::lib::error_code ec;
    try {
        // Stop keepAlive
        if (thread_keepAlive.joinable()) {
            is_running.store(false);
            thread_keepAlive.join();
        }
        // Stop client
        if (connection->get_state() == websocketpp::session::state::open)
            client.close(connection, websocketpp::close::status::normal, std::string("disconnect"), ec);
        if (ec)
            warn("> Error on disconnect close: %s", ec.message().c_str());
        // Don't wait for connection close
        client.stop();
        if (wait && thread.joinable()) {
            thread.join();
        } else {
            // Remove handlers
            client.set_open_handler([](...) {});
            client.set_close_handler([](...) {});
            client.set_fail_handler([](...) {});
            if (thread.joinable())
                thread.detach();
        }
    } catch (const websocketpp::exception & e) {
        warn("disconnect exception: %s", e.what());
        return false;
    }
    // OK
    return true;
}

std::string VideoRoomWebsocketClientImpl::sanitizeString(const std::string & s)
{
    std::string _my_s = s;
    size_t p = _my_s.find_first_not_of(" \n\r\t");
    _my_s.erase(0, p);
    p = _my_s.find_last_not_of(" \n\r\t");
    if (p != std::string::npos)
        _my_s.erase(p + 1);
    return _my_s;
}

void VideoRoomWebsocketClientImpl::handleDisconnect(
        websocketpp::connection_hdl connectionHdl,
        WebsocketClient::Listener * listener)
{
    UNUSED_PARAMETER(connectionHdl);

    info("> set_close_handler called");
    if (listener)
    {
        listener->onDisconnected();
    }
}

void VideoRoomWebsocketClientImpl::handleFail(
        websocketpp::connection_hdl connectionHdl,
        WebsocketClient::Listener * listener)
{
    UNUSED_PARAMETER(connectionHdl);

    info("> set_fail_handler called");
    if (listener)
    {
        if (hasTimedOut()) {
            warn("Connection lost - no messages received");
            listener->onDisconnected();
        } else {
            listener->onLoggedError(-1);
        }
    }
}

/*********************** MESSAGE CONSTRUCTION AND TRANSMISSION ***********************/

void VideoRoomWebsocketClientImpl::sendKeepAliveMessage()
{
    json keepaliveMsg = {
            {"janus",       "keepalive"},
            {"session_id",  session_id},
            {"transaction", "keepalive-" + std::to_string(rand())}
    };
    sendMessage(keepaliveMsg, "keep-alive");
}

bool VideoRoomWebsocketClientImpl::sendTrickleMessage(const std::string &mid, int index, const std::string &candidate, bool last)
{
    json trickle;
    if (!last) {
        trickle = {
                { "janus", "trickle" },
                { "handle_id", handle_id },
                { "session_id", session_id },
                { "transaction", "trickle" + std::to_string(rand()) },
                { "candidate",
                        {
                                { "sdpMid", mid },
                                { "sdpMLineIndex", index },
                                { "candidate", candidate }
                        }
                }
        };
    }
    else
    {
        trickle = {
                { "janus", "trickle" },
                { "handle_id", handle_id },
                { "session_id", session_id },
                { "transaction", "trickle" + std::to_string(rand()) },
                { "candidate",
                        {
                                { "completed", true }
                        }
                }
        };
    }

    return sendMessage(trickle, "trickle");
}

bool VideoRoomWebsocketClientImpl::sendOpenMessage(const std::string &sdp, const std::string &video_codec, const std::string &audio_codec)
{
    json body_no_codec = {
            { "request", "configure" },
            { "muted", false },
            { "video", true },
            { "audio", true }
    };
    json body_with_codec = {
            { "request", "configure" },
            { "videocodec", video_codec },
            { "audiocodec", audio_codec },
            { "muted", false },
            { "video", true },
            { "audio", true }
    };
    // Send offer
    json open = {
            { "janus", "message" },
            { "session_id", session_id },
            { "handle_id", handle_id },
            { "transaction", std::to_string(rand()) },
            { "body", (video_codec.empty() && audio_codec.empty()) ? body_no_codec : body_with_codec },
            { "jsep",
                    {
                            { "type", "offer" },
                            { "sdp", sdp },
                            { "trickle", true }
                    }
            }
    };

    return sendMessage(open, "open");
}

bool VideoRoomWebsocketClientImpl::sendMessage(json msg, const char *name)
{
    info("Sending %s message...", name);

    // Serialize and send
    if (connection->send(msg.dump()))
    {
        warn("Error sending %s message", name);
        return false;
    }

    return true;
}


int VideoRoomWebsocketClientImpl::parsePluginErrorCode(json &msg)
{
    if (msg.find("plugindata") == msg.end())
    {
        return 0;
    }

    auto plugindata = msg["plugindata"];
    if (plugindata.find("data") == plugindata.end())
    {
        return 0;
    }

    auto data = plugindata["data"];

    if (data.find("error_code") == data.end())
    {
        return 0;
    }

    int error_code = data["error_code"];
    return error_code;
}

bool VideoRoomWebsocketClientImpl::processJoinResponse(json &msg)
{
    return true;
}

bool VideoRoomWebsocketClientImpl::hasTimedOut()
{
    auto current_time = std::chrono::system_clock::now();
    std::chrono::duration<double> gap = current_time - last_message_recd_time;
    return gap.count() > EVERCAST_MESSAGE_TIMEOUT;
}

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

/*********************** END MESSAGE CONSTRUCTION AND TRANSMISSION ***********************/
