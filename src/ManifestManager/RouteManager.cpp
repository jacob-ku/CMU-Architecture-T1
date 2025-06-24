#include "RouteManager.h"
#include <fstream>
#include <iostream>
#include <vector>

// Static member to hold the route data - initialized with explicit constructor

bool RouteManager::LoadRouteFromFile(const std::string& filename) {
    std::cout << "Loading route from file: " << filename << std::endl;
    return true;
}

Route& RouteManager::GetRoute(std::string& callsign) {
    std::cout << "Getting route for callsign: " << callsign << std::endl;
    std::vector<std::string> waypoints = {"KONT", "KGSO", "KPSP"}; // Example waypoints, replace with actual logic to parse routeData

    Route routeData("AAC710","AAC","710","AAC", waypoints);

    dummy = routeData; // Assign to static member for demonstration purposes

    std::cout << "Route data retrieved for callsign: " << callsign << std::endl;
    std::cout << "Route: " << routeData.getCallsign() << ", "  
              << routeData.getCode() << ", " 
              << routeData.getNumber() << ", " 
              << routeData.getAirlineCode() << std::endl;
    return dummy;
}
