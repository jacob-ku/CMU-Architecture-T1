#include "AirportDB.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <memory>
#include <mutex>
using namespace std;

#define AIRPORT_MAX_NUM 40000

std::unique_ptr<AirportDB> AirportDB::instance = nullptr;
std::mutex AirportDB::mtx;

AirportDB::AirportDB() {

    isloaded = false;
    std::cout << "AirportDB instance created" << std::endl;
}


bool AirportDB::loadFromFile(const std::string& filePath) {
    std::cout << "Loading airport data from file: " << filePath << std::endl;
      try {
        io::CSVReader<9, io::trim_chars<>, io::double_quote_escape<',','\"'>> in(filePath);

        in.read_header(io::ignore_extra_column, "Code","Name","ICAO","IATA","Location","CountryISO2","Latitude","Longitude","AltitudeFeet");
        std::string code, name, icao, iata, location, countryISO2;
        float latitude, longitude;
        int altitude;

        while(in.read_row(code, name, icao, iata, location, countryISO2, latitude, longitude, altitude)) {
            airportCodeMap[code] = Airport(code, name, icao, iata, countryISO2, latitude, longitude, altitude);
        }
        std::cout << "Finished reading airport data from file." << std::endl;
    }
    catch (const io::error::can_not_open_file& e) {
        std::cout << "Error: Cannot open file - " << e.what() << std::endl;
        return false;
    }
    catch (const io::error::line_length_limit_exceeded& e) {
        std::cout << "Error: Line too long - " << e.what() << std::endl;
        return false;
    }
    catch (const io::error::missing_column_in_header& e) {
        std::cout << "Error: Missing column in header - " << e.what() << std::endl;
        return false;
    }
    catch (const io::error::extra_column_in_header& e) {
        std::cout << "Error: Extra column in header - " << e.what() << std::endl;
        return false;
    }
    catch (const io::error::duplicated_column_in_header& e) {
        std::cout << "Error: Duplicated column in header - " << e.what() << std::endl;
        return false;
    }
    catch (const std::exception& e) {
        std::cout << "Error: Unexpected exception - " << e.what() << std::endl;
        return false;
    }
    catch (...) {
        std::cout << "Error: Unknown exception occurred while parsing CSV file" << std::endl;
        return false;
    }
    return true;
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
