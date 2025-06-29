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
    std::unordered_map<std::string, Route> routeMap;
    std::unordered_map<std::string, std::string> lineDataMap;
    bool isLoaded = false;
    std::unique_ptr<DataUpdaterInterface> updater;
    
    void onUpdateComplete(bool updateStatus);

public:
    RouteDB();  // public 기본 생성자
    RouteDB(std::unique_ptr<DataUpdaterInterface> updater);  // public 의존성 주입 생성자
    ~RouteDB(); // 소멸자 추가
    
    bool loadFromFile(const std::string& filePath = "");
    bool loadFromFileByLine(const std::string& filePath = "");
    Route getRouteByCallsign(std::string& callsign);
    Route getRouteInfoOnWeb(std::string& callsign);

    bool LodingStatus();
    bool startUpdateMonitor();
};
        
#endif