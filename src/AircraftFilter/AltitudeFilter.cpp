#include "altitudefilter.h"
#include <iostream>

AltitudeFilter::AltitudeFilter(double minAlt, double maxAlt) 
    : minAltitude(minAlt), maxAltitude(maxAlt), filterActive(false) {
    std::cout << "AltitudeFilter: Initialized with range [" << minAlt << ", " << maxAlt << "]" << std::endl;
}

AltitudeFilter::~AltitudeFilter() {
    std::cout << "AltitudeFilter: Destroyed" << std::endl;
}

bool AltitudeFilter::isAirplaneIncluded(const TADS_B_Aircraft& targetair) {
    if (!filterActive) {
        return true; // If filter is not active, include all aircraft
    }
    
    double altitude = targetair.Altitude;
    bool included = (altitude >= minAltitude && altitude <= maxAltitude);
    
    if (included) {
        std::cout << "AltitudeFilter: Aircraft " << targetair.HexAddr 
                  << " at altitude " << altitude << " is included" << std::endl;
    }
    
    return included;
}

bool AltitudeFilter::ClearFilter() {
    std::cout << "AltitudeFilter: Clearing filter" << std::endl;
    filterActive = false;
    return true;
}

bool AltitudeFilter::getFilteredAirplanes() const {
    // Return whether filter is active (simplified implementation)
    return filterActive;
}

void AltitudeFilter::ActivateFilter() {
    filterActive = true;
    std::cout << "AltitudeFilter: Filter activated with range [" 
              << minAltitude << ", " << maxAltitude << "]" << std::endl;
}

void AltitudeFilter::DeactivateFilter() {
    filterActive = false;
    std::cout << "AltitudeFilter: Filter deactivated" << std::endl;
}

bool AltitudeFilter::isFilterActive() const {
    return filterActive;
}

void AltitudeFilter::setAltitudeRange(double minAlt, double maxAlt) {
    minAltitude = minAlt;
    maxAltitude = maxAlt;
    std::cout << "AltitudeFilter: Altitude range set to [" << minAlt << ", " << maxAlt << "]" << std::endl;
}

void AltitudeFilter::setAndFilter(bool value) {
    // Set AND filter mode for ZoneFilter
    // This could be used to determine how multiple conditions are combined
    std::cout << "ZoneFilter: AND filter mode set to " << (value ? "true" : "false") << std::endl;
    // Implementation can be expanded based on specific requirements
}
