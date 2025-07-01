#include "MilitaryFilter.h"

MilitaryFilter::MilitaryFilter() : filterActive(false), isAndFilter(false) {
    std::cout << "MilitaryFilter: Initialized" << std::endl;
}

MilitaryFilter::~MilitaryFilter() {
    std::cout << "MilitaryFilter: Destroyed" << std::endl;
}

bool MilitaryFilter::isAirplaneIncluded(const TADS_B_Aircraft& targetair) {
    if (!filterActive) {
        // std::cout << "MilitaryFilter: Filter is not active, including all aircraft." << std::endl;
        return true; // If filter is not active, include all aircraft
    }
    
    // Check if the aircraft is military
    // The TADS_B_Aircraft structure has an IsMilitary flag
    bool isMilitary = targetair.IsMilitary;
    if (isMilitary)
    {
        // std::cout << "MilitaryFilter: Aircraft " << targetair.HexAddr 
        //     << " military status: " << std::endl;
    }
    
    
    return isMilitary;
}

bool MilitaryFilter::ClearFilter() {
    std::cout << "MilitaryFilter: Clearing filter" << std::endl;
    filterActive = false;
    return true;
}

bool MilitaryFilter::getFilteredAirplanes() const {
    // This method typically returns filtered results
    // For military filter, we return the current filter state
    std::cout << "MilitaryFilter: Getting filtered airplanes, filter active: " 
              << (filterActive ? "YES" : "NO") << std::endl;
    return filterActive;
}

void MilitaryFilter::ActivateFilter() {
    std::cout << "MilitaryFilter: Activating military aircraft filter" << std::endl;
    filterActive = true;
}

void MilitaryFilter::DeactivateFilter() {
    std::cout << "MilitaryFilter: Deactivating military aircraft filter" << std::endl;
    filterActive = false;
}

bool MilitaryFilter::isFilterActive() const {
    return filterActive;
}

void MilitaryFilter::setAndFilter(bool value) {
    std::cout << "MilitaryFilter: Setting AND filter mode: " << (value ? "ON" : "OFF") << std::endl;
    isAndFilter = value;
}

bool MilitaryFilter::getAndfilter() const {
    return isAndFilter;
}