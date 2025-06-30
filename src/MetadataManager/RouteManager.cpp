#include "RouteManager.h"
#include "RouteDB.h"
#include "Route.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <chrono>

RouteManager::RouteManager() {
    routeDB = new RouteDB();  // 직접 객체 생성
    std::cout << "RouteManager created with new RouteDB instance" << std::endl;
}
RouteManager::~RouteManager() {
    std::cout << "RouteManager destructor called" << std::endl;
    if (routeDB) {
        delete routeDB;  // 직접 객체 삭제
        routeDB = nullptr;
        std::cout << "RouteDB instance deleted" << std::endl;
    }
}

bool RouteManager::StartUpdateMonitor()
{
    return routeDB->startUpdateMonitor();
}



bool RouteManager::LoadRouteFromFile(const std::string& filename) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::cout << "Loading route from file: " << filename << std::endl;
    
    try {
        bool result = routeDB->loadFromFileByLine();
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "LoadRouteFromFile completed in " << duration.count() << " milliseconds" << std::endl;
        std::cout << "Load result: " << (result ? "Success" : "Failed") << std::endl;
        
        return result;
    }
    catch (const std::exception& e) {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cerr << "Error in LoadRouteFromFile: " << e.what() << std::endl;
        std::cerr << "Failed after " << duration.count() << " milliseconds" << std::endl;
        return false;
    }
}

Route RouteManager::GetRoute(std::string& callsign) {
    std::cout << "Getting route for callsign: " << callsign << std::endl; 
    
    // using local db first, and move to web if not found
    Route serchRoute = routeDB->getRouteByCallsign(callsign);

    std::cout << "Route data retrieved for callsign: " << callsign << std::endl;
    std::cout << "Route: " << serchRoute.getCallsign() << ", "  
              << serchRoute.getCode() << ", " 
              << serchRoute.getNumber() << ", " 
              << serchRoute.getAirlineCode();
              std::cout << "Waypoints: ";
              std::vector<std::string> waypoints = serchRoute.getWaypoints();
    for (const auto& wp : waypoints) {
        std::cout << wp << " ";
    }
    std::cout << std::endl;

    return serchRoute;
}
