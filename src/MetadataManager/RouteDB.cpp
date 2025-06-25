#include "RouteDB.h"
#include "csv.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <memory>

// Static member 초기화
std::unique_ptr<RouteDB> RouteDB::instance;
std::mutex RouteDB::mtx;

bool RouteDB::loadFromFile(const std::string& filePath) {
    std::string csvpath = filePath;
    if (filePath.empty()) {
        csvpath = "routes.csv"; // Default file path if none provided
    }
    std::cout << "Loading route data from file: " << csvpath << std::endl;
    
    try {
        // CSV Reader for Route data - expecting 5 columns: Callsign, Code, Number, AirlineCode, Waypoints
        io::CSVReader<5, io::trim_chars<>, io::double_quote_escape<',','\"'>> in(csvpath);

        in.read_header(io::ignore_extra_column, "Callsign", "Code", "Number", "AirlineCode", "AirportCodes");
        std::string callsign, code, number, airlineCode, waypointsStr;

        while(in.read_row(callsign, code, number, airlineCode, waypointsStr)) {
            
            // Parse waypoints string into vector
            
            
            // Create Route object and add to map
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
    std::cout << "Searching for route with callsign: " << callsign << std::endl;
    if (lineDataMap.find(callsign) == lineDataMap.end()) {
        std::cerr << "Error: Route with callsign '" << callsign << "' not found." << std::endl;
        return Route(); // Return an empty Route object
    }
    std::cout << "Found route data for callsign: " << callsign << std::endl;

    std::string routedata = lineDataMap[callsign];
    
    // lineDataMap의 값을 쉼표로 파싱해서 routePtr에 저장
    std::vector<std::string> fields;
    std::stringstream ss(routedata);
    std::string field;
    
    // 쉼표로 분리하여 필드들 추출
    while (std::getline(ss, field, ',')) {
        // 필드에서 앞뒤 공백 제거
        field.erase(0, field.find_first_not_of(" \t\r\n"));
        field.erase(field.find_last_not_of(" \t\r\n") + 1);
        fields.push_back(field);
    }
    
    // 필드가 충분히 있는지 확인 (최소 5개: Callsign, Code, Number, AirlineCode, Waypoints)
    if (fields.size() >= 5) {
        // Route 객체 생성하여 routePtr에 저장
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



// 한줄씩 읽어서 첫 번째 콤마 앞의 단어를 키로 하여 전체 라인을 저장
bool RouteDB::loadFromFileByLine(const std::string& filePath) {
    // 전체 실행시간 측정 시작
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::string csvpath = filePath;
    if (filePath.empty()) {
        csvpath = "routes.csv"; // Default file path if none provided
    }
    
    std::cout << "Loading route data line by line from file: " << csvpath << std::endl;
    
    std::ifstream file(csvpath);
    if (!file.is_open()) {
        // 파일 열기 실패 시에도 실행시간 측정
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cerr << "Error: Cannot open file " << csvpath << std::endl;
        std::cerr << "Failed to open file after " << duration.count() << " milliseconds" << std::endl;
        return false;
    }
    
    try {
        // Clear existing data
        lineDataMap.clear();
        
        std::string line;
        bool isFirstLine = true;
        
        while (std::getline(file, line)) {
            // 첫 번째 줄(헤더)은 무시
            if (isFirstLine) {
                isFirstLine = false;
                std::cout << "Skipping header line: " << line << std::endl;
                continue;
            }
            
            // 빈 줄 건너뛰기
            if (line.empty()) {
                continue;
            }
            
            // 첫 번째 콤마 찾기
            size_t firstCommaPos = line.find(',');
            if (firstCommaPos != std::string::npos) {
                // 첫 번째 콤마 앞의 단어를 키로 사용
                std::string key = line.substr(0, firstCommaPos);
                
                // 키에서 공백 제거 (trim)
                key.erase(0, key.find_first_not_of(" \t\r\n"));
                key.erase(key.find_last_not_of(" \t\r\n") + 1);
                
                // 키와 전체 라인을 맵에 저장
                lineDataMap[key] = line;
                
                // std::cout << "Stored line with key: '" << key << "'" << std::endl;
            } else {
                std::cout << "Warning: No comma found in line: " << line << std::endl;
            }
        }
        
        file.close();
        isLoaded = true;
        
  
        std::cout << "Finished reading file line by line. Total lines stored: " << lineDataMap.size() << std::endl;
        // std::cout << "Line-by-line loading completed in " << duration.count() << " milliseconds" << std::endl;
        return true;
        
    } catch (const std::exception& e) {

        
        std::cerr << "Error while reading file: " << e.what() << std::endl;
        file.close();
        return false;
    }
}
