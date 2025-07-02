#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <memory>
#include <csv2/reader.hpp>

#include "RouteDB.h"
#include "csv.h"
#include "../Util/WebDownloadManager.h"
#include "MetadataUpdater.h"

RouteDB::RouteDB() {
    routeMap = new std::unordered_map<std::string, Route>();
    routeMap->reserve(ROUTE_MAX_NUM); // Reserve space for maximum number of routes

    tempMap = new std::unordered_map<std::string, Route>();
    tempMap->reserve(ROUTE_MAX_NUM); // Reserve space for maximum number of routes

    inSwap.store(false);
    updater = std::make_unique<MetaDataUpdater>(ROUTE_SOURCE_URL, ROUTE_TARGET_FILE);
    std::cout << "[RouteDB] Ctor: done" << std::endl;
}

RouteDB::RouteDB(std::unique_ptr<DataUpdaterInterface> injectedUpdater) 
    : updater(std::move(injectedUpdater)) { // move ownership of unique_ptr for updater
        routeMap = new std::unordered_map<std::string, Route>();

    routeMap->reserve(ROUTE_MAX_NUM); // Reserve space for maximum number of routes

    tempMap = new std::unordered_map<std::string, Route>();
    tempMap->reserve(ROUTE_MAX_NUM); // Reserve space for maximum number of routes

    inSwap.store(false);
    std::cout << "[RouteDB] Ctor with updater injection: done" << std::endl;
}

RouteDB::~RouteDB() {
    updater->stop(); // Stop the updater if it's running

    // iterate all elements in the map and delete them
    for (auto& pair : *routeMap) {
        delete &(pair.second); // releae dynamically allocated Route
    }
    routeMap->clear();

    for (auto& pair : *tempMap) {
        delete &(pair.second); // releae dynamically allocated Route
    }
    tempMap->clear();

    std::cout << "[RouteDB] Dtor : done" << std::endl;
}

bool RouteDB::loadFromFile(const std::string& filePath, boolean isUpdate) {
    std::string csvpath = (filePath.empty() ? ROUTE_TARGET_FILE : filePath);
    std::cout << "[RouteDB] Loading airport data from file: " << csvpath << std::endl;

    std::unordered_map<std::string, Route>* targetMap = isUpdate ? tempMap : routeMap;
    
    std::ifstream file(csvpath);
    if (!file.is_open()) {
        std::cerr << "[RouteDB] Error: Cannot open file " << csvpath << std::endl;
        return false;
    }

    try {
        targetMap->clear();
        
        std::string line;
        bool isFirstLine = true;
        
        while (std::getline(file, line)) {
            if (isFirstLine) {
                isFirstLine = false;
                std::cout << "[RouteDB] Skipping header line: " << line << std::endl;
                continue;
            }

            if (line.empty()) {
                continue;
            }

            size_t firstCommaPos = line.find(',');
            if (firstCommaPos != std::string::npos) {
                std::string key = line.substr(0, firstCommaPos);
                
                key.erase(0, key.find_first_not_of(" \t\r\n"));
                key.erase(key.find_last_not_of(" \t\r\n") + 1);

                Route* route = parseRouteFromCSVLine(line);
                if (route != nullptr) {
                    (*targetMap)[key] = *route;
                }
            } else {
                std::cout << "[RouteDB] Warning: No comma found in line: " << line << std::endl;
            }
        }
        
        file.close();
        std::cout << "[RouteDB] Finished reading file line by line. Total lines stored: " << targetMap->size() << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[RouteDB] Error while reading file: " << e.what() << std::endl;
        file.close();
        return false;
    }
}

