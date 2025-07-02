#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <memory>
#include <mutex>
#include <chrono>
#include <csv2/reader.hpp>

#include "AirportDB.h"

AirportDB::AirportDB() {
    airportCodeMap.reserve(AIRPORT_MAX_NUM); // Reserve space for maximum number of airports
    // std::cout << "[AirportDB] Ctor: done" << std::endl;
}

AirportDB::~AirportDB() {
    // iterate all elements in the map and delete them
    for (auto& pair : airportCodeMap) {
        delete &(pair.second); // releae dynamically allocated Airport
    }
    airportCodeMap.clear();
    // std::cout << "[AirportDB] Dtor: done" << std::endl;
}

bool AirportDB::loadFromFile(const std::string& filePath) {
    std::string csvpath = (filePath.empty() ? AIRPORT_TARGET_FILE : filePath);
    std::cout << "[AirportDB] Loading airport data from file: " << csvpath << std::endl;
    
    std::ifstream file(csvpath);
    if (!file.is_open()) {
        std::cerr << "[AirportDB] Error: Cannot open file " << csvpath << std::endl;
        return false;
    }
    
    try {
        airportCodeMap.clear();
        
        std::string line;
        bool isFirstLine = true;
        
        while (std::getline(file, line)) {
            if (isFirstLine) {
                isFirstLine = false;
                std::cout << "[AirportDB] Skipping header line: " << line << std::endl;
                continue;
            }

            if (line.empty()) {
                continue;
            }

            size_t firstCommaPos = line.find(',');
            if (firstCommaPos != std::string::npos) {
                std::string key = line.substr(0, firstCommaPos);

                key.erase(0, key.find_first_not_of(" \t\r\n\""));
                key.erase(key.find_last_not_of(" \t\r\n\"") + 1);

                Airport* airport = parseAirportFromCSVLine(line);
                if (airport != nullptr) {
                    airportCodeMap[key] = *airport;
                }
            } else {
                // std::cout << "[AirportDB] Warning: No comma found in line: " << line << std::endl;
            }
        }
        file.close();
        
        std::cout << "[AirportDB] Finished reading file. Total airports loaded : " << airportCodeMap.size() << std::endl;

        return true;

    } catch (const std::exception& e) {
        std::cerr << "[AirportDB] Error while reading file: " << e.what() << std::endl;
        file.close();
        return false;
    }
}

std::unordered_map<std::string, Airport>& AirportDB::getAirportCodeMap() {
    return airportCodeMap;
}

const Airport* AirportDB::getAirportByCode(const std::string& code) {
    // std::cout << "[AirportDB] Searching for airport with code: " << code << std::endl;

    if (airportCodeMap.find(code) == airportCodeMap.end()) {
        return nullptr; // Return nullptr if not found

    }
    return &airportCodeMap[code];
   
    
}

const Airport* AirportDB::getAirportByICAO(const std::string& icao) {
    // std::cout << "[AirportDB] Searching for airport with ICAO: " << icao << std::endl;
    for (const auto& pair : airportCodeMap) {
        if (pair.second.getICAO() == icao) {
            return &(pair.second);
        }
    }
    return nullptr; // Return nullptr if not found
}

void AirportDB::parseCSVLine(const std::string& line, std::vector<std::string>& fields) {
    csv2::Reader<csv2::delimiter<','>, csv2::quote_character<'"'>, csv2::first_row_is_header<false>> csv;
    csv.parse(line);

    for (const auto row : csv) {
        for (const auto cell : row) {
            std::string value;
            cell.read_value(value);
            fields.push_back(value);
        }
        break; // Only process the first (and only) row
    }
}

Airport* AirportDB::parseAirportFromCSVLine(const std::string& csvLine) {
    std::vector<std::string> fields;
    try {
        parseCSVLine(csvLine, fields);
        
        if (fields.size() < 9) {
            std::cout << "[AirportDB] Error: CSV line has insufficient fields (" << fields.size() << " < 9)" << std::endl;
            return nullptr;
        }

        std::string code = fields[0];
        std::string name = fields[1];
        std::string icao = fields[2];
        std::string iata = fields[3];
        std::string location = fields[4];
        std::string countryISO2 = fields[5];
        float latitude = std::stof(fields[6]);
        float longitude = std::stof(fields[7]);
        int altitude = std::stoi(fields[8]);

        return new Airport(code, name, icao, iata, countryISO2, latitude, longitude, altitude);
    }
    catch (const std::exception& e) {
        std::cout << "[AirportDB] Error parsing CSV line values: " << e.what() << std::endl;
        return nullptr;
    }
}