#include "AircraftFilter.h"
#include <iostream>

AircraftFilter::AircraftFilter() {
    std::cout << "AircraftFilter: Initialized" << std::endl;
}

AircraftFilter::~AircraftFilter() {
    std::cout << "AircraftFilter: Destroyed" << std::endl;
    clearAllFilters();
}

void AircraftFilter::addFilter(const std::string& filterName, std::unique_ptr<AirplaneFilterInterface> filterInstance) {
    if (filterName.empty()) {
        std::cerr << "AircraftFilter: Error - Filter name cannot be empty" << std::endl;
        return;
    }
    
    if (!filterInstance) {
        std::cerr << "AircraftFilter: Error - Filter instance cannot be null" << std::endl;
        return;
    }
    
    if (filters.find(filterName) != filters.end()) {
        // std::cout << "AircraftFilter: Warning - Filter '" << filterName << "' already exists, replacing..." << std::endl;
    }
    
    filters[filterName] = std::move(filterInstance);
}

void AircraftFilter::removeFilter(const std::string& filterName) {
    auto it = filters.find(filterName);
    if (it != filters.end()) {
        std::cout << "AircraftFilter: Removing filter '" << filterName << "'" << std::endl;
        filters.erase(it);
    } else {
        std::cout << "AircraftFilter: Warning - Filter '" << filterName << "' not found for removal" << std::endl;
    }
}

void AircraftFilter::replaceFilter(const std::string& filterName, std::unique_ptr<AirplaneFilterInterface> filterInstance) {
    if (filterName.empty()) {
        std::cerr << "AircraftFilter: Error - Filter name cannot be empty for replacement" << std::endl;
        return;
    }
    
    if (!filterInstance) {
        std::cerr << "AircraftFilter: Error - Filter instance cannot be null for replacement" << std::endl;
        return;
    }
    
    
    bool filterExisted = hasFilter(filterName);
    if (filterExisted) {
        std::cout << "AircraftFilter: Existing filter '" << filterName << "' will be replaced" << std::endl;
    } else {
        std::cout << "AircraftFilter: Filter '" << filterName << "' does not exist, creating new one" << std::endl;
    }
    
    removeFilter(filterName);
    addFilter(filterName, std::move(filterInstance));
    
}

void AircraftFilter::activateFilter(const std::string& filterName) {
    auto it = filters.find(filterName);
    if (it != filters.end()) {
        if (it->second->isFilterActive()) {
            return; // Filter is already active
        }
        std::cout << "AircraftFilter: Activating filter '" << filterName << "'" << std::endl;
        it->second->ActivateFilter();
    } else {
        std::cerr << "AircraftFilter: Error - Filter '" << filterName << "' not found for activation" << std::endl;
    }
}

void AircraftFilter::deactivateFilter(const std::string& filterName) {
    auto it = filters.find(filterName);
    if (it != filters.end()) {
        std::cout << "AircraftFilter: Deactivating filter '" << filterName << "'" << std::endl;
        it->second->DeactivateFilter();
    } else {
        std::cerr << "AircraftFilter: Error - Filter '" << filterName << "' not found for deactivation" << std::endl;
    }
}

bool AircraftFilter::filterAircraft(const TADS_B_Aircraft& aircraft) {
    if (filters.empty()) {
        return true;
    }
    
    // std::cout << "AircraftFilter: Filtering aircraft " << aircraft.HexAddr << " through " << filters.size() << " filters" << std::endl;
    
    for (const auto& [filterName, filterInstance] : filters) {
        
        // std::cout << "[KEY]AircraftFilter: Applying filter '" << filterName << "' to aircraft " << aircraft.HexAddr << std::endl;
        try {
            if (filterInstance->isAirplaneIncluded(aircraft)) {
                // std::cout << "AircraftFilter: Aircraft " << aircraft.HexAddr 
                //          << " pass filter: " << filterName << std::endl;
                         return true; // Aircraft passed the filter                
            } else {
                continue;
            }
        } catch (const std::exception& e) {
            std::cerr << "AircraftFilter: Error in filter '" << filterName << "': " << e.what() << std::endl;
            return false;
        }
    }
    
    // std::cout << "AircraftFilter: Aircraft " << aircraft.HexAddr << " could not pass filters" << std::endl;
    return false;
}

bool AircraftFilter::filterAircraftPosition(double latitude, double longitude) {
    // std::cout << "AircraftFilter: Filtering position (" << latitude << ", " << longitude << ")" << std::endl;
    
    TADS_B_Aircraft tempAircraft;
    strcpy(tempAircraft.FlightNum, "TEMP"); // Temporary flight number for filtering
    tempAircraft.Latitude = latitude;
    tempAircraft.Longitude = longitude;
    
    return filterAircraft(tempAircraft);
}


bool AircraftFilter::hasFilter(const std::string& filterName) const {
    return filters.find(filterName) != filters.end();
}

void AircraftFilter::clearAllFilters() {
    if (!filters.empty()) {
        std::cout << "AircraftFilter: Clearing " << filters.size() << " filters" << std::endl;
        filters.clear();
    }
}

size_t AircraftFilter::getFilterCount() const {
    return filters.size();
}

void AircraftFilter::printFilterStatus() const {
    std::cout << "=== AircraftFilter Status ===" << std::endl;
    std::cout << "Total Filters: " << filters.size() << std::endl;
    
    if (filters.empty()) {
        std::cout << "No filters defined" << std::endl;
    } else {
        for (const auto& [filterName, filterInstance] : filters) {
            std::cout << "Filter '" << filterName << "': ";
            if (filterInstance) {
                std::cout << "Active" << std::endl;
            } else {
                std::cout << "NULL" << std::endl;
            }
        }
    }
    std::cout << "============================" << std::endl;
}


