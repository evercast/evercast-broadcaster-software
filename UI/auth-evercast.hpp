#pragma once

#include "httplib.h"
#include "json.hpp"

#include <obs-module.h>

#include <thread>
#include <mutex>
#include <vector>

class EvercastAuth {
public:

	struct Credentials {
                std::string email;
		std::string password;
		std::string trackingId;
	};

	struct RoomInfo {
		std::string id;
		std::string name;
	};

	struct AuthInfo {
		bool success = false;
                std::string streamKey;
		std::vector<RoomInfo> rooms;
	};

private:
        struct TokenInfo {
                std::string token;
                std::string nonce;
        };

private:

        static void skipChar(const std::string& text, int& pos, char c);
        static bool findChar(const std::string& text, int& pos, char c);
        static std::string parseValue(const std::string& text,  const std::string& key);
        static TokenInfo getTokenInfoFromCookies(const httplib::Headers& headers);

private:

        static nlohmann::json createLoginQuery(const std::string& email, const std::string& password, const std::string& trackingId);
        static nlohmann::json createStreamKeyQuery();
        static nlohmann::json createRoomsQuery();

private:
        Credentials m_credentials;
	AuthInfo m_authInfo;
	std::mutex m_mutex;
public:


        static Credentials loadCredentials(obs_data_t *settings);
	static void saveCredentials(const Credentials& creds, obs_data_t *settings);

        static AuthInfo getAuthInfo(const std::string& email, const std::string& password, const std::string& trackingId);

	template<typename Callback>
	void login(Callback callback) {

                std::thread t([this, callback]{

			const auto& creds = getCredentials();
			const auto& authInfo = getAuthInfo(creds.email, creds.password, creds.trackingId);

			{
				std::lock_guard<std::mutex> lock(m_mutex);
				m_authInfo = authInfo;
			}

			callback();

		});

		t.detach();

	}

	void setCredentials(const Credentials& credentials) {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_credentials = credentials;
	}

	Credentials getCredentials() {
                std::lock_guard<std::mutex> lock(m_mutex);
		return m_credentials;
	}

	AuthInfo getAuthInfo() {
                std::lock_guard<std::mutex> lock(m_mutex);
		return m_authInfo;
	}

};
