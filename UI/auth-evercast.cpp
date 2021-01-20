
#include "auth-evercast.hpp"

#include "obs-app.hpp"

EvercastAuth::BaseUrlAndPath EvercastAuth::parseUrlComponents(const std::string& url) {

	BaseUrlAndPath result;

        int pos = -1;
        for(unsigned int i = 1; i < url.length(); i++) {
                if(url[i] == '/' && url[i - 1] != ':' && url[i - 1] != '/') {
                        pos = i;
                        break;
                }
        }

        if(pos == -1) {
                result.baseUrl = url;
                result.path = "/";
        } else {
                result.baseUrl = url.substr(0, pos);
                result.path = url.substr(pos, url.length());
        }

	return result;
}

void EvercastAuth::skipChar(const std::string& text, int& pos, char c) {
        while(pos < text.length() && text[pos] == c) {
                pos ++;
        }
}

bool EvercastAuth::findChar(const std::string& text, int& pos, char c) {
        while(pos < text.length() && text[pos] != c) {
                pos ++;
        }
        return pos < text.length() && text[pos] == c;
}

std::string EvercastAuth::parseValue(const std::string& text,  const std::string& key) {

        int pos = 0;

        int keyPos = -1;
        int valuePos = -1;

        std::string value = "";

        while(pos < text.length()) {
                if(keyPos == -1 && valuePos == -1) {
                        skipChar(text, pos, ' ');
                        keyPos = pos;
                } else if(keyPos != -1 && valuePos == -1) {
                        if(!findChar(text, pos, '=')) break;
                        std::string currKey(&text[keyPos], pos - keyPos);
                        if(currKey == key) {
                                pos ++;
                                keyPos = -1;
                                valuePos = pos;
                        } else {
                                if(!findChar(text, pos, ';')) break;
                                pos ++;
                                keyPos = -1;
                                valuePos = -1;
                        }
                } else if(keyPos == -1 && valuePos != -1) {
                        if(!findChar(text, pos, ';')) break;
                        value = std::string(&text[valuePos], pos - valuePos);
                        break;
                }
        }

        return value;

}

EvercastAuth::Token EvercastAuth::getTokenInfoFromCookies(const httplib::Headers& headers) {

        Token result;

        const auto& map = headers;
        auto itlow = map.lower_bound("Set-Cookie");
        auto itup = map.upper_bound("Set-Cookie");

        for(auto it = itlow; it != itup; it ++) {

                auto token = parseValue(it->second, "__Host-jwt");
                auto nonce = parseValue(it->second, "__Host-nonce");

                if(!token.empty()) result.token = token;
                if(!nonce.empty()) result.nonce = nonce;

        }

        return result;

}

nlohmann::json EvercastAuth::createLoginQuery(const Credentials& credentials) {

        nlohmann::json j;
        j["query"] = "mutation authenticateMutation($input: AuthenticateInput!) {authenticate(input: $input) {clientMutationId}}";
        j["variables"] = {
                {"input",
                        {
                                { "email", credentials.email },
                                { "password", credentials.password },
                                { "trackingId", credentials.trackingId }
                        }
                }
        };

        return j;

}

nlohmann::json EvercastAuth::createStreamKeyQuery() {

        nlohmann::json j;
	// The below strange construct is needed because some compilers on runner-up operating systems resolve {} to null
	nlohmann::json input = nlohmann::ordered_map<std::string, std::string>();
        j["query"] = "mutation getStreamKeyMutation(\n  $input: GetStreamKeyInput!\n) {\n  getStreamKey(input: $input) {\n    uuid\n  }\n}\n";
        j["variables"] = {
                {"input", input}
        };
        return j;

}

