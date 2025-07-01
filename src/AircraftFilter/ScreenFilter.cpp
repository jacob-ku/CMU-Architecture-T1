#include "ScreenFilter.h"
#include <iostream>

ScreenFilter::ScreenFilter() : ZoneFilter(), 
                               screenMinX(0.0), screenMinY(0.0), 
                               screenMaxX(0.0), screenMaxY(0.0),
                               screenBoundsSet(false) {
}

ScreenFilter::~ScreenFilter() {
}

bool ScreenFilter::isAirplaneIncluded(const TADS_B_Aircraft& targetair) {
    // First check if the aircraft is within the screen bounds (if set)
    
    // Call parent implementation for zone filtering
    return isPointInOutBox(targetair.Latitude, targetair.Longitude);
}

void ScreenFilter::serFilterArea(const TArea& area) {
    
    if (area.NumPoints < 3) {
        std::cout << "ScreenFilter: Area must have at least 3 points, got " << area.NumPoints << std::endl;
        screenBoundsSet = false;
        return;
    }
    
    // Initialize screen bounds from the area points
    bool firstPoint = true;
    for (DWORD i = 0; i < area.NumPoints; i++) {
        double latitude = area.Points[i][1];   // Y coordinate (latitude)
        double longitude = area.Points[i][0];  // X coordinate (longitude)
        
        if (firstPoint) {
            screenMinX = screenMaxX = longitude;
            screenMinY = screenMaxY = latitude;
            firstPoint = false;
        } else {
            if (longitude < screenMinX) screenMinX = longitude;
            if (longitude > screenMaxX) screenMaxX = longitude;
            if (latitude < screenMinY) screenMinY = latitude;
            if (latitude > screenMaxY) screenMaxY = latitude;
        }
    }

    maxLatitude = screenMaxY;
    minLatitude = screenMaxY;
    maxLongitude = screenMinY;
    minLongitude = screenMinX;

    screenBoundsSet = true;
    
    // std::cout << "ScreenFilter: Screen bounds set:" << std::endl;
    // std::cout << "  X (Longitude): " << screenMinX << " to " << screenMaxX << std::endl;
    // std::cout << "  Y (Latitude): " << screenMinY << " to " << screenMaxY << std::endl;
    // std::cout << "  Width: " << (screenMaxX - screenMinX) << "°" << std::endl;
    // std::cout << "  Height: " << (screenMaxY - screenMinY) << "°" << std::endl;
}
