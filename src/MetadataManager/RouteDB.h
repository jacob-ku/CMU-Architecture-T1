#ifndef ROUTEDB_H
#define ROUTEDB_H
#include <string>
#include <unordered_map>
#include "Route.h"
#include <memory>
#include <mutex>
#include "csv.h"



class RouteDB {
private:
    RouteDB() = default;
    std::unordered_map<std::string, Route> routeMap;
    std::unordered_map<std::string, std::string> lineDataMap; // 한줄씩 읽은 데이터 저장용
    static std::unique_ptr<RouteDB> instance;
    static std::mutex mtx;
    bool isLoaded = false;
public:
    static RouteDB* getInstance() {
        std::lock_guard<std::mutex> lock(mtx); // 멀티스레드 안전성 확보
        if (instance == nullptr) {
            instance = std::unique_ptr<RouteDB>(new RouteDB());
        }
        return instance.get();
    }
    static void destroyInstance() {
        // Singleton instance will be destroyed automatically when the program exits
    }
    bool loadFromFile(const std::string& filePath = "");
    bool loadFromFileByLine(const std::string& filePath = "");
    Route getRouteByCallsign(std::string& callsign);
    bool nowloading() {
        return isLoaded;
    }
};
        
#endif