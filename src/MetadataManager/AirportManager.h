#ifndef AIRPORTMANAGER_H
#define AIRPORTMANAGER_H

#include "AirportManagerInterface.h"
#include "AirportDB.h"

class AirportManager : public AirportManagerInterface {
private:
    AirportDB* airportDB;

public:
    AirportManager();
    ~AirportManager();
    
    bool LoadAirportFromFile(const std::string& sourcefile = "") override;
    std::unordered_map<std::string, Airport>& getAirportCodeMap() override;
    const Airport* getAirportByCode(const std::string& code) override;
};

#endif  // AIRPORTMANAGER_H