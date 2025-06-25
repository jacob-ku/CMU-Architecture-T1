#include "AirportDB.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <memory>
#include <mutex>
using namespace std;

#define AIRPORT_MAX_NUM 40000

// Static 멤버 정의 및 초기화
std::unique_ptr<AirportDB> AirportDB::instance = nullptr;
std::mutex AirportDB::mtx;

//static int CSV_callback(struct CSV_context *ctx, const char *value)
//{
//    int rc = 1;
//    std::cout << "CSV callback called for field: " << ctx->field_num << ", value: " << value << std::endl;
//
//    return (rc);
//}

// Private 생성자
AirportDB::AirportDB() {
    // 해시테이블 초기화 등 필요한 초기화 작업

    isloaded = false;
    std::cout << "AirportDB instance created" << std::endl;
}

// Private 소멸자
//AirportDB::~AirportDB() {
//    std::cout << "AirportDB instance destroyed" << std::endl;
//}



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

Airport& AirportDB::getAirportByCode(const std::string& code) {
    std::cout << "Searching for airport with code: " << code << std::endl;
    
    // TODO: 해시테이블에서 검색
    // 임시 구현 - 실제로는 해시테이블에서 검색해야 함
    return airportCodeMap[code];
    // 찾지 못한 경우 예외 발생 (참조 반환이므로 nullptr 불가)
    
}

Airport& AirportDB::getAirportByICAO(const std::string& icao) {
    std::cout << "Searching for airport with ICAO: " << icao << std::endl;
    
    // TODO: 해시테이블에서 ICAO로 검색
    // 임시 구현 - 실제로는 해시테이블에서 검색해야 함
    
    // 찾지 못한 경우 예외 발생 (참조 반환이므로 nullptr 불가)
    throw std::runtime_error("Airport not found with ICAO: " + icao);
}
