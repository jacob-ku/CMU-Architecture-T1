#include "MetadataUpdater.h"
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <future>
#include <memory>
#include <ctime>
#include <chrono>
#include <functional>

#include <locale>
#include <codecvt>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;


bool MetaDataUpdater::update(const string& url, const string& file, const std::function<void(bool)>& callback) {
    std::cout << "[MetaDataUpdater] update from" << url << std::endl;
    
    if (isUpdateThreadRunning) {
        std::cout << "[MetaDataUpdater] Update thread is already running" << std::endl;
        return true;
    }
    
    if (callback) {
        std::cout << "[MetaDataUpdater] Update callback function set from parameter" << std::endl;
    } else {
        std::cout << "[MetaDataUpdater] No callback function provided" << std::endl;
    }
    
    isUpdateThreadRunning = true;
    shouldStop = false;
    
    updateThread = std::thread([this, url, file, callback]() {
        std::cout << "[UPDATE_THREAD] Starting periodic update thread..." << std::endl;
        
        // auto lastCallbackTime = std::chrono::steady_clock::now();
        const auto oneHour = std::chrono::hours(1);
        const auto thirtyMinutes = std::chrono::minutes(30);
        const auto tenMinutes = std::chrono::minutes(10);
        const auto tenSeconds = std::chrono::seconds(10);

        const auto period = thirtyMinutes; // Check every 30 minutes
        // const auto period = tenMinutes; // Check every 10 minutes

        int periodInSeconds = std::chrono::duration_cast<std::chrono::seconds>(period).count();
        
        while (!shouldStop) {
            // auto currentTime = std::chrono::steady_clock::now();
            
            // std::cout << "[UPDATE_THREAD] Performing 30-minute check..." << std::endl;
            // std::time_t remoteUpdatedTime = webDownloadManager.getLastModifiedTime(url);
            // if (remoteUpdatedTime == 0) {
            //     std::cout << "[UPDATE_THREAD] Failed to get last modified time for: " << url << std::endl;
            //     std::cout << "[UPDATE_THREAD] Retrying after 1800 seconds..." << std::endl;

            //     for(int i = 0; i < periodInSeconds && !shouldStop; ++i) {
            //         std::this_thread::sleep_for(std::chrono::seconds(1));
            //     }
            //     continue;
            // }
            // std::time_t currentDBFileTime = FileLastWriteTimeToTimeT(file);
            // std::cout << "[UPDATE_THREAD] File updated time: " << std::ctime(&remoteUpdatedTime) 
            //           << "Current DB file time: " << std::ctime(&currentDBFileTime) << std::endl;
            
            // if (remoteUpdatedTime < currentDBFileTime) {
                std::cout << "[UPDATE_THREAD] New data available, updating..." << std::endl;
                webDownloadManager.setTimeLogging(true);
                bool res = webDownloadManager.downloadFile(url, file);
                if (!res) {
                    std::cout << "[UPDATE_THREAD] Failed to download file header from: " << url << std::endl;
                    continue;
                }
                std::cout << "[UPDATE_THREAD] Downloaded file header from: " << url << std::endl;
                callback(true);
            // } else {
            //     std::cout << "[UPDATE_THREAD] No new data available" << std::endl;
            // }
            
            for (int i = 0; i < periodInSeconds && !shouldStop; ++i) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        
        std::cout << "[UPDATE_THREAD] Update thread stopped" << endl;
        isUpdateThreadRunning = false;
    });
    
    updateThread.detach();
    
    std::cout << "Periodic update thread started successfully" << endl;
    return true;
}

bool MetaDataUpdater::initialize() {
    std::cout << "[MetaDataUpdater] initialize called" << endl;
    return true;
}

bool MetaDataUpdater::reset() {
    std::cout << "[MetaDataUpdater] reset called" << endl;
    return true;
}

bool MetaDataUpdater::stop() {
    std::cout << "[MetaDataUpdater] stop called" << endl;
    
    shouldStop = true;
    
    if (isUpdateThreadRunning) {
        std::cout << "[MetaDataUpdater] Waiting for update thread to stop..." << endl;
        for (int i = 0; i < 50 && isUpdateThreadRunning; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        if (isUpdateThreadRunning) {
            std::cout << "[MetaDataUpdater] Warning: Update thread did not stop within timeout" << endl;
        } else {
            std::cout << "[MetaDataUpdater] Update thread stopped successfully" << endl;
        }
    }
    
    return true;
}

void MetaDataUpdater::setWebDownloadManager(const WebDownloadManager& manager) {
    webDownloadManager = manager;
}

WebDownloadManager MetaDataUpdater::getWebDownloadManager() const {
    return webDownloadManager;
}

bool MetaDataUpdater::isTimeLoggingEnabled() const {
    return webDownloadManager.isTimeLoggingEnabled();
}

void MetaDataUpdater::setTimeLogging(bool enable) {
    webDownloadManager.setTimeLogging(enable);
}

bool MetaDataUpdater::downloadFileHeader(const string &url, const string &outputPath) {
    std::cout << "[MetaDataUpdater] Downloading file header from: " << url << " to: " << outputPath << endl;
    return webDownloadManager.downloadFile(url, outputPath);
}

bool MetaDataUpdater::downloadOnBackground(const string &url, const string &outputPath) {
    std::cout << "[MetaDataUpdater] Starting background download from: " << url << " to: " << outputPath << endl;
    
    thread downloadThread([this, url, outputPath]() {
        bool result = webDownloadManager.downloadFile(url, outputPath, 
            [url, outputPath](bool success, const string& message) {
                if (success) {
                    cout << "[ASYNC] Background download completed successfully!" << endl;
                    cout << "[ASYNC] Downloaded: " << url << " -> " << outputPath << endl;
                    cout << "[ASYNC] Message: " << message << endl;
                } else {
                    cout << "[ASYNC] Background download failed!" << endl;
                    cout << "[ASYNC] URL: " << url << endl;
                    cout << "[ASYNC] Output Path: " << outputPath << endl;
                    cout << "[ASYNC] Error: " << message << endl;
                }
            });
        
        cout << "[ASYNC] Download thread completed for: " << url << endl;
    });
    
    // downloadThread.detach();
    
    cout << "[MetaDataUpdater] Background download thread started for: " << url << endl;
    return true;
}

future<bool> MetaDataUpdater::downloadOnBackgroundWithFuture(const string &url, const string &outputPath) {
    cout << "[MetaDataUpdater] Starting future-based background download from: " << url << " to: " << outputPath << endl;
    
    return std::async(std::launch::async, [this, url, outputPath]() -> bool {
        std::cout << "[FUTURE] Starting download in background thread..." << std::endl;
        
        bool result = webDownloadManager.downloadFile(url, outputPath, 
            [url, outputPath](bool success, const std::string& message) {
                if (success) {
                    std::cout << "[FUTURE] Download completed: " << url << std::endl;
                } else {
                    std::cout << "[FUTURE] Download failed: " << url << " - " << message << std::endl;
                }
            });
        
        std::cout << "[FUTURE] Download thread finished for: " << url << " (result: " << (result ? "success" : "failed") << ")" << std::endl;
        return result;
    });
}

std::vector<std::future<bool>> MetaDataUpdater::downloadMultipleOnBackground(
    const std::vector<std::pair<std::string, std::string>>& urlPathPairs) {
    
    std::cout << "[MetaDataUpdater] Starting parallel download of " << urlPathPairs.size() << " files..." << std::endl;
    
    std::vector<std::future<bool>> futures;
    futures.reserve(urlPathPairs.size());
    
    for (const auto& pair : urlPathPairs) {
        futures.push_back(downloadOnBackgroundWithFuture(pair.first, pair.second));
    }
    
    std::cout << "[MetaDataUpdater] All download threads started" << std::endl;
    return futures;
}

void MetaDataUpdater::waitForDownloadsCompletion(std::vector<std::future<bool>>& futures) {
    std::cout << "[MetaDataUpdater] Waiting for " << futures.size() << " downloads to complete..." << std::endl;
    
    int completedCount = 0;
    int successCount = 0;
    
    for (auto& future : futures) {
        try {
            bool result = future.get();
            completedCount++;
            if (result) {
                successCount++;
            }
            
            std::cout << "[MetaDataUpdater] Download " << completedCount << "/" << futures.size() 
                      << " completed. Success: " << (result ? "Yes" : "No") << std::endl;
        } catch (const std::exception& e) {
            std::cout << "Download " << completedCount << " threw exception: " << e.what() << std::endl;
            completedCount++;
        }
    }
    
    std::cout << "[MetaDataUpdater] All downloads completed. Success rate: " << successCount << "/" 
              << completedCount << " (" << (successCount * 100 / completedCount) << "%)" << std::endl;
}

void MetaDataUpdater::setUpdateCallback(const std::function<void()>& callback) {
    std::cout << "[MetaDataUpdater] Setting update callback function" << endl;
}

bool MetaDataUpdater::isUpdateRunning() const {
    return isUpdateThreadRunning.load();
}

std::time_t MetaDataUpdater::FileLastWriteTimeToTimeT(const std::string& filePath) {
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;


    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wfilepath = converter.from_bytes(filePath);


    if (!GetFileAttributesExW(wfilepath.c_str(), GetFileExInfoStandard, &fileInfo)) {
        std::cout << "[MetaDataUpdater] could not get a time " << filePath << std::endl;
        return std::time_t(0);
    }
    
     ULARGE_INTEGER ull;
     ull.LowPart = fileInfo.ftLastWriteTime.dwLowDateTime;
     ull.HighPart = fileInfo.ftLastWriteTime.dwHighDateTime;
    
     constexpr ULONGLONG EPOCH_DIFF = 116444736000000000ULL;
     ull.QuadPart -= EPOCH_DIFF;
    
     return static_cast<std::time_t>(ull.QuadPart / 10000000ULL);
}

void MetaDataUpdater::scheduleUpdate(int intervalMinutes) {
    std::cout << "[MetaDataUpdater] Scheduling updates every " << intervalMinutes << " minutes" << std::endl;
    
    if (intervalMinutes > 0) {
        std::cout << "[MetaDataUpdater] Update scheduling configured for " << intervalMinutes << " minute intervals" << std::endl;
    }
}

void MetaDataUpdater::stopUpdates() {
    std::cout << "[MetaDataUpdater] Stopping all update processes" << std::endl;
    
    if (stop()) {
        std::cout << "[MetaDataUpdater] Updates stopped successfully" << std::endl;
    } else {
        std::cout << "[MetaDataUpdater] Failed to stop updates" << std::endl;
    }
}

MetaDataUpdater::~MetaDataUpdater() {
    std::cout << "MetaDataUpdater::~MetaDataUpdater() - Starting destructor" << std::endl;
    
    // Stop the update thread safely
    if (isUpdateThreadRunning) {
        std::cout << "MetaDataUpdater::~MetaDataUpdater() - Stopping update thread..." << std::endl;
        shouldStop = true;
        
        // Wait for the thread to finish
        if (updateThread.joinable()) {
            updateThread.join();
            std::cout << "MetaDataUpdater::~MetaDataUpdater() - Update thread stopped successfully" << std::endl;
        }
        
        isUpdateThreadRunning = false;
    }
    
    std::cout << "MetaDataUpdater::~MetaDataUpdater() - Destructor completed" << std::endl;
}
