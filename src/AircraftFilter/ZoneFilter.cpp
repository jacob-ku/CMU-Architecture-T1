#include "ZoneFilter.h"
#include <iostream>
#include <cmath>

ZoneFilter::ZoneFilter() : filterActive(false), currentFilterIndex(""),
                           maxLatitude(90.0), minLatitude(-90.0),
                           maxLongitude(180.0), minLongitude(-180.0) {
    // std::cout << "ZoneFilter: Initialized" << std::endl;
}

ZoneFilter::~ZoneFilter() {
    // std::cout << "ZoneFilter: Destroyed" << std::endl;
}


void ZoneFilter::serFilterArea(const TArea& area) {
    // std::cout << "ZoneFilter: Setting filter area - " << area.Name.c_str() << std::endl;
    
    try {
        setPolygonFromArea(area);
        filterActive = true;
        // std::cout << "ZoneFilter: Filter area set successfully with " << area.NumPoints << " points" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "ZoneFilter: Error setting filter area - " << e.what() << std::endl;
        filterActive = false;
    }
}

void ZoneFilter::updateFilterArea(const TArea& area, const std::string filter_index) {
    // std::cout << "ZoneFilter: Updating filter area - Index: " << filter_index << std::endl;
    
    currentFilterIndex = filter_index;
    serFilterArea(area);
}

bool ZoneFilter::isAirplaneIncluded(const TADS_B_Aircraft& targetair) {
    if (!filterActive) {
        return false; 
    }
    
    if (!isPointInOutBox(targetair.Latitude, targetair.Longitude)) {
        // std::cout << "ZoneFilter: Aircraft " << targetair.HexAddr 
        //           << " at (" << targetair.Latitude << ", " << targetair.Longitude 
        //           << ") is outside polygon bounds" << std::endl;
        return false; 
    }
    // if (isPointInInBox(targetair.Latitude, targetair.Longitude)) {
    //     // std::cout << "ZoneFilter: Aircraft " << targetair.HexAddr 
    //     //           << " at (" << targetair.Latitude << ", " << targetair.Longitude 
    //     //           << ") is inside polygon bounds" << std::endl;
    //     return true; 
    // }
    try {
        double latitude = targetair.Latitude;
        double longitude = targetair.Longitude;
        // std::cout << "ZoneFilter: Checking if aircraft " << targetair.HexAddr 
        //           << " at (" << latitude << ", " << longitude << ") is included in zone" << std::endl;
        bool included = isPointInPolygon(latitude, longitude);
        
        // if (included) {
        //     std::cout << "ZoneFilter: Aircraft " << targetair.HexAddr
        //              << " is included in zone" << std::endl;
        // }
        
        return included;
        //return true;
    } catch (const std::exception& e) {
        std::cerr << "ZoneFilter: Error checking airplane inclusion - " << e.what() << std::endl;
        return false;
    }
}

bool ZoneFilter::ClearFilter() {
    if (!filterActive) {
        std::cout << "ZoneFilter: Filter is not active" << std::endl;
        return false;
    }
    
    // std::cout << "ZoneFilter: Filtering airplanes in zone" << std::endl;
    clearFilteredAirplanes();
    
    // std::cout << "ZoneFilter: Filtered " << filteredAirplanes.size() << " airplanes" << std::endl;
    return true;
}

bool ZoneFilter::getFilteredAirplanes() const {
    // std::cout << "ZoneFilter: Getting filtered airplanes count - " << filteredAirplanes.size() << std::endl;
    return !filteredAirplanes.empty();
}

void ZoneFilter::ActivateFilter() {
    filterActive = true;
    // std::cout << "ZoneFilter: Filter activated" << std::endl;
}

void ZoneFilter::DeactivateFilter() {
    filterActive = false;
    clearFilteredAirplanes();
    // std::cout << "ZoneFilter: Filter deactivated" << std::endl;
}


