/*
BigQuery Analytics API Client helper
HTTP client to fetch BigQuery analytics results in real-time for DisplayGUI
*/

#ifndef BIGQUERY_API_CLIENT_H
#define BIGQUERY_API_CLIENT_H

#include <string>
#include <vector>
#include <memory>
#include <functional>

// Simple structs for JSON parsing
struct AircraftInfo {
    std::string hex_ident;
    std::string callsign;
    double latitude;
    double longitude;
    int altitude;
    int ground_speed;
    double track;
    bool emergency;
    bool alert;
    bool on_ground;
    std::string last_seen;
};

struct TrafficSummary {
    int active_aircraft;
    int total_messages;
    double avg_altitude;
    double avg_speed;
    int emergency_count;
    int alert_count;
    std::string timestamp;
};

struct EmergencyAlert {
    std::string hex_ident;
    std::string callsign;
    double latitude;
    double longitude;
    int altitude;
    std::vector<std::string> alert_types;
    std::string squawk;
    std::string timestamp;
};

class BigQueryAPIClient {
private:
    std::string base_url;
    int timeout_seconds;

    // HTTP GET request helper
    std::string makeHttpRequest(const std::string& endpoint);

    // JSON parsing helper functions
    TrafficSummary parseTrafficSummary(const std::string& json_response);
    std::vector<AircraftInfo> parseAircraftList(const std::string& json_response);
    std::vector<EmergencyAlert> parseEmergencyAlerts(const std::string& json_response);

public:
    BigQueryAPIClient(const std::string& server_url = "http://localhost:8080",
                      int timeout = 5);

    // APIs for DisplayGUI main screen
    TrafficSummary getTrafficSummary();
    std::vector<AircraftInfo> getAircraftInArea(double lat_min, double lat_max,
                                                double lon_min, double lon_max);
    AircraftInfo getAircraftDetail(const std::string& hex_ident);
    std::vector<EmergencyAlert> getEmergencyAlerts();

    // For DisplayGUI map filtering
    std::vector<AircraftInfo> getAircraftInCurrentView(double center_lat, double center_lon,
                                                      double zoom_factor);

    // For DisplayGUI alert system
    bool hasNewEmergencies();
    std::vector<EmergencyAlert> getNewEmergenciesSince(const std::string& timestamp);

    // Connection status checks
    bool isServerReachable();
    std::string getServerStatus();
};

// Callback function types for integration with DisplayGUI
typedef std::function<void(const TrafficSummary&)> TrafficUpdateCallback;
typedef std::function<void(const std::vector<AircraftInfo>&)> AircraftUpdateCallback;
typedef std::function<void(const std::vector<EmergencyAlert>&)> EmergencyCallback;

// DisplayGUI background update manager
class AnalyticsUpdateManager {
private:
    BigQueryAPIClient* api_client;
    bool is_running;
    int update_interval_ms;

    // Callback functions
    TrafficUpdateCallback traffic_callback;
    AircraftUpdateCallback aircraft_callback;
    EmergencyCallback emergency_callback;

    // Background thread
    void updateLoop();

public:
    AnalyticsUpdateManager(BigQueryAPIClient* client, int interval_ms = 5000);

    // Register callbacks
    void setTrafficUpdateCallback(TrafficUpdateCallback callback);
    void setAircraftUpdateCallback(AircraftUpdateCallback callback);
    void setEmergencyCallback(EmergencyCallback callback);

    // Start/stop updates
    void startUpdates();
    void stopUpdates();

    // Update based on current view (when DisplayGUI map view changes)
    void updateCurrentView(double lat_min, double lat_max, double lon_min, double lon_max);
};

#endif // BIGQUERY_API_CLIENT_H

/*
Usage example (in DisplayGUI.cpp):

// 1. Initialize API client
BigQueryAPIClient* analytics_client = new BigQueryAPIClient("http://localhost:8080");
AnalyticsUpdateManager* update_manager = new AnalyticsUpdateManager(analytics_client, 5000);

// 2. Register callbacks
update_manager->setTrafficUpdateCallback([this](const TrafficSummary& summary) {
    // Update DisplayGUI status bar
    StatusBar->Panels->Items[0]->Text = "Aircraft: " + IntToStr(summary.active_aircraft);
    StatusBar->Panels->Items[1]->Text = "Messages: " + IntToStr(summary.total_messages);
    if (summary.emergency_count > 0) {
        // Show emergency notification
        showEmergencyNotification(summary.emergency_count);
    }
});

update_manager->setAircraftUpdateCallback([this](const std::vector<AircraftInfo>& aircraft) {
    // Update aircraft markers on map
    updateAircraftMarkers(aircraft);
});

update_manager->setEmergencyCallback([this](const std::vector<EmergencyAlert>& alerts) {
    // Emergency alert popup/notification
    for (const auto& alert : alerts) {
        showEmergencyAlert(alert);
    }
});

// 3. Start updates
update_manager->startUpdates();

// 4. Called when map view changes
void __fastcall TMainForm::MapViewChanged() {
    double lat_min, lat_max, lon_min, lon_max;
    getCurrentMapBounds(lat_min, lat_max, lon_min, lon_max);
    update_manager->updateCurrentView(lat_min, lat_max, lon_min, lon_max);
}

// 5. Aircraft click for detailed information
void __fastcall TMainForm::AircraftClicked(const std::string& hex_ident) {
    AircraftInfo detail = analytics_client->getAircraftDetail(hex_ident);
    showAircraftDetailPanel(detail);
}
*/
