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
    AirportDB();

    std::unordered_map<std::string, Airport> airportCodeMap;
    std::unordered_map<std::string, std::string> lineDataMap;
    bool isloaded;
    
    static std::unique_ptr<AirportDB> instance;
    static std::mutex mtx;


    public:
    static AirportDB* getInstance() {
   
        std::lock_guard<std::mutex> lock(mtx);
        if (instance == nullptr) {
            instance = std::unique_ptr<AirportDB>(new AirportDB());
        }
        return instance.get();
    
    }

    static void destroyInstance() {
    }

    bool loadFromFile (const std::string& filePath);
    bool loadFromFileByLine(const std::string& filePath);
    
    // CSV line parsing utilities using csv2
    std::vector<std::string> parseCSVLine(const std::string& line);
    bool parseAirportFromCSVLine(const std::string& csvLine, Airport& airport);

    Airport getAirportByCode(const std::string& code);
    Airport getAirportByICAO(const std::string& icao);
    
    // Get raw CSV line by airport code (for loadFromFileByLine data)
    std::string getAirportLineByCode(const std::string& code);
    
    // Parse all lineDataMap entries and populate airportCodeMap
    bool parseLineDataToAirportMap();

    std::unordered_map<std::string, Airport>& getAirportCodeMap() {
        return airportCodeMap;
    }

    bool isLoaded() const {
        return isloaded;
    }

};

#endif // AIRPORTDB_H