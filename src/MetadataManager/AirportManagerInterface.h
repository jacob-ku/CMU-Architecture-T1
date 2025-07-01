#ifndef AIRPORTMANAGERINTERFACE_H
#define AIRPORTMANAGERINTERFACE_H 

#include <string>
#include <unordered_map>
#include "Airport.h"

interface AirportManagerInterface
{
public:
    virtual ~AirportManagerInterface() = default;

    virtual bool LoadAirportFromFile(const std::string& sourcefile = "") = 0;
    virtual std::unordered_map<std::string, Airport>& getAirportCodeMap() = 0; 
    virtual const Airport* getAirportByCode(const std::string& code) = 0;
};

#endif // AIRPORTMANAGERINTERFACE_H