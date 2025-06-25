#include "RouteManager.h"
#include "RouteDB.h"
#include "Route.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <chrono>

// Constructor implementation
RouteManager::RouteManager() {
    routeDB = RouteDB::getInstance(); // Get the singleton instance of RouteDB
    // Initialize other members if needed
}
RouteManager::~RouteManager() {
    RouteDB::destroyInstance(); // Clean up the singleton instance
}
// Static member to hold the route data - initialized with explicit constructor

bool RouteManager::LoadRouteFromFile(const std::string& filename) {
    // 전체 실행시간 측정 시작
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::cout << "Loading route from file: " << filename << std::endl;
    
    try {
        bool result = routeDB->loadFromFileByLine(); // Load route data from the specified file
        
        // 전체 실행시간 측정 종료 및 결과 출력
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "LoadRouteFromFile completed in " << duration.count() << " milliseconds" << std::endl;
        std::cout << "Load result: " << (result ? "Success" : "Failed") << std::endl;
        
        return result;
    }
    catch (const std::exception& e) {
        // 예외 발생 시에도 실행시간 측정
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cerr << "Error in LoadRouteFromFile: " << e.what() << std::endl;
        std::cerr << "Failed after " << duration.count() << " milliseconds" << std::endl;
        return false;
    }
}

Route RouteManager::GetRoute(std::string& callsign) {
    std::cout << "Getting route for callsign: " << callsign << std::endl; 
    
    // RouteDB에서 직접 참조를 가져와서 반환
    Route serchRoute = routeDB->getRouteByCallsign(callsign);

    std::cout << "Route data retrieved for callsign: " << callsign << std::endl;
    std::cout << "Route: " << serchRoute.getCallsign() << ", "  
              << serchRoute.getCode() << ", " 
              << serchRoute.getNumber() << ", " 
              << serchRoute.getAirlineCode() << std::endl;

    return serchRoute;
}