/*
bool RouteDB::loadFromFileThroughCSVReader(const std::string& filePath) {
    std::string csvpath = (filePath.empty() ? ROUTE_TARGET_FILE : filePath);
    std::cout << "[RouteDB] Loading route data from file: " << csvpath << std::endl;
    
    try {
        io::CSVReader<5, io::trim_chars<>, io::double_quote_escape<',','\"'>> in(csvpath);

        in.read_header(io::ignore_extra_column, "Callsign", "Code", "Number", "AirlineCode", "AirportCodes");
        std::string callsign, code, number, airlineCode, waypointsStr;

        while(in.read_row(callsign, code, number, airlineCode, waypointsStr)) {
            Route* route = new Route(callsign, code, number, airlineCode, waypointsStr);
            (*routeMap)[callsign] = *route;
        }

        std::cout << "[RouteDB] Finished reading route data from file. Total routes: " << routeMap->size() << std::endl;
        isLoaded.store(true);   // mark done

        return true;
    }
    catch (const io::error::can_not_open_file& e) {
        std::cerr << "[RouteDB] Error: Cannot open file - " << e.what() << std::endl;
        return false;
    }
    catch (const io::error::line_length_limit_exceeded& e) {
        std::cerr << "[RouteDB] Error: Line too long - " << e.what() << std::endl;
        return false;
    }
    catch (const io::error::missing_column_in_header& e) {
        std::cerr << "[RouteDB] Error: Missing column in header - " << e.what() << std::endl;
        return false;
    }
    catch (const io::error::extra_column_in_header& e) {
        std::cerr << "[RouteDB] Error: Extra column in header - " << e.what() << std::endl;
        return false;
    }
    catch (const io::error::duplicated_column_in_header& e) {
        std::cerr << "[RouteDB] Error: Duplicated column in header - " << e.what() << std::endl;
        return false;
    }
    catch (const std::exception& e) {
        std::cerr << "[RouteDB] Error: Unexpected exception - " << e.what() << std::endl;
        return false;
    }
    catch (...) {
        std::cerr << "[RouteDB] Error: Unknown exception occurred while parsing CSV file" << std::endl;
        return false;
    }
}
*/

std::unordered_map<std::string, Route>& RouteDB::getRouteCallSignMap() {
    return *routeMap;
}

const Route* RouteDB::getRouteByCallsign(const std::string& callsign) {
    // std::cout << "[RouteDB] Searching for route with callsign: " << callsign << std::endl;

//    if (isLoaded.load() == false) {
//        std::cout << "[RouteDB] RouteDB is not loaded. try to retrieve from the web" << std::endl;
//        return getRouteInfoOnWeb(callsign);
//    }
    while(inSwap.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // wait until swap is done
        std::cout << "[RouteDB] Waiting for swap to complete..." << std::endl;
    }

    if (routeMap->find(callsign) == routeMap->end()) {
        // std::cerr << "[RouteDB] Error: Route with callsign '" << callsign << "' not found." << std::endl;
        return new Route();
    }
    // std::cout << "[RouteDB] Found route data for callsign: " << callsign << std::endl;

    return &((*routeMap)[callsign]);
//    std::string routedata = lineDataMap[callsign];
//
//    std::vector<std::string> fields;
//    std::stringstream ss(routedata);
//    std::string field;
//
//    while (std::getline(ss, field, ',')) {
//        field.erase(0, field.find_first_not_of(" \t\r\n"));
//        field.erase(field.find_last_not_of(" \t\r\n") + 1);
//        fields.push_back(field);
//    }
//
//    if (fields.size() >= 5) {
//        Route parsedRoute(fields[0], fields[1], fields[2], fields[3], fields[4]);
//#if 0
//        std::cout << "Successfully parsed route data for: " << callsign << std::endl;
//        std::cout << "Route details: " << std::endl;
//        std::cout << "Callsign: " << parsedRoute.getCallsign() << std::endl;
//        std::cout << "Code: " << parsedRoute.getCode() << std::endl;
//        std::cout << "Number: " << parsedRoute.getNumber() << std::endl;
//        std::cout << "Airline Code: " << parsedRoute.getAirlineCode() << std::endl;
//        std::cout << "Waypoints: ";
//        for (const auto& wp : parsedRoute.getWaypoints()) {
//            std::cout << wp << " ";
//        }
//#endif
//        return parsedRoute;
//    } else {
//        std::cerr << "[RouteDB] Error: Insufficient fields in route data for " << callsign << std::endl;
//        std::cerr << "[RouteDB] Expected 5 fields, got " << fields.size() << std::endl;
//        return Route();
//    }
}

