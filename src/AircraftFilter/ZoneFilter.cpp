#include "ZoneFilter.h"
#include <iostream>
#include <cmath>

// 생성자
ZoneFilter::ZoneFilter() : filterActive(false), currentFilterIndex("") {
    // std::cout << "ZoneFilter: Initialized" << std::endl;
}

// 소멸자
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
        return true; 
    }
    
    try {
        double latitude = targetair.Latitude;
        double longitude = targetair.Longitude;
        
        bool included = isPointInPolygon(latitude, longitude);
        
        if (included) {
            // std::cout << "ZoneFilter: Aircraft " << targetair.HexAddr
            //          << " is included in zone" << std::endl;
        }
        
        return included;
    } catch (const std::exception& e) {
        std::cerr << "ZoneFilter: Error checking airplane inclusion - " << e.what() << std::endl;
        return false;
    }
}

bool ZoneFilter::filterAirplane() {
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
    
    for (DWORD i = 0; i < area.NumPoints; i++) {
        double latitude = area.Points[i][1];   
        double longitude = area.Points[i][0];  
        
        boost::geometry::model::point<double, 2, boost::geometry::cs::geographic<boost::geometry::degree>> point(longitude, latitude); // Boost.Geometry는 (경도, 위도) 순서
        boost::geometry::append(filterPolygon.outer(), point);
        
        // std::cout << "ZoneFilter: Added point (" << latitude << ", " << longitude << ")" << std::endl;
    }
    
    if (area.NumPoints > 0) {
        double firstLat = area.Points[0][1];
        double firstLon = area.Points[0][0];
        boost::geometry::model::point<double, 2, boost::geometry::cs::geographic<boost::geometry::degree>> firstPoint(firstLon, firstLat);
        boost::geometry::append(filterPolygon.outer(), firstPoint);
    }
    
    if (!boost::geometry::is_valid(filterPolygon)) {
        std::cerr << "ZoneFilter: Warning - Created polygon is not valid" << std::endl;
        boost::geometry::correct(filterPolygon);
    }
    
    // std::cout << "ZoneFilter: Polygon created successfully" << std::endl;
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


// 디버깅 및 유틸리티 메서드들

void ZoneFilter::printPolygonInfo() const {
    std::cout << "=== ZoneFilter Polygon Info ===" << std::endl;
    std::cout << "Filter Active: " << (filterActive ? "Yes" : "No") << std::endl;
    std::cout << "Current Filter Index: " << currentFilterIndex << std::endl;
    
    if (filterActive && !filterPolygon.outer().empty()) {
        std::cout << "Polygon Points (" << filterPolygon.outer().size() << "):" << std::endl;
        for (size_t i = 0; i < filterPolygon.outer().size(); ++i) {
            const auto& point = filterPolygon.outer()[i];
            std::cout << "  Point " << i << ": (" 
                     << boost::geometry::get<1>(point) << ", " << boost::geometry::get<0>(point) << ")" << std::endl; // (위도, 경도)
        }
        
        std::cout << "Polygon Area: " << boost::geometry::area(filterPolygon) << std::endl;
        std::cout << "Polygon Valid: " << (boost::geometry::is_valid(filterPolygon) ? "Yes" : "No") << std::endl;
    } else {
        std::cout << "No polygon defined" << std::endl;
    }
    std::cout << "===============================" << std::endl;
}

