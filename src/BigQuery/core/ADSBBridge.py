"""
Python bridge for integrating C++ ADS-B Display GUI with BigQuery
Handles real-time data collection and BigQuery transmission
"""

import subprocess
import threading
import time
import json
import os
import socket
from datetime import datetime
from typing import Optional, Dict, List
from dataclasses import dataclass
from src.BigQuery.core.RealTimeStreaming import ADSBStreamer
from src.BigQuery.core.AnalyticsQueries import ADSBAnalytics


@dataclass
class ADSBConfig:
    """ADS-B system configuration"""
    project_id: str = "scs-lg-arch-1"
    dataset_id: str = "SBS_Data"
    table_id: str = "RealTimeStream"
    credentials_path: str = "YourJsonFile.json"
    sbs_port: int = 30003
    batch_size: int = 100
    enable_analytics: bool = True
    log_level: str = "INFO"


class ADSBBridge:
    """Data bridge between C++ GUI and BigQuery"""

    def __init__(self, config: ADSBConfig):
        self.config = config
        self.streamer = None
        self.analytics = None
        self.is_running = False
        self.message_count = 0
        self.start_time = None

        # Log file setup
        self.log_file = f"adsb_bridge_{datetime.now().strftime('%Y%m%d_%H%M%S')}.log"

    def initialize(self) -> bool:
        """System initialization"""
        try:
            self.log("Initializing ADS-B Bridge...")

            # Initialize BigQuery streamer
            try:
                self.streamer = ADSBStreamer(
                    project_id=self.config.project_id,
                    dataset_id=self.config.dataset_id,
                    table_id=self.config.table_id,
                    credentials_path=self.config.credentials_path,
                    batch_size=self.config.batch_size
                )
            except Exception as streamer_error:
                self.log(f"BigQuery connection failed: {streamer_error}",
                         level="ERROR")
                self.log("Please try the following:", level="INFO")
                self.log("1. gcloud auth application-default login",
                         level="INFO")
                self.log("2. Set up correct service account key file",
                         level="INFO")
                self.log("3. Verify BigQuery API is enabled", level="INFO")
                return False

            # Initialize analytics module
            if self.config.enable_analytics:
                self.analytics = ADSBAnalytics(
                    project_id=self.config.project_id,
                    dataset_id=self.config.dataset_id
                )

            self.log("ADS-B Bridge initialized successfully")
            return True

        except Exception as e:
            self.log(f"Failed to initialize: {e}", level="ERROR")
            return False

    def start(self):
        """Start the bridge"""
        if not self.is_running:
            self.is_running = True
            self.start_time = datetime.now()
            self.message_count = 0

            # Start BigQuery streaming
            if self.streamer:
                self.streamer.start_streaming()

            # Start receiving SBS messages from C++ process
            self._start_message_processing()

            self.log("ADS-B Bridge started")

    def stop(self):
        """Stop the bridge"""
        if self.is_running:
            self.is_running = False

            # Stop BigQuery streaming
            if self.streamer:
                self.streamer.stop_streaming()

            runtime = (datetime.now() - self.start_time
                      if self.start_time else None)
            self.log(f"ADS-B Bridge stopped. Runtime: {runtime}, "
                     f"Messages: {self.message_count}")

    def _start_message_processing(self):
        """Start message processing thread"""
        processing_thread = threading.Thread(target=self._process_sbs_messages)
        processing_thread.daemon = True
        processing_thread.start()

    def _process_sbs_messages(self):
        """SBS message processing loop"""
        self.log("Starting SBS message processing...")

        # In actual implementation, receive SBS messages from C++ GUI
        # Here we use dummy data for simulation
        while self.is_running:
            try:
                # Get SBS message from C++ DisplayGUI
                # In reality, communicate through Named Pipe or Socket
                sbs_message = self._get_sbs_message_from_cpp()

                if sbs_message and self.streamer:
                    # Debug only the first message
                    if self.message_count == 0:
                        self.streamer.debug_sbs_message(sbs_message)

                    self.streamer.add_message(sbs_message)
                    self.message_count += 1

                    # Log every 1000 messages
                    if self.message_count % 1000 == 0:
                        queue_size = self.streamer.get_queue_size()
                        self.log(f"Processed {self.message_count} messages, "
                                f"Queue: {queue_size}")

                time.sleep(0.01)  # 100ms interval

            except Exception as e:
                self.log(f"Error processing SBS message: {e}", level="ERROR")
                time.sleep(1)

    def _get_sbs_message_from_cpp(self) -> Optional[str]:
        """Receive SBS messages from real ADS-B data source"""
        try:
            # Connect to real ADS-B data source (128.237.96.41:5002)
            import socket

            # Create new socket if none exists
            if not hasattr(self, '_adsb_socket') or self._adsb_socket is None:
                self._connect_to_adsb_source()

            if self._adsb_socket:
                try:
                    # Set timeout for non-blocking mode
                    self._adsb_socket.settimeout(0.1)
                    data = self._adsb_socket.recv(1024)
                    if data:
                        message = data.decode('utf-8').strip()
                        # Multiple lines may come, return first complete line
                        lines = message.split('\n')
                        for line in lines:
                            if line.startswith('MSG'):
                                return line
                except socket.timeout:
                    # Timeout is normal (when no data available)
                    pass
                except Exception as e:
                    self.log(f"Socket receive error: {e}", level="ERROR")
                    # Retry connection
                    self._reconnect_adsb_source()

            return None

        except Exception as e:
            self.log(f"ADS-B data receive failed: {e}", level="ERROR")
            return None

    def _connect_to_adsb_source(self):
        """Connect to ADS-B data source"""
        try:
            self._adsb_socket = socket.socket(socket.AF_INET,
                                             socket.SOCK_STREAM)
            self._adsb_socket.connect(('128.237.96.41', 5002))
            self.log("ADS-B data source connected: 128.237.96.41:5002",
                     level="INFO")
        except Exception as e:
            self.log(f"ADS-B data source connection failed: {e}",
                     level="ERROR")
            self._adsb_socket = None

    def _reconnect_adsb_source(self):
        """Reconnect to ADS-B data source"""
        try:
            if hasattr(self, '_adsb_socket') and self._adsb_socket:
                self._adsb_socket.close()
        except Exception:
            pass

        self._adsb_socket = None
        self._connect_to_adsb_source()

    def get_analytics_summary(self) -> Dict:
        """Provide analytics summary information"""
        if not self.analytics:
            return {"error": "Analytics not enabled"}

        try:
            # Real-time statistics
            runtime = (datetime.now() - self.start_time
                      if self.start_time else None)
            queue_size = (self.streamer.get_queue_size()
                         if self.streamer else 0)

            summary = {
                "runtime_seconds": (runtime.total_seconds()
                                   if runtime else 0),
                "total_messages": self.message_count,
                "queue_size": queue_size,
                "messages_per_second": (
                    self.message_count / runtime.total_seconds()
                    if runtime and runtime.total_seconds() > 0 else 0
                ),
                "status": "running" if self.is_running else "stopped"
            }

            return summary

        except Exception as e:
            return {"error": f"Failed to get analytics: {e}"}

    def get_recent_aircraft(self, minutes: int = 10) -> List[Dict]:
        """Get aircraft information from recent N minutes"""
        if not self.analytics:
            return []

        try:
            # Query recent aircraft information from BigQuery
            # In actual implementation, use analytics.client.query()
            return [
                {
                    "hex_ident": "A12345",
                    "callsign": "UAL123",
                    "altitude": 35000,
                    "speed": 450,
                    "latitude": 40.7128,
                    "longitude": -74.0060,
                    "last_seen": "2024-12-30T10:00:00Z"
                }
            ]

        except Exception as e:
            self.log(f"Error getting recent aircraft: {e}", level="ERROR")
            return []

    def detect_nearby_aircraft(self, lat: float, lon: float,
                              radius_km: int = 50) -> List[Dict]:
        """Detect nearby aircraft"""
        if not self.analytics:
            return []

        # In actual implementation, use BigQuery geographical queries
        return [
            {
                "hex_ident": "B67890",
                "callsign": "DAL456",
                "distance_km": 25.5,
                "bearing": 45,
                "altitude": 37000
            }
        ]

    def log(self, message: str, level: str = "INFO"):
        """Record log"""
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        log_message = f"[{timestamp}] {level}: {message}"

        print(log_message)

        # Write to log file
        try:
            with open(self.log_file, "a", encoding="utf-8") as f:
                f.write(log_message + "\n")
        except Exception:
            pass  # Continue even if log file write fails


