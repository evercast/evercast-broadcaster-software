
#include "evercast-utils.hpp"
#include "obs-app.hpp"

#ifndef EVERCAST_DOMAIN
#define EVERCAST_DOMAIN "app.evercast.us"
#endif

#ifndef EVERCAST_CONFIG_KEY_GRAPH_API_URL
#define EVERCAST_CONFIG_KEY_GRAPH_API_URL "evercast_url_graphql"
#endif

#ifndef EVERCAST_CONFIG_KEY_WS_API_URL
#define EVERCAST_CONFIG_KEY_WS_API_URL "evercast_url_websocket"
#endif

EvercastUtils::UrlComponents EvercastUtils::parseUrlComponents(const std::string& url) {

        UrlComponents result;

        auto schemeEndPos = url.find("://");
        if(schemeEndPos != std::string::npos) {
                result.scheme = url.substr(0, schemeEndPos);
                auto pathStartPos = url.find("/", schemeEndPos + 3);
                if(pathStartPos != std::string::npos) {
                        result.domain = url.substr(schemeEndPos + 3, pathStartPos - schemeEndPos - 3);
                        result.path = url.substr(pathStartPos);
                } else {
                        result.domain = url.substr(schemeEndPos + 3);
                }
        } else {
                auto pathStartPos = url.find("/");
                if(pathStartPos != std::string::npos) {
                        result.domain = url.substr(0, pathStartPos);
                        result.path = url.substr(pathStartPos);
                } else {
                        result.domain = url;
                }
        }

        return result;

}

std::string EvercastUtils::parseRoomIdFromUrl(const std::string& url) {
        auto roomsPathPos = url.find("rooms/");
        if(roomsPathPos != std::string::npos) {
                return url.substr(roomsPathPos + 6);
        }
        return "";
}

std::string EvercastUtils::getGraphApiUrlForDomain(const std::string& domain) {
	return "https://" + domain + "/api/graphql";
}

std::string EvercastUtils::getWSApiUrlForDomain(const std::string& domain) {
        return "wss://" + domain + "/websockets";
}

std::string EvercastUtils::getRoomUrlForDomain(const std::string& domain, const std::string& roomId) {
        return "https://" + domain + "/rooms/" + roomId;
}

void EvercastUtils::setDefaultGraphApiUrl(const std::string& url) {
        config_set_default_string(GetGlobalConfig(), "General", EVERCAST_CONFIG_KEY_GRAPH_API_URL, url.c_str());
}

void EvercastUtils::setGraphApiUrl(const std::string& url) {
        config_set_string(GetGlobalConfig(), "General", EVERCAST_CONFIG_KEY_GRAPH_API_URL, url.c_str());
}

std::string EvercastUtils::getGraphApiUrl() {
        return config_get_string(GetGlobalConfig(), "General", EVERCAST_CONFIG_KEY_GRAPH_API_URL);
}

void EvercastUtils::setDefaultWSApiUrl(const std::string& url) {
        config_set_default_string(GetGlobalConfig(), "General", EVERCAST_CONFIG_KEY_WS_API_URL, url.c_str());
}

void EvercastUtils::setWSApiUrl(const std::string& url) {
        config_set_string(GetGlobalConfig(), "General", EVERCAST_CONFIG_KEY_WS_API_URL, url.c_str());
}

std::string EvercastUtils::getWSApiUrl() {
        return config_get_string(GetGlobalConfig(), "General", EVERCAST_CONFIG_KEY_WS_API_URL);
}

std::string EvercastUtils::getRoomUrl(const std::string& roomId) {
	auto urlComponents = parseUrlComponents(getGraphApiUrl());
        return getRoomUrlForDomain(urlComponents.domain, roomId);
}

void EvercastUtils::initDefaults() {
	setDefaultGraphApiUrl(getGraphApiUrlForDomain(EVERCAST_DOMAIN));
        setDefaultWSApiUrl(getWSApiUrlForDomain(EVERCAST_DOMAIN));
}