void ZoneFilter::setPolygonFromArea(const TArea& area) {
    // std::cout << "ZoneFilter: Converting TArea to polygon with " << area.NumPoints << " points" << std::endl;
    
    if (area.NumPoints < 3) {
        throw std::runtime_error("ZoneFilter: Area must have at least 3 points to form a polygon");
    }
    
    filterPolygon.clear();
    
    // Initialize min/max values for the first point
    bool firstPoint = true;
    double polygonMaxLat = -90.0, polygonMinLat = 90.0;
    double polygonMaxLon = -180.0, polygonMinLon = 180.0;
    
    for (DWORD i = 0; i < area.NumPoints; i++) {
        double latitude = area.Points[i][1];   
        double longitude = area.Points[i][0];  
        
        // Calculate min/max values for polygon bounds
        if (firstPoint) {
            polygonMaxLat = polygonMinLat = latitude;
            polygonMaxLon = polygonMinLon = longitude;
            firstPoint = false;
        } else {
            if (latitude > polygonMaxLat) polygonMaxLat = latitude;
            if (latitude < polygonMinLat) polygonMinLat = latitude;
            if (longitude > polygonMaxLon) polygonMaxLon = longitude;
            if (longitude < polygonMinLon) polygonMinLon = longitude;
        }
        
        boost::geometry::model::point<double, 2, boost::geometry::cs::geographic<boost::geometry::degree>> point(longitude, latitude); // Boost.Geometry는 (경도, 위도) 순서
        boost::geometry::append(filterPolygon.outer(), point);

    }
    
    // Store the calculated bounds
    maxLatitude = polygonMaxLat;
    minLatitude = polygonMinLat;
    maxLongitude = polygonMaxLon;
    minLongitude = polygonMinLon;

    // // Log the polygon bounds
    // std::cout << "ZoneFilter: Polygon bounds calculated:" << std::endl;
    // std::cout << "  Latitude: " << minLatitude << " to " << maxLatitude << std::endl;
    // std::cout << "  Longitude: " << minLongitude << " to " << maxLongitude << "°" << std::endl;
    // std::cout << "  Lat range: " << (maxLatitude - minLatitude) << "°" << std::endl;
    // std::cout << "  Lon range: " << (maxLongitude - minLongitude) << "°" << std::endl;
    
    if (area.NumPoints > 0) {
        double firstLat = area.Points[0][1];
        double firstLon = area.Points[0][0];
        boost::geometry::model::point<double, 2, boost::geometry::cs::geographic<boost::geometry::degree>> firstPoint(firstLon, firstLat);
        boost::geometry::append(filterPolygon.outer(), firstPoint);
    }
    
    getMaxInscribedRectangle(filterPolygon, mininnerLongitude, maxinnerLongitude, mininnerLatitude, maxinnerLatitude);

    if (!boost::geometry::is_valid(filterPolygon)) {
        std::cerr << "ZoneFilter: Warning - Created polygon is not valid" << std::endl;
        boost::geometry::correct(filterPolygon);
    } else {
           // std::cout << "ZoneFilter: Polygon created successfully with " << filterPolygon.outer().size() << " points" << std::endl;
    }
    
}

bool ZoneFilter::isPointInPolygon(double latitude, double longitude) const {
    if (!filterActive) {
        return true;
    }
    
    try {
        boost::geometry::model::point<double, 2, boost::geometry::cs::geographic<boost::geometry::degree>> point(longitude, latitude); // (경도, 위도) 순서
        bool result = boost::geometry::within(point, filterPolygon);
        
        // std::cout << "ZoneFilter: Point (" << latitude << ", " << longitude 
        //          << ") is " << (result ? "inside" : "outside") << " polygon" << std::endl;
        
        return result;
    } catch (const std::exception& e) {
        std::cerr << "ZoneFilter: Error checking point in polygon - " << e.what() << std::endl;
        return false;
    }
}

bool ZoneFilter::isPointInOutBox(double latitude, double longitude) const {
    // Check if the given latitude and longitude are within the polygon bounds
    if (latitude >= minLatitude && latitude <= maxLatitude &&
        longitude >= minLongitude && longitude <= maxLongitude) {
        return true;
    }
    return false;
}

bool ZoneFilter::isPointInInBox(double latitude, double longitude) const {
    // Check if the given latitude and longitude are within the polygon bounds
    if (latitude >= maxinnerLatitude && latitude <= maxinnerLatitude &&
        longitude >= mininnerLongitude && longitude <= maxinnerLongitude) {
        return true;
    }
    return false;
}

void ZoneFilter::clearFilteredAirplanes() {
    filteredAirplanes.clear();
    std::cout << "ZoneFilter: Cleared filtered airplanes list" << std::endl;
}

size_t ZoneFilter::getFilteredAirplanesCount() const {
    return filteredAirplanes.size();
}

bool ZoneFilter::isFilterActive() const {
    return filterActive;
}

// Getter functions for polygon bounds
double ZoneFilter::getMaxLatitude() const {
    return maxLatitude;
}

double ZoneFilter::getMinLatitude() const {
    return minLatitude;
}

double ZoneFilter::getMaxLongitude() const {
    return maxLongitude;
}

double ZoneFilter::getMinLongitude() const {
    return minLongitude;
}

void ZoneFilter::getPolygonBounds(double& maxLat, double& minLat, double& maxLon, double& minLon) const {
    maxLat = maxLatitude;
    minLat = minLatitude;
    maxLon = maxLongitude;
    minLon = minLongitude;
}



