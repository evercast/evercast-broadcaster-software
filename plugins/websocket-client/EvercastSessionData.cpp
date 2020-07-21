#include "EvercastSessionData.h"

std::map<long long, EvercastSessionData *> EvercastSessionData::sessions = {};

/***** STATIC SESSION MANAGEMENT FUNCTIONS *****/

EvercastSessionData *EvercastSessionData::findOrCreateSession(long long key)
{
	EvercastSessionData *result = sessions[key];
	if (NULL == result) {
		result = new EvercastSessionData(key);
		sessions[key] = result;
	}

	return result;
}

bool EvercastSessionData::terminateSession(long long key)
{
	EvercastSessionData *data = sessions[key];
	if (NULL == data) {
		return false;
	}

	sessions.erase(key);
	delete data;

	return true;
}

/***** DATA STORAGE *****/

void EvercastSessionData::storeAttendees(std::vector<AttendeeIdentifier>& attendees)
{
	// Store attendee information
	{
		const std::lock_guard<std::mutex> lock(initialization_mutex);
		this->meeting_attendees = std::vector<AttendeeIdentifier>(attendees);
	}

	initialized_condition.notify_all();
}

void EvercastSessionData::storeIceServers(
	std::vector<IceServerDefinition> &servers)
{
	// Actually modify ice servers
	{
		const std::lock_guard<std::mutex> lock(initialization_mutex);
		this->ice_servers = std::vector<IceServerDefinition>(servers);
	}

	initialized_condition.notify_all();
}

bool EvercastSessionData::awaitJoinComplete(int timeoutSeconds)
{
	std::unique_lock<std::mutex> server_lock(initialization_mutex);

	initialized_condition.wait_for(
		server_lock,
		std::chrono::seconds(timeoutSeconds),
		[&] {
			return closing || (!ice_servers.empty() && !meeting_attendees.empty());
		});

	return !ice_servers.empty() && !meeting_attendees.empty();
}

std::vector<AttendeeIdentifier> EvercastSessionData::getAttendees()
{
	return std::vector<AttendeeIdentifier>(meeting_attendees);
}

std::vector<IceServerDefinition> EvercastSessionData::getIceServers()
{
	return std::vector<IceServerDefinition>(ice_servers);
}

/***** CONSTRUCTOR/DESTRUCTOR *****/

EvercastSessionData::EvercastSessionData(long long key)
{
	this->session_key = key;
	this->closing = false;
}

EvercastSessionData::~EvercastSessionData()
{
	closing = true;

	// Release init condition so that anyone who is waiting can wrap up and let go
	initialized_condition.notify_all();

	// Remove self from static connection collection
	terminateSession(this->session_key);
}
