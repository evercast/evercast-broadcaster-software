#include "JanusWebsocketManager.h"
#include "EvercastMessageProcessor.h"

typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

JanusWebsocketManager::JanusWebsocketManager(messageProcessorFactory factory)
{
    this->processorFactory = factory;
    // Set logging to be pretty verbose (everything except message payloads)
    client.set_access_channels(websocketpp::log::alevel::all);
    client.clear_access_channels(websocketpp::log::alevel::frame_payload);
    client.set_error_channels(websocketpp::log::elevel::all);
    // Initialize ASIO
    client.init_asio();
}

JanusWebsocketManager::~JanusWebsocketManager()
{
	is_running.store(false);
	messageQueue.close();
	if (std::this_thread::get_id() != processingThread.get_id() && processingThread.joinable())
		processingThread.join();
	if (std::this_thread::get_id() != ioThread.get_id() && ioThread.joinable())
		ioThread.join();
}

// WebsocketClient implementation
bool JanusWebsocketManager::connect(const std::string& raw_url, const std::string& raw_room,
	const std::string& raw_username, const std::string& raw_token,
	WebsocketClient::Listener *listener)
{
    using namespace std::placeholders;

    websocketpp::lib::error_code ec;
    std::string url = sanitizeString(raw_url);
    std::string room = sanitizeString(raw_room);
    std::string username = sanitizeString(raw_username);
    std::string token = sanitizeString(raw_token);

    initializeMessageProcessor(url, room, username, token, listener);

    // Reset login flag
    try {
        // --- Message handler
        client.set_message_handler([=](
                websocketpp::connection_hdl /* con */, message_ptr frame) {
            const char* x = frame->get_payload().c_str();
            info("MESSAGE RECEIVED:\n%s\n", x);

            json msg = json::parse(frame->get_payload());

	    // Queue up the message for asynchronous processing
	    this->messageQueue.push(msg);
        });

        // --- Open handler
        client.set_open_handler([=](websocketpp::connection_hdl /* con */) {
            // Launch event
            listener->onConnected();
            messageProcessor->onWebsocketOpened();
        });

        // --- Close handler
        client.set_close_handler(std::bind(&JanusWebsocketManager::handleDisconnect, this, _1, listener));

        // --- Failure handler
        client.set_fail_handler(std::bind(&JanusWebsocketManager::handleFail, this, _1, listener));

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
        ioThread = std::thread([&]() {
		try
		{
		    // Start ASIO io_service run loop
		    // (single connection will be made to the server)
		    client.run();  // will exit when this connection is closed
		}
		catch (const std::exception& ex)
		{
		    error("Error during websocket execution: %s\n", ex.what());
		}
        });
    } catch (websocketpp::exception const & e) {
        warn("connect exception: %s", e.what());
        return false;
    }
    // OK
    return true;
}

bool JanusWebsocketManager::open(const std::string &sdp, const std::string &video_codec,
	  const std::string &audio_codec, int video_profile,
	  const std::string & /* Id */)
{
    bool result;
    try {
		result = this->messageProcessor->sendOpenMessage(sdp, video_codec, audio_codec, video_profile);
    } catch (const websocketpp::exception & e) {
        warn("open exception: %s", e.what());
    }
    return result;
}

bool JanusWebsocketManager::trickle(const std::string &mid, const int index,
	     const std::string &candidate, const bool last)
{
    bool result = false;
    try {
	result = this->messageProcessor->sendTrickleMessage(mid, index, candidate, last);
    } catch (const websocketpp::exception & e) {
        warn("trickle exception: %s", e.what());
    }
    return result;
}

bool JanusWebsocketManager::disconnect(const bool wait)
{
	bool result = true;

	is_running.store(false);
	this->messageProcessor->sendDestroyMessage();
	this->messageProcessor->close();

	if (!connection)
		return true;

	websocketpp::lib::error_code ec;
	try {
		// Stop client
		if (connection->get_state() ==
		    websocketpp::session::state::open)
			client.close(connection,
				     websocketpp::close::status::normal,
				     std::string("disconnect"), ec);
		if (ec)
			warn("> Error on disconnect close: %s",
			     ec.message().c_str());
		// Don't wait for connection close
		client.stop();
		// Stop the message queue
                messageQueue.close();
		// Remove handlers
		client.set_open_handler([](...) {});
		client.set_close_handler([](...) {});
		client.set_fail_handler([](...) {});
		client.set_message_handler([](...) {});

		if (wait && processingThread.joinable()) {
			processingThread.join();
		}

		if (wait && ioThread.joinable()) {
			ioThread.join();
		}
	} catch (const websocketpp::exception &e) {
		warn("disconnect exception: %s", e.what());
		result = false;
	}

	// OK
	return result;
}

void JanusWebsocketManager::destroy()
{
	this->messageProcessor->sendDestroyMessage();
}

void JanusWebsocketManager::initializeMessageProcessor(
	const std::string& url,
	const std::string& room,
	const std::string& username,
	const std::string& token,
	WebsocketClient::Listener* listener)
{
	messageProcessor.reset(processorFactory(url, room, username, token, this, listener));

	is_running.store(true);
	processingThread = std::thread(&JanusWebsocketManager::processWorker, this);
}

void JanusWebsocketManager::processWorker()
{
	while (is_running.load()) {
		try
		{
			json message;
			if (messageQueue.awaitItem(&message))
			{
				messageProcessor->processServerMessage(message);
			}
		}
		catch (const std::exception& ex)
		{
			error("Error during message processing: %s\n", ex.what());
		}
	}
}

std::string JanusWebsocketManager::sanitizeString(const std::string &s)
{
	std::string _my_s = s;
	size_t p = _my_s.find_first_not_of(" \n\r\t");
	_my_s.erase(0, p);
	p = _my_s.find_last_not_of(" \n\r\t");
	if (p != std::string::npos)
		_my_s.erase(p + 1);
	return _my_s;
}

void JanusWebsocketManager::handleDisconnect(
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

void JanusWebsocketManager::handleFail(
        websocketpp::connection_hdl connectionHdl,
        WebsocketClient::Listener * listener)
{
    UNUSED_PARAMETER(connectionHdl);

    info("> set_fail_handler called");
    if (listener)
    {
	listener->onLoggedError(-1);
    }
}

bool JanusWebsocketManager::sendMessage(json &msg, const char* name)
{
	// Don't send if socket is shut down
	if (!connection || connection->get_state() != websocketpp::session::state::value::open) {
		return false;
	}

	info("Sending %s message...", name);
	debug("MESSAGE: %s\n", msg.dump().c_str());

	// Serialize and send
	if (connection->send(msg.dump())) {
		warn("Error sending %s message", name);
		return false;
	}

	return true;
}

void JanusWebsocketManager::onTimeout()
{
	this->disconnect(false);
}

long long JanusWebsocketManager::getId()
{
	return (long long)this;
}