void ZoneFilter::printPolygonInfo() const {
    std::cout << "=== ZoneFilter Polygon Info ===" << std::endl;
    std::cout << "Filter Active: " << (filterActive ? "Yes" : "No") << std::endl;
    std::cout << "Current Filter Index: " << currentFilterIndex << std::endl;
    
    if (filterActive && !filterPolygon.outer().empty()) {
        std::cout << "Polygon Bounds:" << std::endl;
        std::cout << "  Max Latitude: " << maxLatitude << "°" << std::endl;
        std::cout << "  Min Latitude: " << minLatitude << "°" << std::endl;
        std::cout << "  Max Longitude: " << maxLongitude << "°" << std::endl;
        std::cout << "  Min Longitude: " << minLongitude << "°" << std::endl;
        std::cout << "  Latitude Range: " << (maxLatitude - minLatitude) << "°" << std::endl;
        std::cout << "  Longitude Range: " << (maxLongitude - minLongitude) << "°" << std::endl;
        
        std::cout << "Polygon Points (" << filterPolygon.outer().size() << "):" << std::endl;
        for (size_t i = 0; i < filterPolygon.outer().size(); ++i) {
            const auto& point = filterPolygon.outer()[i];
            std::cout << "  Point " << i << ": (" 
                     << boost::geometry::get<1>(point) << ", " << boost::geometry::get<0>(point) << ")" << std::endl;
        }
        
        std::cout << "Polygon Area: " << boost::geometry::area(filterPolygon) << std::endl;
        std::cout << "Polygon Valid: " << (boost::geometry::is_valid(filterPolygon) ? "Yes" : "No") << std::endl;
    } else {
        std::cout << "No polygon defined" << std::endl;
    }
    std::cout << "===============================" << std::endl;
}

void ZoneFilter::setAndFilter(bool value) {
    // Set AND filter mode for ZoneFilter
    // This could be used to determine how multiple conditions are combined
    std::cout << "ZoneFilter: AND filter mode set to " << (value ? "true" : "false") << std::endl;
    // Implementation can be expanded based on specific requirements
}

bool ZoneFilter::getVisualCenter(double& longitude, double& latitude) const {
    if (boost::geometry::is_empty(filterPolygon) || !boost::geometry::is_valid(filterPolygon)) {
        std::cout << "ZoneFilter: Cannot get visual center - polygon is invalid" << std::endl;
        longitude = 0.0;
        latitude = 0.0;
        return false;
    }
    
    try {
        // 경계상자(envelope) 중심으로 계산
        boost::geometry::model::box<boost::geometry::model::point<double, 2, boost::geometry::cs::geographic<boost::geometry::degree>>> boundingBox;
        boost::geometry::envelope(filterPolygon, boundingBox);
        
        // 경계상자의 중심점 계산
        longitude = (boost::geometry::get<boost::geometry::min_corner, 0>(boundingBox) + 
                    boost::geometry::get<boost::geometry::max_corner, 0>(boundingBox)) / 2.0;
        latitude = (boost::geometry::get<boost::geometry::min_corner, 1>(boundingBox) + 
                   boost::geometry::get<boost::geometry::max_corner, 1>(boundingBox)) / 2.0;
        
        std::cout << "ZoneFilter: Visual center calculated - Lon: " << longitude 
                  << ", Lat: " << latitude << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cout << "ZoneFilter: Error calculating visual center - " << e.what() << std::endl;
        longitude = 0.0;
        latitude = 0.0;
        return false;
    }
}


