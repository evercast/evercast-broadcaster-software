#ifndef __WEBSOCKET_SENDER_H__
#define __WEBSOCKET_SENDER_H__

#include "nlohmann/json.hpp"

using json = nlohmann::json;

class WebsocketSender {
public:
    virtual bool sendMessage(json &msg, const char *name) = 0;
    virtual long long getId() = 0;
};

#endif

