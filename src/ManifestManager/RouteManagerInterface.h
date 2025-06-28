#ifndef ROUTEMANAGERINTERFACE_H
#define ROUTEMANAGERINTERFACE_H 
#include <string>
#include "Route.h"

interface RouteManagerInterface
{
    public:
    virtual ~RouteManagerInterface() = default;

    // Method to load a route from a file
    virtual bool LoadRouteFromFile(const std::string& filename) = 0;
  
    // Method to get the current route as a string
    virtual Route& GetRoute(std::string& callsign) = 0;

};



#endif // ROUTEMANAGERINTERFACE_H