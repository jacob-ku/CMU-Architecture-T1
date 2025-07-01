#include "AirportManager.h"

#define ELAPSED_TIME_CHK    // enable macro for execution time measurement
#include "Util/ExecutionTimer.h"

AirportManager::AirportManager() {
    airportDB = new AirportDB();  // 직접 객체 생성
    std::cout << "[AirportManager] Ctor: create new AirportDB instance" << std::endl;
}

AirportManager::~AirportManager() {
    if (airportDB) {
        delete airportDB;  // 직접 객체 삭제
        airportDB = nullptr;
        std::cout << "[AirportManager] Dtor: AirportDB instance deleted" << std::endl;
    }
    std::cout << "[AirportManager] Dtor: done" << std::endl;
}

bool AirportManager::LoadAirportFromFile(const std::string& sourcefile) {

    EXECUTION_TIMER(fileLoadTime);
    bool result = airportDB->loadFromFile(sourcefile);
    EXECUTION_TIMER_ELAPSED(elapsedTime, fileLoadTime);

    std::cout << "[AirportManager] LoadAirportFromFile completed in " << elapsedTime << " milliseconds" << std::endl;

    return result;
}

std::unordered_map<std::string, Airport>& AirportManager::getAirportCodeMap() {
    return airportDB->getAirportCodeMap();
}

const Airport* AirportManager::getAirportByCode(const std::string& code) {
    return airportDB->getAirportByCode(code);
}