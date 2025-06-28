#ifndef ROUTEMANAGER_H
#define ROUTEMANAGER_H

#include "RouteManagerInterface.h"
#include "Route.h"
#include "RouteDB.h"
#include <iostream>
#include <string>
#include <vector>

class RouteManager : public RouteManagerInterface {
    private:
        RouteDB* routeDB;
    public:
        std::string routeData;
        Route dummy;

        RouteManager();
        ~RouteManager();

        bool LoadRouteFromFile(const std::string& filename = "") override;

        Route GetRoute(std::string& callsign) override;

        bool StartUpdateMonitor();
};

#endif
