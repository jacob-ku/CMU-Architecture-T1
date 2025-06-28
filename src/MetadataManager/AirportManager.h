#ifndef AIRPORTMANAGER_H
#define AIRPORTMANAGER_H

#include "AirportDB.h"
#include "AirportManagerInterface.h"

class AirportManager : public AirportManagerInterface {
    private:
        AirportDB* airportDB;

    public:
        AirportManager();
        ~AirportManager();
        
        bool LoadAirport(const std::string& sourcefile = "") override;
        std::unordered_map<std::string, Airport>& getAirportCodeMap() override;
        Airport getAirportByCode(const std::string& code) override;
        std::vector<std::string> getAirportList() const override;
};

#endif