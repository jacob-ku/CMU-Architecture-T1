#ifndef AIRPORTDB_H
#define AIRPORTDB_H

#define AIRPORT_TARGET_FILE "airports.csv"
#define AIRPORT_MAX_NUM 40000

#include <unordered_map>
#include <string>
#include <vector>
#include "Airport.h"

class AirportDB {
private:
    std::unordered_map<std::string, Airport> airportCodeMap;

public:
    AirportDB();  // public 생성자
    ~AirportDB(); // 소멸자 추가

    bool loadFromFile(const std::string& filePath);
    std::unordered_map<std::string, Airport>& getAirportCodeMap();
    const Airport* getAirportByCode(const std::string& code);
    const Airport* getAirportByICAO(const std::string& icao);

private:
    void parseCSVLine(const std::string& line, std::vector<std::string>& fields);
    Airport* parseAirportFromCSVLine(const std::string& csvLine);
};

#endif // AIRPORTDB_H