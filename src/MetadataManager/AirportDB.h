#ifndef AIRPORTDB_H
#define AIRPORTDB_H

#include <iostream>
#include "Airport.h"
#include <string>
#include <mutex>
#include <memory>
#include <unordered_map>
#include <vector>
#include <csv2/reader.hpp>

class AirportDB {
    
    private:
    std::unordered_map<std::string, Airport> airportCodeMap;
    std::unordered_map<std::string, std::string> lineDataMap;
    bool isloaded;

    public:
    AirportDB();  // public 생성자
    ~AirportDB(); // 소멸자 추가

    bool loadFromFile (const std::string& filePath);
    bool loadFromFileByLine(const std::string& filePath);
    
    std::vector<std::string> parseCSVLine(const std::string& line);
    bool parseAirportFromCSVLine(const std::string& csvLine, Airport& airport);

    Airport getAirportByCode(const std::string& code);
    Airport getAirportByICAO(const std::string& icao);
    
    std::string getAirportLineByCode(const std::string& code);
    
    bool parseLineDataToAirportMap();

    std::unordered_map<std::string, Airport>& getAirportCodeMap() {
        return airportCodeMap;
    }

    bool isLoaded() const {
        return isloaded;
    }

};

#endif // AIRPORTDB_H