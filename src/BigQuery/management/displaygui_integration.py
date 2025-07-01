#!/usr/bin/env python3
"""
DisplayGUI Integration Manager
Integrated service connecting C++ DisplayGUI program with BigQuery analytics
"""

import sys
from pathlib import Path
import subprocess
import threading
import time
import json
import requests
from datetime import datetime

# Add project root to Python path
project_root = Path(__file__).parent.parent
sys.path.append(str(project_root))


class DisplayGUIIntegrationManager:
    def __init__(self):
        self.api_server_process = None
        self.alert_system_process = None
        self.streaming_process = None
        self.is_running = False

    def start_analytics_api_server(self):
        """Start Analytics API server"""
        try:
            print("🚀 Starting Analytics API server...")
            self.api_server_process = subprocess.Popen([
                sys.executable,
                str(project_root / "core" / "AnalyticsAPIServer.py")
            ], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

            # Wait for server to start
            time.sleep(3)

            # Check server status
            try:
                response = requests.get("http://localhost:8080/api/health", timeout=5)
                if response.status_code == 200:
                    print("✅ Analytics API server started (port 8080)")
                    return True
            except requests.exceptions.RequestException:
                print("❌ Analytics API server failed to start")
                return False

        except Exception as e:
            print(f"❌ Analytics API server startup error: {e}")
            return False

    def start_alert_system(self):
        """Start real-time alert system"""
        try:
            print("🚨 Starting real-time alert system...")
            self.alert_system_process = subprocess.Popen([
                sys.executable,
                str(project_root / "core" / "RealTimeAlertSystem.py")
            ], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

            time.sleep(2)
            print("✅ Real-time alert system started (port 8081)")
            return True

        except Exception as e:
            print(f"❌ Real-time alert system startup error: {e}")
            return False

    def start_data_streaming(self):
        """Start data streaming"""
        try:
            print("📡 Checking data streaming...")

            # Check if streaming is already running
            try:
                response = requests.get("http://localhost:8080/api/traffic/summary", timeout=5)
                if response.status_code == 200:
                    data = response.json()
                    if data.get('active_aircraft', 0) > 0:
                        print("✅ Data streaming is already active")
                        return True
            except:
                pass

            # Start streaming
            print("🚀 Starting data streaming...")
            self.streaming_process = subprocess.Popen([
                sys.executable,
                str(project_root / "management" / "start_streaming.py")
            ], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

            time.sleep(5)
            print("✅ Data streaming started successfully")
            return True

        except Exception as e:
            print(f"❌ Data streaming startup error: {e}")
            return False

    def create_displaygui_config(self):
        """Create configuration file for DisplayGUI"""
        config = {
            "analytics_api": {
                "base_url": "http://localhost:8080",
                "endpoints": {
                    "traffic_summary": "/api/traffic/summary",
                    "aircraft_area": "/api/aircraft/area",
                    "aircraft_detail": "/api/aircraft/{hex_ident}",
                    "emergency_alerts": "/api/alerts/emergency",
                    "health_check": "/api/health"
                }
            },
            "websocket_alerts": {
                "url": "ws://localhost:8081",
                "reconnect_interval": 5000,
                "max_reconnect_attempts": 10
            },
            "update_intervals": {
                "traffic_summary_ms": 5000,
                "aircraft_positions_ms": 2000,
                "emergency_check_ms": 1000
            },
            "display_settings": {
                "show_analytics_overlay": True,
                "show_emergency_alerts": True,
                "show_traffic_density": True,
                "alert_sound_enabled": True
            }
        }

        config_path = project_root / "config" / "displaygui_integration.json"
        with open(config_path, 'w', encoding='utf-8') as f:
            json.dump(config, f, indent=2, ensure_ascii=False)

        print(f"✅ DisplayGUI configuration file created: {config_path}")
        return config_path

    def test_integration(self):
        """Integration test"""
        print("\n🔍 Starting integration test...")

        tests = [
            ("API server status", self._test_api_server),
            ("Traffic summary query", self._test_traffic_summary),
            ("Aircraft area search", self._test_aircraft_area),
            ("Emergency situation check", self._test_emergency_alerts)
        ]

        results = {}
        for test_name, test_func in tests:
            try:
                result = test_func()
                results[test_name] = "✅ Success" if result else "❌ Failed"
                print(f"  {test_name}: {results[test_name]}")
            except Exception as e:
                results[test_name] = f"❌ Error: {e}"
                print(f"  {test_name}: {results[test_name]}")

        return results

    def _test_api_server(self):
        """API server test"""
        response = requests.get("http://localhost:8080/api/health", timeout=5)
        return response.status_code == 200

    def _test_traffic_summary(self):
        """Traffic summary test"""
        response = requests.get("http://localhost:8080/api/traffic/summary", timeout=5)
        if response.status_code == 200:
            data = response.json()
            return 'active_aircraft' in data
        return False

    def _test_aircraft_area(self):
        """Aircraft area search test"""
        params = {
            'lat_min': 40.0,
            'lat_max': 42.0,
            'lon_min': -88.0,
            'lon_max': -86.0
        }
        response = requests.get("http://localhost:8080/api/aircraft/area",
                              params=params, timeout=5)
        if response.status_code == 200:
            data = response.json()
            return 'aircraft' in data
        return False

    def _test_emergency_alerts(self):
        """Emergency alert test"""
        response = requests.get("http://localhost:8080/api/alerts/emergency", timeout=5)
        if response.status_code == 200:
            data = response.json()
            return 'alerts' in data
        return False

    def generate_displaygui_integration_guide(self):
        """Generate DisplayGUI integration guide"""
        guide_content = """
# DisplayGUI BigQuery Integration Guide

## 1. System Architecture

### Services:
- **Analytics API Server** (port 8080): Provides real-time analytics data via REST API
- **WebSocket Alert System** (port 8081): Real-time event notifications
- **Data Streaming**: Streams SBS messages to BigQuery in real time

## 2. DisplayGUI C++ Code Integration

### 2.1 Adding HTTP Client
```cpp
// Include BigQueryAPIClient.h
#include "BigQuery/core/BigQueryAPIClient.h"

// Add member variables to DisplayGUI.h
private:
    BigQueryAPIClient* analytics_client;
    AnalyticsUpdateManager* update_manager;
    TTimer* analytics_timer;
```

### 2.2 Initialization Code (Form Constructor)
```cpp
__fastcall TMainForm::TMainForm(TComponent* Owner) : TForm(Owner) {
    // Existing initialization code...

    // Initialize BigQuery Analytics integration
    analytics_client = new BigQueryAPIClient("http://localhost:8080");
    update_manager = new AnalyticsUpdateManager(analytics_client, 5000);

    // Register callbacks
    setupAnalyticsCallbacks();

    // Start updates
    update_manager->startUpdates();
}
```

### 2.3 Implementing Callback Functions
```cpp
void TMainForm::setupAnalyticsCallbacks() {
    // Update traffic summary
    update_manager->setTrafficUpdateCallback([this](const TrafficSummary& summary) {
        StatusBar->Panels->Items[0]->Text =
            "Aircraft: " + IntToStr(summary.active_aircraft);
        StatusBar->Panels->Items[1]->Text =
            "Messages: " + IntToStr(summary.total_messages);

        // Emergency alerts
        if (summary.emergency_count > 0) {
            EmergencyPanel->Visible = true;
            EmergencyLabel->Caption =
                "EMERGENCY: " + IntToStr(summary.emergency_count) + " aircraft";
        }
    });

    // Update aircraft positions
    update_manager->setAircraftUpdateCallback([this](const std::vector<AircraftInfo>& aircraft) {
        updateMapAircraft(aircraft);
    });

    // Emergency alerts
    update_manager->setEmergencyCallback([this](const std::vector<EmergencyAlert>& alerts) {
        for (const auto& alert : alerts) {
            showEmergencyNotification(alert);
        }
    });
}
```

### 2.4 Map Integration
```cpp
void TMainForm::updateMapAircraft(const std::vector<AircraftInfo>& aircraft) {
    // Clear existing markers
    clearAircraftMarkers();

    // Add new aircraft markers
    for (const auto& ac : aircraft) {
        if (ac.latitude != 0 && ac.longitude != 0) {
            // Determine marker color
            TColor marker_color = clBlue;
            if (ac.emergency) marker_color = clRed;
            else if (ac.alert) marker_color = clOrange;
            else if (ac.on_ground) marker_color = clGray;

            // Add marker to map
            addAircraftMarker(ac.latitude, ac.longitude, ac.hex_ident,
                            ac.callsign, marker_color);
        }
    }
}
```

### 2.5 Aircraft Click Event Handling
```cpp
void __fastcall TMainForm::AircraftMarkerClick(const std::string& hex_ident) {
    // Fetch detailed info from BigQuery
    AircraftInfo detail = analytics_client->getAircraftDetail(hex_ident);

    // Show detail panel
    showAircraftDetailPanel(detail);
}
```

## 3. Real-Time Features

### 3.1 Automatic Screen Updates
- Update traffic summary every 5 seconds
- Update aircraft positions every 2 seconds
- Check for emergencies every 1 second

### 3.2 Regional Filtering
- Display only aircraft within the current map view
- Automatic filtering based on zoom level

### 3.3 Anomaly Detection
- Sudden altitude changes
- Emergency codes (7500, 7600, 7700)
- High traffic density alerts

## 4. UI Improvement Suggestions

### 4.1 Adding New Panels
- **Analytics Summary Panel**: Real-time statistics
- **Emergency Alert Panel**: Emergency notifications
- **Traffic Density Overlay**: Traffic density heatmap

### 4.2 Alert System
- Popup notifications
- Sound alerts
- Status bar indicator

## 5. Performance Optimization

### 5.1 Caching
- Cache API responses for 30 seconds
- Cache regional data

### 5.2 Update Optimization
- Update only the visible map area
- Asynchronous processing in the background
"""

        guide_path = project_root / "docs" / "DisplayGUI_Integration_Guide.md"
        with open(guide_path, 'w', encoding='utf-8') as f:
            f.write(guide_content)

        print(f"✅ DisplayGUI integration guide created: {guide_path}")
        return guide_path

    def start_all_services(self):
        """Start all services"""
        print("🚀 Starting DisplayGUI BigQuery integration system")
        print("=" * 60)

        success_count = 0
        total_services = 3

        # 1. Start data streaming
        if self.start_data_streaming():
            success_count += 1

        # 2. Start Analytics API server
        if self.start_analytics_api_server():
            success_count += 1

        # 3. Start real-time alert system
        if self.start_alert_system():
            success_count += 1

        # 4. Create configuration file
        self.create_displaygui_config()

        # 5. Create integration guide
        self.generate_displaygui_integration_guide()

        print(f"\n📊 Startup complete: {success_count}/{total_services} services")

        if success_count == total_services:
            print("✅ All services started successfully!")
            print("\n🔗 DisplayGUI integration ready:")
            print("   - Analytics API: http://localhost:8080")
            print("   - WebSocket Alerts: ws://localhost:8081")
            print("   - Configuration file: config/displaygui_integration.json")
            print("   - Integration guide: docs/DisplayGUI_Integration_Guide.md")

            # Run integration tests
            self.test_integration()

            self.is_running = True
            return True
        else:
            print("❌ Some services failed to start.")
            return False

    def stop_all_services(self):
        """Stop all services"""
        print("\n🛑 Stopping all services...")

        if self.api_server_process:
            self.api_server_process.terminate()
            print("✅ Analytics API server stopped")

        if self.alert_system_process:
            self.alert_system_process.terminate()
            print("✅ Real-time alert system stopped")

        if self.streaming_process:
            self.streaming_process.terminate()
            print("✅ Data streaming stopped")

        self.is_running = False
        print("✅ All services have been stopped.")


def main():
    """Main execution function"""
    manager = DisplayGUIIntegrationManager()

    try:
        if manager.start_all_services():
            print("\n⌨️  Press Ctrl+C to stop all services...")
            while manager.is_running:
                time.sleep(1)
    except KeyboardInterrupt:
        print("\n\n🛑 Stopped by user.")
    finally:
        manager.stop_all_services()


if __name__ == "__main__":
    main()