nlohmann::json EvercastAuth::createRoomsQuery() {

        nlohmann::json j;
        j["query"] = "query homeQuery {\n"
		"\tcurrentProfile {\n"
		"\t\t...Home_currentProfile\n"
		"\t}\n"
		"\tcanCreateRoom\n"
		"}\n"
		"\n"
		"fragment Home_currentProfile on Profile {\n"
		"\n"
		"\trooms: liveroomsByCreatorId(first: 100, orderBy: [CREATED_AT_DESC]) {\n"
		"\t\tnodes {\n"
		"\t\t\tid\n"
		"\t\t\thash\n"
		"\t\t\tcreatedAt\n"
		"\t\t\tdeletedAt\n"
		"\t\t\t...RoomCard_room\n"
		"\t\t}\n"
		"\t}\n"
		"\tinvites: invitesByProfileId(first: 100, orderBy: [CREATED_AT_DESC]) {\n"
		"\t\tnodes {\n"
		"\t\t\tliveroomByRoomId {\n"
		"\t\t\t\tid\n"
		"\t\t\t\tcreatedAt\n"
		"\t\t\t\tdeletedAt\n"
		"\t\t\t\t...RoomCard_room\n"
		"\t\t\t\tprofileByCreatorId {\n"
		"\t\t\t\t\tdisplayName\n"
		"\t\t\t\t}\n"
		"\t\t\t}\n"
		"\t\t}\n"
		"\t}\n"
		"\trecentRooms: joinedRoomsByProfileId(first: 100, orderBy: [JOINED_AT_DESC]) {\n"
		"\t\tnodes {\n"
		"\t\t\tjoinedAt\n"
		"\t\t\tliveroomByRoomId {\n"
		"\t\t\t\tid\n"
		"\t\t\t\tcreatedAt\n"
		"\t\t\t\tdeletedAt\n"
		"\t\t\t\t...RoomCard_room\n"
		"\t\t\t}\n"
		"\t\t}\n"
		"\t}\n"
		"}\n"
		"\n"
		"fragment RoomCard_room on Liveroom {\n"
		"\tid\n"
		"\thash\n"
		"\tdisplayName\n"
		"\tcreatorId\n"
		"\tcreatedAt\n"
		"\tsessionCount\n"
		"\tnoteCount\n"
		"}";

        return j;

}

EvercastAuth::Token EvercastAuth::obtainToken(const Credentials& credentials) {

        std::string apiURL = config_get_string(GetGlobalConfig(), "General", "evercast_url_graphql");
        blog(LOG_INFO, "apiURL='%s'", apiURL.c_str());
        const auto& urlComponents = parseUrlComponents(apiURL);

	httplib::Client client(urlComponents.baseUrl.c_str());

        auto query = createLoginQuery(credentials);

        auto res = client.Post(urlComponents.path.c_str(), query.dump(), "application/json");

        if (res) {
                return getTokenInfoFromCookies(res->headers);
        }

	return {};

}

std::string EvercastAuth::obtainStreamKey(const Token& token) {

        std::string apiURL = config_get_string(GetGlobalConfig(), "General", "evercast_url_graphql");
        blog(LOG_INFO, "apiURL='%s'", apiURL.c_str());
        const auto& urlComponents = parseUrlComponents(apiURL);

        httplib::Client client(urlComponents.baseUrl.c_str());

        auto query = createStreamKeyQuery();

        httplib::Headers headers({
                                         {"X-Double-Submit", token.nonce},
                                         {"cookie", "__Host-nonce=" + token.nonce + "; __Host-jwt=" + token.token}
                                 });

        auto res = client.Post(urlComponents.path.c_str(), headers, query.dump(), "application/json");

        if (res) {
		try {
			auto j = nlohmann::json::parse(res->body);
			return j["data"]["getStreamKey"]["uuid"];
		} catch (nlohmann::json::exception e) {
                        blog(LOG_ERROR, "[EvercastAuth::obtainStreamKey]: '%s'", e.what());
		}
        }

	return "";

}

