#ifndef ROUTEDB_H
#define ROUTEDB_H
#include <string>
#include <unordered_map>
#include "Route.h"
#include <memory>
#include <mutex>
#include "csv.h"
#include "DataUpdaterInterface.h"

#define SOURCE_URL "https://vrs-standing-data.adsb.lol/routes.csv"
#define TARGET_FILE "routes.csv"


class RouteDB {
private:
    RouteDB();
    
    RouteDB(std::unique_ptr<DataUpdaterInterface> updater);
    
    std::unordered_map<std::string, Route> routeMap;
    std::unordered_map<std::string, std::string> lineDataMap;
    static std::unique_ptr<RouteDB> instance;
    static std::mutex mtx;
    bool isLoaded = false;
    std::unique_ptr<DataUpdaterInterface> updater;
    
    void onUpdateComplete(bool updateStatus);

public:
    static RouteDB* getInstance() {
        std::lock_guard<std::mutex> lock(mtx);
        if (instance == nullptr) {
            instance = std::unique_ptr<RouteDB>(new RouteDB());
        }
        return instance.get();
    }
    
    static RouteDB* getInstance(std::unique_ptr<DataUpdaterInterface> updater) {
        std::lock_guard<std::mutex> lock(mtx);
        if (instance == nullptr) {
            instance = std::unique_ptr<RouteDB>(new RouteDB(std::move(updater)));
        }
        return instance.get();
    }
    
    static void destroyInstance() {
        std::lock_guard<std::mutex> lock(mtx);
        instance.reset();
    }
    
    bool loadFromFile(const std::string& filePath = "");
    bool loadFromFileByLine(const std::string& filePath = "");
    Route getRouteByCallsign(std::string& callsign);
    Route getRouteInfoOnWeb(std::string& callsign);

    bool nowloading();
    bool startUpdateMonitor();
};
        
#endif