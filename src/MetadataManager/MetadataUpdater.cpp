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


bool MetaDataUpdater::update(const string& src, const std::function<void(bool)>& fileUnavailableCallback) {
    std::cout << "MetaDataUpdater::update called with src: " << src << std::endl;
    
    if (isUpdateThreadRunning) {
        std::cout << "Update thread is already running" << std::endl;
        return true;
    }
    
    updateSource = src;
    
    if (fileUnavailableCallback) {
        std::cout << "Update callback function set from parameter" << std::endl;
    } else {
        std::cout << "No callback function provided" << std::endl;
    }
    
    isUpdateThreadRunning = true;
    shouldStop = false;
    
    updateThread = std::thread([this, src, fileUnavailableCallback]() {
        std::cout << "[UPDATE_THREAD] Starting periodic update thread..." << std::endl;
        
        auto lastCallbackTime = std::chrono::steady_clock::now();
        const auto tenMinutes = std::chrono::minutes(10);
        const auto oneHour = std::chrono::hours(1);
        
        while (!shouldStop) {
            auto currentTime = std::chrono::steady_clock::now();
            
            std::cout << "[UPDATE_THREAD] Performing 10-minute check..." << std::endl;
            std::time_t fileUpdatedTime = webDownloadManager.getLastModifiedTime(src);
            std::time_t currentDBFileTime = FileLastWriteTimeToTimeT("routes.csv");
            std::cout << "[UPDATE_THREAD] File updated time: " << std::ctime(&fileUpdatedTime) 
                      << "Current DB file time: " << std::ctime(&currentDBFileTime) << std::endl;
            
            if (fileUpdatedTime > currentDBFileTime) {
                std::cout << "[UPDATE_THREAD] New data available, updating..." << std::endl;
                fileUnavailableCallback(false);
                webDownloadManager.setTimeLogging(true);
                bool res = webDownloadManager.downloadFile("https://vrs-standing-data.adsb.lol/routes.csv", "routes.csv");
                if (!res) {
                    std::cout << "[UPDATE_THREAD] Failed to download file header from: " << src << std::endl;
                    continue;
                }
                std::cout << "[UPDATE_THREAD] Downloaded file header from: " << src << std::endl;
                fileUnavailableCallback(true);
            } else {
                fileUnavailableCallback(true);
                std::cout << "[UPDATE_THREAD] No new data available" << std::endl;
            }
            
            for (int i = 0; i < 2 && !shouldStop; ++i) {
                std::this_thread::sleep_for(std::chrono::seconds(20));
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
    std::cout << "MetaDataUpdater::initialize called" << endl;
    return true;
}

bool MetaDataUpdater::reset() {
    std::cout << "MetaDataUpdater::reset called" << endl;
    return true;
}

bool MetaDataUpdater::stop() {
    std::cout << "MetaDataUpdater::stop called" << endl;
    
    shouldStop = true;
    
    if (isUpdateThreadRunning) {
        std::cout << "Waiting for update thread to stop..." << endl;
        for (int i = 0; i < 50 && isUpdateThreadRunning; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        if (isUpdateThreadRunning) {
            std::cout << "Warning: Update thread did not stop within timeout" << endl;
        } else {
            std::cout << "Update thread stopped successfully" << endl;
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
    std::cout << "Downloading file header from: " << url << " to: " << outputPath << endl;
    return webDownloadManager.downloadFile(url, outputPath);
}

bool MetaDataUpdater::downloadOnBackground(const string &url, const string &outputPath) {
    std::cout << "Starting background download from: " << url << " to: " << outputPath << endl;
    
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
    
    downloadThread.detach();
    
    cout << "Background download thread started for: " << url << endl;
    return true;
}

future<bool> MetaDataUpdater::downloadOnBackgroundWithFuture(const string &url, const string &outputPath) {
    cout << "Starting future-based background download from: " << url << " to: " << outputPath << endl;
    
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
    
    std::cout << "Starting parallel download of " << urlPathPairs.size() << " files..." << std::endl;
    
    std::vector<std::future<bool>> futures;
    futures.reserve(urlPathPairs.size());
    
    for (const auto& pair : urlPathPairs) {
        futures.push_back(downloadOnBackgroundWithFuture(pair.first, pair.second));
    }
    
    std::cout << "All download threads started" << std::endl;
    return futures;
}

void MetaDataUpdater::waitForDownloadsCompletion(std::vector<std::future<bool>>& futures) {
    std::cout << "Waiting for " << futures.size() << " downloads to complete..." << std::endl;
    
    int completedCount = 0;
    int successCount = 0;
    
    for (auto& future : futures) {
        try {
            bool result = future.get();
            completedCount++;
            if (result) {
                successCount++;
            }
            
            std::cout << "Download " << completedCount << "/" << futures.size() 
                      << " completed. Success: " << (result ? "Yes" : "No") << std::endl;
        } catch (const std::exception& e) {
            std::cout << "Download " << completedCount << " threw exception: " << e.what() << std::endl;
            completedCount++;
        }
    }
    
    std::cout << "All downloads completed. Success rate: " << successCount << "/" 
              << completedCount << " (" << (successCount * 100 / completedCount) << "%)" << std::endl;
}

void MetaDataUpdater::setUpdateCallback(const std::function<void()>& callback) {
    std::cout << "Setting update callback function" << endl;
}

bool MetaDataUpdater::isUpdateRunning() const {
    return isUpdateThreadRunning.load();
}

std::time_t MetaDataUpdater::FileLastWriteTimeToTimeT(const std::string& filePath) {
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;


    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wfilepath = converter.from_bytes(filePath);


    if (!GetFileAttributesExW(wfilepath.c_str(), GetFileExInfoStandard, &fileInfo)) {
        std::cout << "could not get a time " << filePath << std::endl;
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
    std::cout << "Scheduling updates every " << intervalMinutes << " minutes" << std::endl;
    
    if (intervalMinutes > 0) {
        std::cout << "Update scheduling configured for " << intervalMinutes << " minute intervals" << std::endl;
    }
}

void MetaDataUpdater::stopUpdates() {
    std::cout << "Stopping all update processes" << std::endl;
    
    if (stop()) {
        std::cout << "Updates stopped successfully" << std::endl;
    } else {
        std::cout << "Failed to stop updates" << std::endl;
    }
}
