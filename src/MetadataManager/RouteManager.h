#ifndef ROUTEMANAGER_H
#define ROUTEMANAGER_H

#include "RouteManagerInterface.h"
#include "RouteDB.h"

class RouteManager : public RouteManagerInterface {
private:
    RouteDB* routeDB;

public:
    RouteManager();
    ~RouteManager();

    bool LoadRouteFromFile(const std::string& filename = "") override;
    std::unordered_map<std::string, Route>& getRouteCallSignMap() override;
    const Route* getRouteByCallSign(const std::string& callSign) override;

    bool startUpdateMonitor();
};

#endif  // ROUTEMANAGER_H
