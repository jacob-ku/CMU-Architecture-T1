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
    // 기본 생성자는 private
    RouteDB();
    
    // 의존성 주입을 위한 생성자
    RouteDB(std::unique_ptr<DataUpdaterInterface> updater);
    
    std::unordered_map<std::string, Route> routeMap;
    std::unordered_map<std::string, std::string> lineDataMap; // 한줄씩 읽은 데이터 저장용
    static std::unique_ptr<RouteDB> instance;
    static std::mutex mtx;
    bool isLoaded = false;
    std::unique_ptr<DataUpdaterInterface> updater;
    
    // 업데이트 완료 콜백 함수 (결합도 낮추기 위해 분리)
    void onUpdateComplete(bool updateStatus);

public:
    static RouteDB* getInstance() {
        std::lock_guard<std::mutex> lock(mtx); // 멀티스레드 안전성 확보
        if (instance == nullptr) {
            instance = std::unique_ptr<RouteDB>(new RouteDB());
        }
        return instance.get();
    }
    
    // 의존성 주입을 위한 정적 팩토리 메서드
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