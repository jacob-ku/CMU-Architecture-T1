#include "RouteManager.h"

#define ELAPSED_TIME_CHK    // enable macro for execution time measurement
#include "Util/ExecutionTimer.h"

RouteManager::RouteManager() {
    routeDB = new RouteDB();  // 직접 객체 생성
    std::cout << "[RouteManager] Ctor: created new RouteDB instance" << std::endl;
}
RouteManager::~RouteManager() {
    if (routeDB) {
        delete routeDB;  // 직접 객체 삭제
        routeDB = nullptr;
        std::cout << "[RouteManager] Dtor: RouteDB instance deleted" << std::endl;
    }
    std::cout << "[RouteManager] Dtor: done" << std::endl;
}

bool RouteManager::LoadRouteFromFile(const std::string& filename) {

    EXECUTION_TIMER(fileLoadTime);
    bool result = routeDB->loadFromFile(filename, false);    // took aroud 1 seconds
    // bool result = routeDB->loadFromFileThroughCSVReader(filename);   // took more than 30 seconds
    EXECUTION_TIMER_ELAPSED(elapsedTime, fileLoadTime);

    std::cout << "[RouteManager] LoadRouteFromFile completed in " << elapsedTime << " milliseconds" << std::endl;

    return result;
}

std::unordered_map<std::string, Route>& RouteManager::getRouteCallSignMap() {
    return routeDB->getRouteCallSignMap();
}

const Route* RouteManager::getRouteByCallSign(const std::string& callsign) {
    // TODO: comment out or delete
    // std::cout << "[RouteManager] getting route for callsign: " << callsign << std::endl; 
    
    // using local db first, and move to web if not found
    const Route* serchRoute = routeDB->getRouteByCallsign(callsign);

    // TODO: comment out or delete
    // std::cout << "[RouteManager] Route data retrieved for callsign: " << callsign << std::endl;
    // std::cout << "[RouteManager] Route: " << serchRoute->getCallsign() << ", "  
    //           << serchRoute->getCode() << ", " 
    //           << serchRoute->getNumber() << ", " 
    //           << serchRoute->getAirlineCode();
    //           std::cout << "Waypoints: ";
    //           std::vector<std::string> waypoints = serchRoute->getWaypoints();
    // for (const auto& wp : waypoints) {
    //     std::cout << wp << " ";
    // }
    // std::cout << std::endl;

    return serchRoute;
}

bool RouteManager::startUpdateMonitor()
{
    return routeDB->startUpdateMonitor();
}