class ADSBWebAPI:
    """Simple REST interface for web API"""

    def __init__(self, bridge: ADSBBridge, port: int = 8080):
        self.bridge = bridge
        self.port = port

    def start_api_server(self):
        """Start web API server (use Flask etc.)"""
        # In actual implementation, use Flask or FastAPI
        self.bridge.log(f"API server would start on port {self.port}")

        # API endpoints:
        # GET /status - system status
        # GET /aircraft - current aircraft list
        # GET /analytics - analytics data
        # POST /search - regional search


def create_cpp_integration_header():
    """Generate header file for C++ integration"""
    header_content = '''
// ADSBBridge.h - Interface for integration with Python BigQuery bridge

#ifndef ADSBBRIDGE_H
#define ADSBBRIDGE_H

#include <string>
#include <vector>
#include <functional>

struct AircraftInfo {
    std::string hex_ident;
    std::string callsign;
    double latitude;
    double longitude;
    int altitude;
    int ground_speed;
    std::string last_seen;
};

struct AnalyticsSummary {
    int total_messages;
    int queue_size;
    double messages_per_second;
    std::string status;
};

class ADSBBridge {
private:
    bool is_initialized;
    std::string python_script_path;

public:
    ADSBBridge();
    ~ADSBBridge();

    // Initialization and control
    bool Initialize(const std::string& config_file);
    bool Start();
    bool Stop();

    // Data transmission
    void SendSBSMessage(const std::string& sbs_message);
    void SendRawMessage(const std::string& raw_message);

    // Analytics data queries
    AnalyticsSummary GetAnalyticsSummary();
    std::vector<AircraftInfo> GetRecentAircraft(int minutes = 10);
    std::vector<AircraftInfo> GetNearbyAircraft(double lat, double lon, int radius_km = 50);

    // Event callbacks
    void SetEmergencyCallback(std::function<void(const AircraftInfo&)> callback);
    void SetTrafficCallback(std::function<void(int)> callback);
};

#endif // ADSBBRIDGE_H
'''
    return header_content


def main():
    """Main execution function"""
    print("=== ADS-B BigQuery Bridge ===")

    # Load configuration
    config = ADSBConfig()

    # Initialize bridge
    bridge = ADSBBridge(config)

    if not bridge.initialize():
        print("Failed to initialize bridge")
        return

    try:
        # Start bridge
        bridge.start()

        # Start API server (optional)
        api = ADSBWebAPI(bridge)
        api.start_api_server()

        # Main loop
        while True:
            time.sleep(10)

            # Status check
            summary = bridge.get_analytics_summary()
            print(f"Status: {summary}")

            # Recent aircraft information
            aircraft = bridge.get_recent_aircraft()
            print(f"Recent aircraft: {len(aircraft)}")

    except KeyboardInterrupt:
        print("\nShutting down...")
    finally:
        bridge.stop()


if __name__ == "__main__":
    main()
