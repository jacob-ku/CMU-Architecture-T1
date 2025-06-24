#include <curl/curl.h>

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

    void setTimeLogging(bool enable);
    bool isTimeLoggingEnabled() const;
};