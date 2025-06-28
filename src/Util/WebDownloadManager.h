#ifndef WEB_DOWNLOAD_MANAGER_H
#define WEB_DOWNLOAD_MANAGER_H
#include <curl/curl.h>
#include <functional>
#include <ctime>

#define CURL_STATICLIB
#pragma comment(lib, "libcurl-bcc-x64.lib")


#include <iostream>

class WebDownloadManager {
private:
    bool enableTimeLogging;

public:
	 WebDownloadManager();
	~WebDownloadManager();
    bool downloadFile(const std::string &url, const std::string &outputPath);
    bool downloadFile(const std::string &url, const std::string &outputPath, std::function<void(bool, const std::string&)> callback);
    std::string downloadToString(const std::string &url);
    std::time_t getLastModifiedTime(const std::string &url);

    void setTimeLogging(bool enable);
    bool isTimeLoggingEnabled() const;
};
#endif // WEB_DOWNLOAD_MANAGER_H