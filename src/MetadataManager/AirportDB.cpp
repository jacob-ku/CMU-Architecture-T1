#include "AirportDB.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <memory>
#include <mutex>
#include <chrono>
#include <csv2/reader.hpp>
using namespace std;

#define AIRPORT_MAX_NUM 40000

AirportDB::AirportDB() {
    isloaded = false;
    std::cout << "[MetadataManager] AirportDB instance created" << std::endl;
}

AirportDB::~AirportDB() {
    std::cout << "[MetadataManager] AirportDB destructor called" << std::endl;
    airportCodeMap.clear();
    lineDataMap.clear();
    isloaded = false;
    std::cout << "[MetadataManager] AirportDB destructor completed" << std::endl;
}



bool AirportDB::loadFromFile(const std::string& filePath) {
    std::cout << "[MetadataManager] Loading airport data from file: " << filePath << std::endl;
    
    try {
        csv2::Reader<csv2::delimiter<','>, csv2::quote_character<'"'>, csv2::first_row_is_header<true>> csv;
        
        if (!csv.mmap(filePath)) {
            std::cout << "[MetadataManager] Error: Cannot open file - " << filePath << std::endl;
            return false;
        }
        
        const auto header = csv.header();
        
        for (const auto row : csv) {
            if (row.length() < 9) {
                std::cout << "[MetadataManager] Warning: Skipping row with insufficient columns" << std::endl;
                continue;
            }
            
            try {
                std::string code, name, icao, iata, location, countryISO2;
                float latitude, longitude;
                int altitude;
                
                for (auto cell : row) {
                    std::string value;
                }
            }
            catch (const std::exception& e) {
                std::cout << "Warning: Failed to parse row - " << e.what() << std::endl;
                continue;
            }
        }
        
        isloaded = true;
        std::cout << "Finished reading airport data from file. Loaded " << airportCodeMap.size() << " airports." << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cout << "Error: Exception occurred while parsing CSV file - " << e.what() << std::endl;
        return false;
    }
    catch (...) {
        std::cout << "Error: Unknown exception occurred while parsing CSV file" << std::endl;
        return false;
    }
}

std::vector<std::string> AirportDB::parseCSVLine(const std::string& line) {
    std::vector<std::string> fields;
    
    try {
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
    catch (const std::exception& e) {
        std::cout << "Error parsing CSV line: " << e.what() << std::endl;
        std::stringstream ss(line);
        std::string field;
        while (std::getline(ss, field, ',')) {
            fields.push_back(field);
        }
    }
    
    return fields;
}

bool AirportDB::parseAirportFromCSVLine(const std::string& csvLine, Airport& airport) {
    auto fields = parseCSVLine(csvLine);
    
    if (fields.size() < 9) {
        std::cout << "Error: CSV line has insufficient fields (" << fields.size() << " < 9)" << std::endl;
        return false;
    }
    
    try {
        std::string code = fields[0];
        std::string name = fields[1];
        std::string icao = fields[2];
        std::string iata = fields[3];
        std::string location = fields[4];
        std::string countryISO2 = fields[5];
        float latitude = std::stof(fields[6]);
        float longitude = std::stof(fields[7]);
        int altitude = std::stoi(fields[8]);
        
        airport = Airport(code, name, icao, iata, countryISO2, latitude, longitude, altitude);
        return true;
    }
    catch (const std::exception& e) {
        std::cout << "Error parsing CSV line values: " << e.what() << std::endl;
        return false;
    }
}

Airport AirportDB::getAirportByCode(const std::string& code) {
    std::cout << "Searching for airport with code: " << code << std::endl;

    if (airportCodeMap.find(code) == airportCodeMap.end()) {
        throw std::runtime_error("Airport not found with code: " + code);

    }
    return airportCodeMap[code];
   
    
}

Airport AirportDB::getAirportByICAO(const std::string& icao) {
    std::cout << "Searching for airport with ICAO: " << icao << std::endl;
    for (const auto& pair : airportCodeMap) {
        if (pair.second.getICAO() == icao) {
            return pair.second;
        }
    }
    throw std::runtime_error("Airport not found with ICAO: " + icao);
}

bool AirportDB::loadFromFileByLine(const std::string& filePath) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::string csvpath = filePath;
    if (filePath.empty()) {
        csvpath = "airports.csv";
    }
    
    std::cout << "Loading airport data line by line from file: " << csvpath << std::endl;
    
    std::ifstream file(csvpath);
    if (!file.is_open()) {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cerr << "Error: Cannot open file " << csvpath << std::endl;
        std::cerr << "Failed to open file after " << duration.count() << " milliseconds" << std::endl;
        return false;
    }
    
    try {
        lineDataMap.clear();
        
        std::string line;
        bool isFirstLine = true;
        
        while (std::getline(file, line)) {
            if (isFirstLine) {
                isFirstLine = false;
                std::cout << "Skipping header line: " << line << std::endl;
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
                
                lineDataMap[key] = line;
                
            } else {
                std::cout << "Warning: No comma found in line: " << line << std::endl;
            }
        }
        
        file.close();
        isloaded = true;
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "Finished reading file line by line. Total lines stored: " << lineDataMap.size() << std::endl;
        std::cout << "Loading completed in " << duration.count() << " milliseconds" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cerr << "Error while reading file: " << e.what() << std::endl;
        std::cerr << "Error occurred after " << duration.count() << " milliseconds" << std::endl;
        file.close();
        return false;
    }
}

std::string AirportDB::getAirportLineByCode(const std::string& code) {
    std::cout << "Getting raw CSV line for airport code: " << code << std::endl;
    
    auto it = lineDataMap.find(code);
    if (it != lineDataMap.end()) {
        return it->second;
    }
    
    throw std::runtime_error("Airport line not found with code: " + code);
}

bool AirportDB::parseLineDataToAirportMap() {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::cout << "Parsing lineDataMap entries to populate airportCodeMap..." << std::endl;
    
    if (lineDataMap.empty()) {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "Warning: lineDataMap is empty. Call loadFromFileByLine first." << std::endl;
        std::cout << "Check completed in " << duration.count() << " milliseconds" << std::endl;
        return false;
    }
    
    airportCodeMap.clear(); // Clear existing data
    
    int successCount = 0;
    int failCount = 0;
    
    for (const auto& pair : lineDataMap) {
        const std::string& code = pair.first;
        const std::string& csvLine = pair.second;
        
        Airport airport;
        if (parseAirportFromCSVLine(csvLine, airport)) {
            airportCodeMap[code] = airport;
            successCount++;
        } else {
            std::cout << "Warning: Failed to parse line for code '" << code << "': " << csvLine << std::endl;
            failCount++;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Parsing completed. Success: " << successCount 
              << ", Failed: " << failCount 
              << ", Total: " << (successCount + failCount) << std::endl;
    std::cout << "Parsing completed in " << duration.count() << " milliseconds" << std::endl;
    
    return failCount == 0; // Return true only if all lines were parsed successfully
}

