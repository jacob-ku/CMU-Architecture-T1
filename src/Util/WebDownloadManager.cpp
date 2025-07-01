#include "WebDownloadManager.h"
#include <chrono>
#include <iostream>
#include <string>

WebDownloadManager::WebDownloadManager() : enableTimeLogging(true)
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

// String용 콜백 함수
static size_t WriteStringCallback(void *contents, size_t size, size_t nmemb, std::string *userp)
{
    size_t totalSize = size * nmemb;
    userp->append((char*)contents, totalSize);
    return totalSize;
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

bool WebDownloadManager::downloadFile(const std::string &url, const std::string &outputPath, std::function<void(bool, const std::string&)> callback)
{
    // 시간 측정 변수 선언
    std::chrono::high_resolution_clock::time_point start_time;
    if (enableTimeLogging) {
        start_time = std::chrono::high_resolution_clock::now();
    }

    CURL *curl;
    FILE *fp;
    CURLcode res;
    std::cout << "Starting download with callback from: " << url << std::endl;

    curl = curl_easy_init();
    if (curl) {
        fp = fopen(outputPath.c_str(), "wb");
        if (!fp) {
            std::cout << "Failed to open file for writing: " << outputPath << std::endl;
            if (callback) {
                callback(false, "Failed to open file for writing: " + outputPath);
            }
            return false;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        std::cout << "setopt finished with callback: " << url << std::endl;

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
                if (callback) {
                    callback(false, "curl_easy_perform() failed: " + std::string(curl_easy_strerror(res)));
                }
                return false;
            }

            std::cout << "Download completed successfully in " << seconds << " seconds" << std::endl;
            if (callback) {
                callback(true, "Download completed successfully in " + std::to_string(seconds) + " seconds");
            }
        } else {
            if (res != CURLE_OK) {
                std::cout << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
                if (callback) {
                    callback(false, "curl_easy_perform() failed: " + std::string(curl_easy_strerror(res)));
                }
                return false;
            }
            if (callback) {
                callback(true, "Download completed successfully");
            }
        }
        std::cout << "File downloaded successfully to: " << outputPath << std::endl;
        return true;
    }

    // CURL 초기화 실패 시
    if (callback) {
        callback(false, "Failed to initialize CURL");
    }
    return false;
}

std::string WebDownloadManager::downloadToString(const std::string &url)
{
    // 시간 측정 변수 선언
    std::chrono::high_resolution_clock::time_point start_time;
    if (enableTimeLogging) {
        start_time = std::chrono::high_resolution_clock::now();
        std::cout << "Starting string download from: " << url << std::endl;
    }

    CURL *curl;
    CURLcode res;
    long response_code = 0;
    std::string response;

    curl = curl_easy_init();
    if (curl) {
        // URL 설정
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        
        // 콜백 함수 설정 (string용)
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteStringCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        
       
        curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
        
        // SSL 검증 비활성화 (필요시)
        // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        
        // 리다이렉트 따라가기
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        
        // 타임아웃 설정 (30초)
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

        // 다운로드 실행
        res = curl_easy_perform(curl);
        
        // 시간 측정 및 출력
        if (enableTimeLogging) {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            double seconds = duration.count() / 1000.0;
            std::cout << "String download completed in " << seconds << " seconds." << std::endl;
        }

        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            std::cout << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            return ""; // 실패 시 빈 문자열 반환
        }

        // CURLE_OK
        if (enableTimeLogging) {
            std::cout << "String downloaded successfully. Content length: " << response.length() << " bytes" << std::endl;
        }
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        if(response_code >= 400 || response_code < 500) {   // client error including 404 NOT_FOUND
            return "400";   // TODO: fix this   
        } else {
            return response;
        }
    }

    // CURL 초기화 실패 시
    if (enableTimeLogging) {
        std::cout << "Failed to initialize CURL for string download." << std::endl;
    }
    
    return ""; // 실패 시 빈 문자열 반환
}

std::time_t WebDownloadManager::getLastModifiedTime(const std::string &url)
{
    CURL *curl;
    CURLcode res;
    long response_code = 0;
    std::time_t last_modified = 0;

    curl = curl_easy_init();
    if (curl) {
        // HEAD 요청으로 헤더 정보만 가져오기
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); // HEAD 요청
        curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
        curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            // HTTP 응답 코드 확인
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            
            if (response_code == 200) {
                // Last-Modified 시간 가져오기
                curl_easy_getinfo(curl, CURLINFO_FILETIME, &last_modified);
                
                if (enableTimeLogging) {
                    if (last_modified > 0) {
                        std::cout << "[WebDownloadManager] Last modified time retrieved for " << url << ": " << last_modified << std::endl;
                    } else {
                        std::cout << "[WebDownloadManager] No last modified time available for " << url << std::endl;
                    }
                }
            } else {
                if (enableTimeLogging) {
                    std::cout << "[WebDownloadManager] HTTP response code " << response_code << " for " << url << std::endl;
                }
            }
        } else {
            if (enableTimeLogging) {
                std::cout << "[WebDownloadManager] Failed to get last modified time for " << url << ": " << curl_easy_strerror(res) << std::endl;
            }
        }

        curl_easy_cleanup(curl);
    }

    return last_modified;
}

