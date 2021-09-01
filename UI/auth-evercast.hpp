#pragma once

#include <obs-module.h>
#include <json11.hpp>

#include <thread>
#include <mutex>
#include <vector>
#include <unordered_map>

class EvercastAuth {
public:

	struct Credentials {
                std::string email;
		std::string password;
		std::string trackingId;
	};

        struct Token {

                std::string token;
                std::string nonce;

		bool empty() const {
			return token.empty();
		}

        };

	struct Room {
		std::string id;
		std::string name;
		std::string hash;
	};

	struct Rooms {
                std::vector<Room> ordered;
	};

public:

	struct HttpResponse {
                long code;
		std::string body;
		std::string error;
		std::vector<std::string> headers;
	};

private:

	static HttpResponse execHttp(const std::string& url,
                                     const std::string& body,
                                     const std::vector<std::string>& headers = std::vector<std::string>({}),
				     int timeoutSec = 5);

        static void skipChar(const std::string& text, int& pos, char c);
        static bool findChar(const std::string& text, int& pos, char c);
        static std::string parseValue(const std::string& text,  const std::string& key);
        static Token getTokenInfoFromCookies(const std::vector<std::string>& headers);

private:

        static json11::Json createLoginQuery(const Credentials& credentials);
        static json11::Json createStreamKeyQuery();
        static json11::Json obtainStreamKeyQuery();
        static json11::Json createRoomsQuery();
        static json11::Json createOneRoomQuery(const std::string& roomHash);
        static json11::Json createIsRoomJoinableQuery(const std::string& roomHash);

        static Token obtainToken(const Credentials& credentials, const std::string& apiUrl);
	static std::string createStreamKey(EvercastAuth::HttpResponse& httpResponse, const Token& token, const std::string& apiUrl);
        static std::string obtainStreamKey(EvercastAuth::HttpResponse& httpResponse, const Token& token, const std::string& apiUrl);
        static Rooms obtainRooms(EvercastAuth::HttpResponse& httpResponse, const Token& token, const std::string& apiUrl);
	static Room obtainOneRoomInfo(EvercastAuth::HttpResponse& httpResponse,
				      const Token& token,
                                      const std::string& roomHash,
				      const std::string& apiUrl);

	static bool obtainIsRoomJoinable(EvercastAuth::HttpResponse& httpResponse,
					 const Token& token,
                                         const std::string& roomHash,
                                         const std::string& apiUrl);

public:

	void updateState(std::string apiUrl = "");
        bool getIsRoomJoinable(EvercastAuth::HttpResponse& httpResponse, const std::string& roomHash, std::string apiUrl = "");
	Room getOneRoomInfo(EvercastAuth::HttpResponse& httpResponse, const std::string& roomHash, std::string apiUrl = "");

private:
        std::mutex m_mutex;
        Credentials m_credentials;
	Token m_token;
	std::string m_streamKey;
	Rooms m_rooms;
public:

        void loadState(obs_data_t *settings);
	void saveState(obs_data_t *settings);

	void clearCurrentState();

	template<typename Callback>
	void updateState(Callback callback, const std::string& apiUrl = "") {

                std::thread t([this, callback, apiUrl]{
			updateState(apiUrl);
			callback();
		});

		t.detach();

	}

        template<typename Callback>
        void getIsRoomJoinable(Callback callback, const std::string& roomHash, const std::string& apiUrl = "") {

                std::thread t([this, callback, roomHash, apiUrl]{
                        EvercastAuth::HttpResponse httpResponse;
                        auto isJoinable = getIsRoomJoinable(httpResponse, roomHash, apiUrl);
                        callback(httpResponse, isJoinable, roomHash);
                });

                t.detach();

        }

        template<typename Callback>
        void getRoomInfo(Callback callback, const std::string& roomHash, const std::string& apiUrl = "") {

                std::thread t([this, callback, roomHash, apiUrl]{
                        EvercastAuth::HttpResponse httpResponse;
                        auto room = getOneRoomInfo(httpResponse, roomHash, apiUrl);
                        callback(httpResponse, room);
                });

                t.detach();

        }

	void setCredentials(const Credentials& credentials);

	Credentials getCredentials();

	void setToken(const Token& token);

        Token getToken();

	void setStreamKey(const std::string& key);

	std::string getStreamKey();

	void setRooms(const Rooms& rooms);

	Rooms getRooms();

};