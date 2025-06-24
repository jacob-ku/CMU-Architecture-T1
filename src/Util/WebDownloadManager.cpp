#include "WebDownloadManager.h"
#include <chrono>
#include <iostream>

WebDownloadManager::WebDownloadManager() : enableTimeLogging(false)
{

}


WebDownloadManager::~WebDownloadManager()
{


}

void WebDownloadManager::setTimeLogging(bool enable)
{
    enableTimeLogging = enable;
}

bool WebDownloadManager::isTimeLoggingEnabled() const
{
    return enableTimeLogging;
}

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, FILE *userp)
{
    size_t written = fwrite(contents, size, nmemb, userp);
    return written;
}


bool WebDownloadManager::downloadFile(const std::string &url, const std::string &outputPath)
{
    // 시간 측정 변수 선언
    std::chrono::high_resolution_clock::time_point start_time;
    if (enableTimeLogging) {
        start_time = std::chrono::high_resolution_clock::now();
    }

    CURL *curl;
    FILE *fp;
    CURLcode res;
    std::cout << "Starting download from: " << url << std::endl;

    curl = curl_easy_init();
    if (curl) {
        fp = fopen(outputPath.c_str(), "wb");
        if (!fp) {
            std::cout << "Failed to open file for writing: " << outputPath << std::endl;
            return false;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        std::cout << "setopt finished: " << url << std::endl;

        res = curl_easy_perform(curl);

        fclose(fp);
        curl_easy_cleanup(curl);

        if (enableTimeLogging) {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            double seconds = duration.count() / 1000.0;

            if (res != CURLE_OK) {
                std::cout << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
                std::cout << "Download failed after " << seconds << " seconds" << std::endl;
                return false;
            }

            std::cout << "Download completed successfully in " << seconds << " seconds" << std::endl;
        } else {
            if (res != CURLE_OK) {
                std::cout << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
                return false;
            }
        }
        std::cout << "File downloaded successfully to: " << outputPath << std::endl;
        return true;
    }

    

    // CURL 초기화 실패 시
    // if (enableTimeLogging) {
    //     auto end_time = std::chrono::high_resolution_clock::now();
    //     auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    //     double seconds = duration.count() / 1000.0;
    //     std::cout << "Failed to initialize CURL after " << seconds << " seconds." << std::endl;
    // } else {
    //     std::cout << "Failed to initialize CURL." << std::endl;
    // }

    return false;
}