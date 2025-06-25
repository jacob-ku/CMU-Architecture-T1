#ifndef AIRPORTDB_H
#define AIRPORTDB_H

#include <iostream>
#include "Airport.h"
#include <string>
#include <mutex>
#include <memory>
#include <unordered_map>
#include "csv.h"

class AirportDB {
    
    private:
    AirportDB();

    std::unordered_map<std::string, Airport> airportCodeMap;
    bool isloaded;
    
    static std::unique_ptr<AirportDB> instance;
    static std::mutex mtx;


    public:
    static AirportDB* getInstance() {
   
        std::lock_guard<std::mutex> lock(mtx); // 멀티스레드 안전성 확보
        if (instance == nullptr) {
            instance = std::unique_ptr<AirportDB>(new AirportDB());
        }
        return instance.get();
    
    }

    static void destroyInstance() {
        // Singleton instance will be destroyed automatically when the program exits
    }

    bool loadFromFile (const std::string& filePath);
    
    Airport& getAirportByCode(const std::string& code);
    Airport& getAirportByICAO(const std::string& icao);
    
    std::unordered_map<std::string, Airport>& getAirportCodeMap() {
        return airportCodeMap;
    }

    bool isLoaded() const {
        return isloaded;
    }

};

#endif // AIRPORTDB_H