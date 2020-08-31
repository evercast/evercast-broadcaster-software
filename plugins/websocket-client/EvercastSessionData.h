#ifndef __EVERCAST_SESSION_DATA_H__
#define __EVERCAST_SESSION_DATA_H__

#include "WebsocketClient.h"
#include "WebRTCSessionEventHandler.h"

#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>
#include <map>
#include <WebsocketClient.h>

struct IceServerDefinition {
    std::string urls;
    std::string username;
    std::string password;
};

struct AttendeeIdentifier {
    std::string id;
    std::string display;
};

/**
 * Manages Evercast session state.  Lifetime of instances of this type will match
 * the lifetime of Evercast WebSockets.
 */
class WEBSOCKETCLIENT_API EvercastSessionData {
public:
    // Create a new session instance, or find one with the appropriate key.
    static EvercastSessionData *findOrCreateSession(long long key);
    // Destroy the specified session instance.
    static bool terminateSession(long long key);

    // Wait for join data to be provided by the client.
    bool awaitJoinComplete(int timeoutSeconds);

    // Write attendee list to state.
    void storeAttendees(std::vector<AttendeeIdentifier> &attendees);

    void attendeeArrived(AttendeeIdentifier attendee);

    void attendeeLeft(std::string attendeeId);

    void registerEventHandler(WebRTCSessionEventHandler *handler);

    // Write a list of ICE servers to the session for use by its owner
    void storeIceServers(std::vector<IceServerDefinition> &servers);

    std::vector<AttendeeIdentifier> getAttendees();
    std::vector<IceServerDefinition> getIceServers();

private:
    static std::map<long long, EvercastSessionData*> sessions;

    long long session_key;
    bool closing;

    std::mutex initialization_mutex;
    std::condition_variable initialized_condition;
    std::vector<AttendeeIdentifier> meeting_attendees;
    std::vector<IceServerDefinition> ice_servers;

    WebRTCSessionEventHandler *event_handler;

    EvercastSessionData(long long key);
    ~EvercastSessionData();
};

#endif
