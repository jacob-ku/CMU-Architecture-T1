#ifndef METADATAUPDATERINTERFACE_H
#define METADATAUPDATERINTERFACE_H

#include <string>
#include <future>
#include <vector>
#include <ctime>
#include <thread>
#include <functional>
#include <atomic>
#include <chrono>
#include "Util/WebDownloadManager.h"
#include "DataUpdaterInterface.h"


struct DownloadStatus {
    std::string url;
    std::string outputPath;
    bool isComplete;
    bool isSuccessful;
    std::string statusMessage;
    
    DownloadStatus(const std::string& u, const std::string& o) 
        : url(u), outputPath(o), isComplete(false), isSuccessful(false) {}
};

class MetaDataUpdater : public DataUpdaterInterface {
private:
    WebDownloadManager webDownloadManager;
    
    std::thread updateThread;
    std::atomic<bool> isUpdateThreadRunning{false};
    std::atomic<bool> shouldStop{false};
    std::string updateSource;
    std::string targetFile;
    
public: 
    MetaDataUpdater() = default;
    MetaDataUpdater(const std::string& sourceUrl, const std::string& targetFile) 
        : updateSource(sourceUrl), targetFile(targetFile) { 
    }
    
    bool update(const std::string& src, const std::function<void(bool)>& callback) override;
    bool initialize() override;
    bool reset() override;
    bool stop() override;
    
    void scheduleUpdate(int intervalMinutes) override;
    void stopUpdates() override;
    ~MetaDataUpdater();
    
    void setWebDownloadManager(const WebDownloadManager& manager);
    WebDownloadManager getWebDownloadManager() const;
    bool isTimeLoggingEnabled() const;
    void setTimeLogging(bool enable);
    bool downloadFileHeader(const std::string &url, const std::string &outputPath);
    bool downloadOnBackground(const std::string &url, const std::string &outputPath);


    std::future<bool> downloadOnBackgroundWithFuture(const std::string &url, const std::string &outputPath);
    std::vector<std::future<bool>> downloadMultipleOnBackground(
        const std::vector<std::pair<std::string, std::string>>& urlPathPairs);
    
    void waitForDownloadsCompletion(std::vector<std::future<bool>>& futures);
    std::time_t FileLastWriteTimeToTimeT(const std::string& filePath);
    void setUpdateCallback(const std::function<void()>& callback);
	bool isUpdateRunning() const;

};

#endif