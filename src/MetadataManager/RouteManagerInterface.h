#ifndef ROUTEMANAGERINTERFACE_H
#define ROUTEMANAGERINTERFACE_H 
#include <string>
#include "Route.h"

interface RouteManagerInterface
{
    public:
    virtual ~RouteManagerInterface() = default;

    virtual bool LoadRouteFromFile(const std::string& filename) = 0;
  
    virtual Route GetRoute(std::string& callsign) = 0;

};



#endif // ROUTEMANAGERINTERFACE_H