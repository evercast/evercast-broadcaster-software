#include "EvercastSessionData.h"

std::map<long long, EvercastSessionData*> EvercastSessionData::sessions = {};

EvercastSessionData* EvercastSessionData::findOrCreateSession(long long key)
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

void EvercastSessionData::storeIceServers(std::vector<IceServerDefinition> &servers)
{
    // Actually modify ice servers
    {
        const std::lock_guard<std::mutex> lock(initialization_mutex);
        this->ice_servers = std::vector<IceServerDefinition>(servers);
    }

    initialized_condition.notify_all();
}

std::vector<IceServerDefinition> EvercastSessionData::awaitIceServers()
{
    std::unique_lock<std::mutex> server_lock(initialization_mutex);

    while(!closing && ice_servers.empty()) {
        initialized_condition.wait(server_lock);
    }

    server_lock.unlock();

    return ice_servers;
}

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
