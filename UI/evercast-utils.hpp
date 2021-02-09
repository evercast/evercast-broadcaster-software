
#ifndef OBS_STUDIO_EVERCAST_UTILS_HPP
#define OBS_STUDIO_EVERCAST_UTILS_HPP

#include <string>

class EvercastUtils {
public:

        struct UrlComponents {
                std::string scheme;
                std::string domain;
                std::string path;
        };

public:

        static UrlComponents parseUrlComponents(const std::string& url);
        static std::string parseRoomIdFromUrl(const std::string& url);

	static std::string getGraphApiUrlForDomain(const std::string& domain);
        static std::string getWSApiUrlForDomain(const std::string& domain);

	static std::string getRoomUrlForDomain(const std::string& domain, const std::string& roomId);

        static void setDefaultGraphApiUrl(const std::string& url);
	static void setGraphApiUrl(const std::string& url);
	static std::string getGraphApiUrl();

        static void setDefaultWSApiUrl(const std::string& url);
        static void setWSApiUrl(const std::string& url);
	static std::string getWSApiUrl();

	static std::string getRoomUrl(const std::string& roomId);

        static void initDefaults();

};

#endif //OBS_STUDIO_EVERCAST_UTILS_HPP