bool ZoneFilter::getMaxInscribedRectangle(const boost::geometry::model::polygon<boost::geometry::model::point<double, 2, boost::geometry::cs::geographic<boost::geometry::degree>>>& polygon,
                                         double& minLon, double& maxLon, double& minLat, double& maxLat) const {
    if (boost::geometry::is_empty(polygon)) {
        std::cout << "ZoneFilter: - polygon is invalid" << std::endl;
        minLon = maxLon = minLat = maxLat = 0.0;
        return false;
    }
    
    try {
        // 1. 전달받은 polygon의 경계상자 중심점 구하기
        boost::geometry::model::box<boost::geometry::model::point<double, 2, boost::geometry::cs::geographic<boost::geometry::degree>>> boundingBox;
        boost::geometry::envelope(polygon, boundingBox);
        
        double visualCenterLon = (boost::geometry::get<boost::geometry::min_corner, 0>(boundingBox) + 
                                 boost::geometry::get<boost::geometry::max_corner, 0>(boundingBox)) / 2.0;
        double visualCenterLat = (boost::geometry::get<boost::geometry::min_corner, 1>(boundingBox) + 
                                 boost::geometry::get<boost::geometry::max_corner, 1>(boundingBox)) / 2.0;
        
        // 2. 경계상자 범위 구하기
        double boundMinLon = boost::geometry::get<boost::geometry::min_corner, 0>(boundingBox);
        double boundMinLat = boost::geometry::get<boost::geometry::min_corner, 1>(boundingBox);
        double boundMaxLon = boost::geometry::get<boost::geometry::max_corner, 0>(boundingBox);
        double boundMaxLat = boost::geometry::get<boost::geometry::max_corner, 1>(boundingBox);
        
        // 3. 중심점에서 시작해서 4방향으로 확장하면서 최대 사각형 찾기
        double maxArea = 0.0;
        double bestMinLon = visualCenterLon, bestMaxLon = visualCenterLon;
        double bestMinLat = visualCenterLat, bestMaxLat = visualCenterLat;
        
        // 중심점에서 경계까지의 최대 가능 거리
        double maxWidthLeft = visualCenterLon - boundMinLon;
        double maxWidthRight = boundMaxLon - visualCenterLon;
        double maxHeightDown = visualCenterLat - boundMinLat;
        double maxHeightUp = boundMaxLat - visualCenterLat;
        
        // 탐색 해상도 (너무 세밀하면 느려짐)
        int steps = 5;
        double stepWidthLeft = maxWidthLeft / steps;
        double stepWidthRight = maxWidthRight / steps;
        double stepHeightDown = maxHeightDown / steps;
        double stepHeightUp = maxHeightUp / steps;
        
        std::cout << "ZoneFilter: Searching for max rectangle from center (" 
                  << visualCenterLon << ", " << visualCenterLat << ")" << std::endl;
        
        // 4방향으로 확장하면서 최대 사각형 찾기
        for (int wl = 1; wl <= steps; wl++) {
            for (int wr = 1; wr <= steps; wr++) {
                for (int hd = 1; hd <= steps; hd++) {
                    for (int hu = 1; hu <= steps; hu++) {
                        double testMinLon = visualCenterLon - wl * stepWidthLeft;
                        double testMaxLon = visualCenterLon + wr * stepWidthRight;
                        double testMinLat = visualCenterLat - hd * stepHeightDown;
                        double testMaxLat = visualCenterLat + hu * stepHeightUp;
                        
                        // 사각형의 4개 꼭짓점 생성
                        boost::geometry::model::point<double, 2, boost::geometry::cs::geographic<boost::geometry::degree>> 
                            corner1(testMinLon, testMinLat),
                            corner2(testMaxLon, testMinLat),
                            corner3(testMaxLon, testMaxLat),
                            corner4(testMinLon, testMaxLat);
                        
                        // 4개 꼭짓점이 모두 polygon 내부에 있는지 확인
                        if (boost::geometry::within(corner1, polygon) &&
                            boost::geometry::within(corner2, polygon) &&
                            boost::geometry::within(corner3, polygon) &&
                            boost::geometry::within(corner4, polygon)) {
                            
                            double rectWidth = testMaxLon - testMinLon;
                            double rectHeight = testMaxLat - testMinLat;
                            double area = rectWidth * rectHeight;
                            
                            if (area > maxArea) {
                                maxArea = area;
                                bestMinLon = testMinLon;
                                bestMaxLon = testMaxLon;
                                bestMinLat = testMinLat;
                                bestMaxLat = testMaxLat;
                            }
                        }
                    }
                }
            }
        }
        
        if (maxArea > 0) {
            minLon = bestMinLon;
            maxLon = bestMaxLon;
            minLat = bestMinLat;
            maxLat = bestMaxLat;
            
            std::cout << "ZoneFilter: Max inscribed rectangle found:" << std::endl;
            std::cout << "  Bounds: [" << minLon << ", " << minLat << "] to [" << maxLon << ", " << maxLat << "]" << std::endl;
            std::cout << "  Size: " << (maxLon - minLon) << " x " << (maxLat - minLat) << " (area: " << maxArea << ")" << std::endl;
            std::cout << "  Center: (" << ((minLon + maxLon) / 2.0) << ", " << ((minLat + maxLat) / 2.0) << ")" << std::endl;
            
            return true;
        } else {
            std::cout << "ZoneFilter: No valid rectangle found" << std::endl;
            minLon = maxLon = visualCenterLon;
            minLat = maxLat = visualCenterLat;
            return false;
        }
        
    } catch (const std::exception& e) {
        std::cout << "ZoneFilter: Error calculating max inscribed rectangle - " << e.what() << std::endl;
        minLon = maxLon = minLat = maxLat = 0.0;
        return false;
    }
}

