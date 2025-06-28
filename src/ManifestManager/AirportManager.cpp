#include "AirportManager.h"

AirportManager::AirportManager() {
    airportDB = AirportDB::getInstance();
    //airportDB->loadFromFile("airports.csv"); // Load dummy data or specify a real file
}

AirportManager::~AirportManager() {
    AirportDB::destroyInstance();
}

bool AirportManager::LoadAirport(const std::string& sourcefile) {
    std::string srcfile = sourcefile;
    if (srcfile == "")
    {
        srcfile = "airports.csv";   
    }
    return airportDB->loadFromFile(srcfile);
}

std::unordered_map<std::string, Airport>& AirportManager::getAirportCodeMap() {
    return airportDB->getAirportCodeMap();
}

Airport& AirportManager::getAirportByCode(const std::string& code) {
    return airportDB->getAirportByCode(code);
}

std::vector<std::string> AirportManager::getAirportList() const {
    std::vector<std::string> airportList;
    for (const auto& pair : airportDB->getAirportCodeMap()) {
        airportList.push_back(pair.first);
    }
    return airportList;
}
