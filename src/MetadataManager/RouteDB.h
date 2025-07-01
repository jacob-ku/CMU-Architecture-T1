#ifndef ROUTEDB_H
#define ROUTEDB_H

#define ROUTE_SOURCE_URL "https://vrs-standing-data.adsb.lol/routes.csv"
#define ROUTE_TARGET_FILE "routes.csv"
#define ROUTE_MAX_NUM 600000

#include <unordered_map>
#include <string>
#include <atomic>
#include "Route.h"
#include "DataUpdaterInterface.h"

class RouteDB {
private:
    std::unordered_map<std::string, Route>* routeMap;   // main db
    std::unordered_map<std::string, Route>* tempMap; // temporary used only for during update
    std::atomic_bool inSwap;
    std::unique_ptr<DataUpdaterInterface> updater;

public:
    RouteDB();  // public 기본 생성자
    RouteDB(std::unique_ptr<DataUpdaterInterface> updater);  // public 의존성 주입 생성자
    ~RouteDB(); // 소멸자 추가
    
    
    bool loadFromFile(const std::string& filePath, boolean isUpdate); // takes around 1 second
    // bool loadFromFileThroughCSVReader(const std::string& filePath);  // DO NOT USE THIS - took 30s
    std::unordered_map<std::string, Route>& getRouteCallSignMap();
    const Route* getRouteByCallsign(const std::string& callsign);

    bool startUpdateMonitor();

private:
    void updateMapBySwap();

    // const Route* getRouteInfoOnWeb(const std::string& callsign);     // Disable for now
    void onUpdateComplete(bool updateStatus);

    void parseCSVLine(const std::string& line, std::vector<std::string>& fields);
    Route* parseRouteFromCSVLine(const std::string& csvLine);
};
        
#endif  // ROUTEDB_H