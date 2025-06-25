#ifndef ROUTEMANAGER_H
#define ROUTEMANAGER_H

#include "RouteManagerInterface.h"
#include "Route.h"
#include <iostream>
#include <string>
#include <vector>

class RouteManager : public RouteManagerInterface {
    private:
        std::string routeData;
        Route dummy; // Static member to hold the route data
    public:
        RouteManager() = default;
        ~RouteManager() = default;

        // Method to load a route from a file
        bool LoadRouteFromFile(const std::string& filename) override;

        // Method to get the current route as a string
        Route& GetRoute(std::string& callsign) override;
};

#endif
