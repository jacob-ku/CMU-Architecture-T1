#include "RouteDB.h"
#include "csv.h"
#include "../Util/WebDownloadManager.h"
#include "MetadataUpdater.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <memory>

std::unique_ptr<RouteDB> RouteDB::instance;
std::mutex RouteDB::mtx;

RouteDB::RouteDB() {
    updater = std::make_unique<MetaDataUpdater>(SOURCE_URL, TARGET_FILE);
}

// 의존성 주입 생성자
RouteDB::RouteDB(std::unique_ptr<DataUpdaterInterface> injectedUpdater) 
    : updater(std::move(injectedUpdater)) {
}

bool RouteDB::loadFromFile(const std::string& filePath) {
    std::string csvpath = filePath;
    if (filePath.empty()) {
        csvpath = "routes.csv";
    }
    std::cout << "Loading route data from file: " << csvpath << std::endl;
    
    try {
        io::CSVReader<5, io::trim_chars<>, io::double_quote_escape<',','\"'>> in(csvpath);

        in.read_header(io::ignore_extra_column, "Callsign", "Code", "Number", "AirlineCode", "AirportCodes");
        std::string callsign, code, number, airlineCode, waypointsStr;

        while(in.read_row(callsign, code, number, airlineCode, waypointsStr)) {
            Route route(callsign, code, number, airlineCode, waypointsStr);
            routeMap[callsign] = route;
        }
        
        isLoaded = true;
        std::cout << "Finished reading route data from file. Total routes: " << routeMap.size() << std::endl;
        return true;
    }
    catch (const io::error::can_not_open_file& e) {
        std::cerr << "Error: Cannot open file - " << e.what() << std::endl;
        return false;
    }
    catch (const io::error::line_length_limit_exceeded& e) {
        std::cerr << "Error: Line too long - " << e.what() << std::endl;
        return false;
    }
    catch (const io::error::missing_column_in_header& e) {
        std::cerr << "Error: Missing column in header - " << e.what() << std::endl;
        return false;
    }
    catch (const io::error::extra_column_in_header& e) {
        std::cerr << "Error: Extra column in header - " << e.what() << std::endl;
        return false;
    }
    catch (const io::error::duplicated_column_in_header& e) {
        std::cerr << "Error: Duplicated column in header - " << e.what() << std::endl;
        return false;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: Unexpected exception - " << e.what() << std::endl;
        return false;
    }
    catch (...) {
        std::cerr << "Error: Unknown exception occurred while parsing CSV file" << std::endl;
        return false;
    }
}

Route RouteDB::getRouteByCallsign(std::string& callsign) {
    if (isLoaded == false) {
        std::cout << "Error: RouteDB is not loaded. Please load the data first." << std::endl;
        return getRouteInfoOnWeb(callsign);
    }

    std::cout << "Searching for route with callsign: " << callsign << std::endl;
    if (lineDataMap.find(callsign) == lineDataMap.end()) {
        std::cerr << "Error: Route with callsign '" << callsign << "' not found." << std::endl;
        return Route();
    }
    std::cout << "Found route data for callsign: " << callsign << std::endl;

    std::string routedata = lineDataMap[callsign];
    
    std::vector<std::string> fields;
    std::stringstream ss(routedata);
    std::string field;
    
    while (std::getline(ss, field, ',')) {
        field.erase(0, field.find_first_not_of(" \t\r\n"));
        field.erase(field.find_last_not_of(" \t\r\n") + 1);
        fields.push_back(field);
    }
    
    if (fields.size() >= 5) {
        Route parsedRoute(fields[0], fields[1], fields[2], fields[3], fields[4]);
#if 0
        std::cout << "Successfully parsed route data for: " << callsign << std::endl;
        std::cout << "Route details: " << std::endl;
        std::cout << "Callsign: " << parsedRoute.getCallsign() << std::endl;
        std::cout << "Code: " << parsedRoute.getCode() << std::endl;
        std::cout << "Number: " << parsedRoute.getNumber() << std::endl;
        std::cout << "Airline Code: " << parsedRoute.getAirlineCode() << std::endl;
        std::cout << "Waypoints: ";
        for (const auto& wp : parsedRoute.getWaypoints()) {
            std::cout << wp << " ";
        }
#endif
        return parsedRoute;
    } else {
        std::cerr << "Error: Insufficient fields in route data for " << callsign << std::endl;
        std::cerr << "Expected 5 fields, got " << fields.size() << std::endl;
        return Route();
    }
}

bool RouteDB::loadFromFileByLine(const std::string& filePath) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::string csvpath = filePath;
    if (filePath.empty()) {
        csvpath = "routes.csv";
    }
    
    std::cout << "Loading route data line by line from file: " << csvpath << std::endl;
    
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
                
                key.erase(0, key.find_first_not_of(" \t\r\n"));
                key.erase(key.find_last_not_of(" \t\r\n") + 1);
                
                lineDataMap[key] = line;
                
            } else {
                std::cout << "Warning: No comma found in line: " << line << std::endl;
            }
        }
        
        file.close();
        isLoaded = true;
        
  
        std::cout << "Finished reading file line by line. Total lines stored: " << lineDataMap.size() << std::endl;
        return true;
        
    } catch (const std::exception& e) {

        
        std::cerr << "Error while reading file: " << e.what() << std::endl;
        file.close();
        return false;
    }
}


Route RouteDB::getRouteInfoOnWeb(std::string& callsign) {
    std::cout << "Getting route information from web for callsign: " << callsign << std::endl;
    
    try {
        WebDownloadManager webManager;
        webManager.setTimeLogging(true);
        std::string baseUrl = "https://vrs-standing-data.adsb.lol/routes/";
        
        std::string airlineCode = callsign.length() >= 2 ? callsign.substr(0, 2) : callsign;
        std::string url = baseUrl + "/" + airlineCode + "/" +callsign + ".txt";
        
        std::string routeInfo = webManager.downloadToString(url);
        
        if (routeInfo.empty()) {
            std::cerr << "No route data found for callsign: " << callsign << std::endl;
            return Route("", "", "", "", "");
        }
        std::cout << "Successfully retrieved route data for callsign: " << callsign << std::endl;
        std::cout << "Route data: " << routeInfo << std::endl;
        
        return Route(callsign, "", "", airlineCode, routeInfo);
        
    } catch (const std::exception& e) {
        std::cerr << "Exception in getRouteInfoOnWeb: " << e.what() << std::endl;
    }
    
    std::cout << "Returning empty route for " << callsign << std::endl;
    return Route("", "", "", "", "");
}

bool RouteDB::nowloading() {
    return isLoaded;
}

void RouteDB::onUpdateComplete(bool updateStatus) {
    if (!isLoaded && updateStatus) {
        loadFromFileByLine(TARGET_FILE);
        std::cout << "RouteDB data reloaded after update." << std::endl;
    }
    std::cout << "Route file update check completed!" << std::endl;
    std::cout << "Checking for route database updates... : " << updateStatus << std::endl;
    if(isLoaded && !updateStatus) {
        isLoaded = false;
        std::cout << "File Updated...." << std::endl;
    } else {
        std::cout << "RouteDB is not loaded yet." << std::endl;
    }
}


bool RouteDB::startUpdateMonitor() {
    if (!updater) {
        std::cerr << "Error: No updater available" << std::endl;
        return false;
    }
    
    return updater->update(SOURCE_URL, std::bind(&RouteDB::onUpdateComplete, this, std::placeholders::_1));
}
