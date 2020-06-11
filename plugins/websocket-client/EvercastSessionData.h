#ifndef __EVERCAST_SESSION_DATA_H__
#define __EVERCAST_SESSION_DATA_H__

#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>
#include <map>

struct IceServerDefinition {
	std::string urls;
	std::string username;
	std::string password;
};

/**
 * Manages Evercast session state.  Currently only used to hold ICE servers and
 * provide access to them from outside; other state will follow.  Lifetime of
 * instances of this type will match the lifetime of Evercast WebSockets.
 */
class EvercastSessionData {
public:
    // Create a new session instance, or find one with the appropriate key.
	static EvercastSessionData *findOrCreateSession(long long key);
    // Destroy the specified session instance.
	static bool terminateSession(long long key);

    // Wait for ICE servers to be provided by the client.
	std::vector<IceServerDefinition> awaitIceServers();

    // Write a list of ICE servers to the session for use by its owner
    void storeIceServers(std::vector<IceServerDefinition> &servers);

private:
	static std::map<long long, EvercastSessionData*> sessions;

    long long session_key;
    bool closing;

	std::mutex initialization_mutex;
	std::condition_variable initialized_condition;
	std::vector<IceServerDefinition> ice_servers;

	EvercastSessionData(long long key);
	~EvercastSessionData();
};

#endif