EvercastAuth::Rooms EvercastAuth::obtainRooms(const Token& token) {

        std::string apiURL = config_get_string(GetGlobalConfig(), "General", "evercast_url_graphql");
        blog(LOG_INFO, "apiURL='%s'", apiURL.c_str());
        const auto& urlComponents = parseUrlComponents(apiURL);

        httplib::Client client(urlComponents.baseUrl.c_str());

        auto query = createRoomsQuery();

        httplib::Headers headers({
                                         {"X-Double-Submit", token.nonce},
                                         {"cookie", "__Host-nonce=" + token.nonce + "; __Host-jwt=" + token.token}
                                 });

        auto res = client.Post(urlComponents.path.c_str(), headers, query.dump(), "application/json");

        if (res) {

                std::unordered_map<std::string, Room> allRooms;

		try {

			auto j = nlohmann::json::parse(res->body);

			for (auto &room : j["data"]["currentProfile"]["recentRooms"]["nodes"].items()) {
				auto jId =room.value()["liveroomByRoomId"]["id"];
				auto jName = room.value()["liveroomByRoomId"]["displayName"];
				if (!jId.empty() && !jName.empty()) {
					Room r = {jId.get<std::string>(),jName.get<std::string>()};
					allRooms.insert({jId.get<std::string>(), r});
				}
			}

			for (auto &room : j["data"]["currentProfile"]["invites"]["nodes"].items()) {
				auto jId =room.value()["liveroomByRoomId"]["id"];
				auto jName = room.value()["liveroomByRoomId"]["displayName"];
				if (!jId.empty() && !jName.empty()) {
					Room r = {jId.get<std::string>(),jName.get<std::string>()};
					allRooms.insert({jId.get<std::string>(), r});
				}
			}

			for (auto &room : j["data"]["currentProfile"]["rooms"]["nodes"] .items()) {
				auto jId = room.value()["id"];
				auto jName = room.value()["displayName"];
				if (!jId.empty() && !jName.empty()) {
					Room r = {jId.get<std::string>(),jName.get<std::string>()};
					allRooms.insert({jId.get<std::string>(), r});
				}
			}

		} catch (nlohmann::json::exception e) {
                        blog(LOG_ERROR, "[EvercastAuth::obtainRooms]: '%s'", e.what());
                }

                Rooms rooms;

                for(auto& pair : allRooms) {
                        rooms.ordered.push_back(pair.second);
                }

                sort(rooms.ordered.begin(), rooms.ordered.end(), []( const Room& r1, const Room& r2 ) {
			return r1.name < r2.name;
		});

		return rooms;

        }

        return {};

}

void EvercastAuth::updateState() {

	auto token = getToken();
	if(token.empty()) {
		/* get token if it's empty */
                token = obtainToken(getCredentials());
		setToken(token);
                if(token.empty()) {
                        return;
                }
	}

	auto streamKey = obtainStreamKey(token);
	if(streamKey.empty()) {
		/* get a new token, old one was revoked */
                token = obtainToken(getCredentials());
                setToken(token);
		if(token.empty()) {
                        setStreamKey("");
			return;
		}
		streamKey = obtainStreamKey(token);
	}
        setStreamKey(streamKey);
        if(streamKey.empty()) {
                return;
        }

	setRooms(obtainRooms(token));

}

void EvercastAuth::loadState(obs_data_t *settings) {

	Credentials credentials;
        credentials.email = obs_data_get_string(settings, "evercast_auth_email");
        credentials.trackingId = obs_data_get_string(settings, "evercast_auth_tracking_id");

	Token token;
	token.token = obs_data_get_string(settings, "evercast_auth_token");
	token.nonce = obs_data_get_string(settings, "evercast_auth_token_nonce");

	std::string streamKey;
	streamKey = obs_data_get_string(settings, "evercast_auth_stream_key");

	setCredentials(credentials);
	setToken(token);
	setStreamKey(streamKey);
}

void EvercastAuth::saveState(obs_data_t *settings) {

	const auto& credentials = getCredentials();
        const auto& token = getToken();
        const auto& streamKey = getStreamKey();

        obs_data_set_string(settings, "evercast_auth_email", credentials.email.c_str());
        obs_data_set_string(settings, "evercast_auth_tracking_id", credentials.trackingId.c_str());

        obs_data_set_string(settings, "evercast_auth_token", token.token.c_str());
        obs_data_set_string(settings, "evercast_auth_token_nonce", token.nonce.c_str());

        obs_data_set_string(settings, "evercast_auth_stream_key", streamKey.c_str());

}

void EvercastAuth::clearCurrentState() {
	setCredentials({getCredentials().email, "", getCredentials().trackingId});
	setToken({});
	setStreamKey("");
	setRooms({});
}

void EvercastAuth::setCredentials(const Credentials& credentials) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_credentials = credentials;
}

EvercastAuth::Credentials EvercastAuth::getCredentials() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_credentials;
}

void EvercastAuth::setToken(const Token& token) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_token = token;
}

EvercastAuth::Token EvercastAuth::getToken() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_token;
}

void EvercastAuth::setStreamKey(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_streamKey = key;
}

std::string EvercastAuth::getStreamKey() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_streamKey;
}

void EvercastAuth::setRooms(const Rooms& rooms) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_rooms = rooms;
}

EvercastAuth::Rooms EvercastAuth::getRooms() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_rooms;
}