bool RouteDB::startUpdateMonitor() {
    if (!updater) {
        std::cerr << "Error: No updater available" << std::endl;
        return false;
    }
    
    return updater->update(ROUTE_SOURCE_URL, ROUTE_TARGET_FILE, std::bind(&RouteDB::onUpdateComplete, this, std::placeholders::_1));
}


//const Route* RouteDB::getRouteInfoOnWeb(const std::string& callsign) {
//    std::cout << "Getting route information from web for callsign: " << callsign << std::endl;
//
//    try {
//        WebDownloadManager webManager;
//        webManager.setTimeLogging(true);
//        std::string baseUrl = "https://vrs-standing-data.adsb.lol/routes";
//
//        std::string airlineCode = callsign.length() >= 2 ? callsign.substr(0, 2) : callsign;
//        std::string url = baseUrl + "/" + airlineCode + "/" +callsign + ".txt";
//
//        std::string routeInfo = webManager.downloadToString(url);
//
//        if (routeInfo.empty()) {    // connection fail
//            std::cerr << "No route data found for callsign: " << callsign << std::endl;
//            return new Route("", "", "", "", "");
//        } else if(routeInfo == "400") {
//            std::cerr << "No route data found for callsign: " << callsign << std::endl;
//            return new Route("", "", "", "", "");
//        }
//
//        std::cout << "Successfully retrieved route data for callsign: " << callsign << std::endl;
//        std::cout << "Route data: " << routeInfo << std::endl;
//
//        return new Route(callsign, "", "", airlineCode, routeInfo);
//
//    } catch (const std::exception& e) {
//        std::cerr << "Exception in getRouteInfoOnWeb: " << e.what() << std::endl;
//    }
//
//    std::cout << "Returning empty route for " << callsign << std::endl;
//    return new Route("", "", "", "", "");
//}

void RouteDB::updateMapBySwap() {
    std::cout << "[RouteDB] Swapping route maps..." << std::endl;
    std::cout << "[RouteDB] Map size before swap: " << routeMap->size() << ", " << tempMap->size() << std::endl;

    // Swap the maps
    std::swap(routeMap, tempMap);

    std::cout << "[RouteDB] Map size after swap: " << routeMap->size() << ", " << tempMap->size() << std::endl;

    // Clear the temporary map
    for (auto& pair : *tempMap) {
        delete &(pair.second); // releae dynamically allocated Route
    }
    tempMap->clear();

    std::cout << "[RouteDB] Route maps swapped successfully." << std::endl;
}

void RouteDB::onUpdateComplete(bool updateStatus) {
    if (updateStatus) { // keep the status for now
        loadFromFile(ROUTE_TARGET_FILE, true);
        std::cout << "[RouteDB] fetch and load complete" << std::endl;

        if (!inSwap.exchange(true)) {   // Only proceed if not already in swap
            updateMapBySwap();
            inSwap.store(false);
        } else {
            std::cerr << "[RouteDB] Error: Swap operation already in progress" << std::endl;
        }
    
        std::cout << "[RouteDB] Route file update completed!" << std::endl;
    }
}

void RouteDB::parseCSVLine(const std::string& line, std::vector<std::string>& fields) {
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

Route* RouteDB::parseRouteFromCSVLine(const std::string& csvLine) {
    std::vector<std::string> fields;
    try {
        parseCSVLine(csvLine, fields);
        
        if (fields.size() < 5) {
            std::cout << "[RouteDB] Error: CSV line has insufficient fields (" << fields.size() << " < 5)" << std::endl;
            return nullptr;
        }
    
        std::string callsign = fields[0];
        std::string code = fields[1];
        std::string number = fields[2];
        std::string airlinecoode = fields[3];
        std::string waypointsStr = fields[4];

        return new Route(callsign, code, number, airlinecoode, waypointsStr);
    }
    catch (const std::exception& e) {
        std::cout << "[RouteDB] Error parsing CSV line values: " << e.what() << std::endl;
        return nullptr;
    }
}