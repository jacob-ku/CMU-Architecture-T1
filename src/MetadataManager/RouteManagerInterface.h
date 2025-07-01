#ifndef ROUTEMANAGERINTERFACE_H
#define ROUTEMANAGERINTERFACE_H 

#include <string>
#include <unordered_map>
#include "Route.h"

interface RouteManagerInterface
{
public:
    virtual ~RouteManagerInterface() = default;

    virtual bool LoadRouteFromFile(const std::string& filename = "") = 0;
    virtual std::unordered_map<std::string, Route>& getRouteCallSignMap() = 0;
    virtual const Route* getRouteByCallSign(const std::string& callSign) = 0;
};

#endif // ROUTEMANAGERINTERFACE_H