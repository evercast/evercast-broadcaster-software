
#include "auth-evercast.hpp"

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

EvercastAuth::TokenInfo EvercastAuth::getTokenInfoFromCookies(const httplib::Headers& headers) {

        TokenInfo result;

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

nlohmann::json EvercastAuth::createLoginQuery(const std::string& email, const std::string& password, const std::string& trackingId) {

        nlohmann::json j;
        j["query"] = "mutation authenticateMutation($input: AuthenticateInput!) {authenticate(input: $input) {clientMutationId}}";
        j["variables"] = {
                {"input",
                        {
                                { "email", email },
                                { "password", password },
                                { "trackingId", trackingId }
                        }
                }
        };

        return j;

}

nlohmann::json EvercastAuth::createStreamKeyQuery() {

        nlohmann::json j;
        j["query"] = "mutation getStreamKeyMutation(\n  $input: GetStreamKeyInput!\n) {\n  getStreamKey(input: $input) {\n    uuid\n  }\n}\n";
        j["variables"] = {
                {"input", {}}
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

EvercastAuth::AuthInfo EvercastAuth::getAuthInfo(const std::string& email, const std::string& password, const std::string& trackingId) {

	TokenInfo tokenInfo;
        AuthInfo authInfo;

        httplib::Client client("https://v2.evercast.us");

	{
		auto query = createLoginQuery(email, password, trackingId);

		auto res = client.Post("/api/graphql", query.dump(), "application/json");

		if (!res) {
			return authInfo;
		}

                tokenInfo = getTokenInfoFromCookies(res->headers);
		if(tokenInfo.nonce.empty() || tokenInfo.token.empty()) {
			return authInfo;
		}
	}

        {

                auto query = createStreamKeyQuery();

                httplib::Headers headers({
                                                 {"X-Double-Submit", tokenInfo.nonce},
                                                 {"cookie", "__Host-nonce=" + tokenInfo.nonce + "; __Host-jwt=" + tokenInfo.token}
                                         });

                auto res = client.Post("/api/graphql", headers, query.dump(), "application/json");

                if (!res) {
                        return authInfo;
                }

                auto j = nlohmann::json::parse(res->body);
                authInfo.streamKey = j["data"]["getStreamKey"]["uuid"];

        }

        {

                auto query = createRoomsQuery();

                httplib::Headers headers({
                                                 {"X-Double-Submit", tokenInfo.nonce},
                                                 {"cookie", "__Host-nonce=" + tokenInfo.nonce + "; __Host-jwt=" + tokenInfo.token}
                                         });

                auto res = client.Post("/api/graphql", headers, query.dump(), "application/json");

                if (!res) {
                        return authInfo;
                }

                auto j = nlohmann::json::parse(res->body);

                for(auto& room : j["data"]["currentProfile"]["recentRooms"]["nodes"].items()) {

			auto jId = room.value()["liveroomByRoomId"]["id"];
			auto jName = room.value()["liveroomByRoomId"]["displayName"];

			if(!jId.empty() && !jName.empty()) {
				RoomInfo roomInfo;
				roomInfo.id = room.value()["liveroomByRoomId"]["id"].get<std::string>();
				roomInfo.name = room.value()["liveroomByRoomId"]["displayName"].get<std::string>();
				authInfo.rooms.push_back(roomInfo);
			}
                }

        }

	authInfo.success = true;

	return authInfo;

}

EvercastAuth::Credentials EvercastAuth::loadCredentials(obs_data_t *settings) {
        Credentials creds;
        creds.email = obs_data_get_string(settings, "evercast_auth_email");
	creds.password = obs_data_get_string(settings, "evercast_auth_password");
        creds.trackingId = obs_data_get_string(settings, "evercast_auth_tracking_id");
	return creds;
}

void EvercastAuth::saveCredentials(const Credentials& creds, obs_data_t *settings) {
        obs_data_set_string(settings, "evercast_auth_email", creds.email.c_str());
        obs_data_set_string(settings, "evercast_auth_password", creds.password.c_str());
        obs_data_set_string(settings, "evercast_auth_tracking_id", creds.trackingId.c_str());
}