#!/usr/bin/env python3
"""
ADS-B BigQuery System Architecture Visualization Tool
Displays system structure in text-based format.
"""

import os
from datetime import datetime

def print_module_view():
    """Print module view"""
    print("📦 MODULE VIEW - System Structure")
    print("=" * 80)
    print("""
📁 src/BigQuery/                          # BigQuery Integration Package
├── 🔧 core/                             # Core Functionality
│   ├── RealTimeStreaming.py             # Real-time data streaming
│   ├── ADSBBridge.py                    # ADS-B data bridge
│   ├── AnalyticsAPIServer.py            # REST API server (Port 8080)
│   ├── RealTimeAlertSystem.py           # WebSocket alerts (Port 8081)
│   ├── AnalyticsQueries.py              # BigQuery query templates
│   └── SetupBigQuery.py                 # BigQuery setup utilities
│
├── 📊 analyses/                         # Analysis Scripts
│   ├── analysis_01_traffic_summary.py   # Traffic summary
│   ├── analysis_02_aircraft_details.py  # Aircraft detail analysis
│   ├── analysis_03_geographic.py        # Geographic distribution analysis
│   ├── analysis_04_trajectory_anomaly.py # Trajectory anomaly detection
│   ├── analysis_05_emergency.py         # Emergency detection
│   └── analysis_06_prediction.py        # Predictive analysis
│
├── ⚙️ management/                       # System Management
│   ├── start_streaming.py               # Start streaming
│   ├── check_data_status.py             # Check data status
│   ├── run_all_analyses.py              # Batch analysis execution
│   └── displaygui_integration.py        # DisplayGUI integration manager
│
├── ⚙️ config/                           # Configuration
│   ├── config.json                      # BigQuery configuration
│   └── YourJsonFile.json                # Service account authentication
│
└── 📚 docs/                             # Documentation
    ├── README.md                        # Project overview
    ├── ANALYSIS_GUIDE.md                # Analysis guide
    └── ARCHITECTURE_VIEWS.md            # Architecture documentation

🖥️ DisplayGUI (C++)                     # External System
├── DisplayGUI.cpp/.h                   # Main GUI application
├── MapRenderer                         # Map rendering engine
└── AircraftTracker                     # Aircraft tracking logic
    """)

def print_runtime_view():
    """Print runtime view"""
    print("\n🔄 RUNTIME VIEW - Execution Flow")
    print("=" * 80)
    print("""
🛩️ ADS-B Aircraft
     │ (Radio Signal)
     ▼
📡 dump1090 (Port 30003)
     │ (SBS Messages)
     ▼
🔗 ADSBBridge.py
     │ (Parsed JSON)
     ▼
📤 RealTimeStreaming.py
     │ (Batch Insert)
     ▼
☁️ Google BigQuery
     │
     ├─── (Query) ──▶ 🌐 AnalyticsAPIServer.py (Port 8080)
     │                       │ (REST API)
     │                       ▼
     └─── (Monitor) ──▶ 🚨 RealTimeAlertSystem.py (Port 8081)
                             │ (WebSocket)
                             ▼
🖥️ DisplayGUI.exe ◀─────────────────────┘
     │
     ▼
👨‍✈️ Air Traffic Controller

Real-time data flow:
• Data latency: < 5 seconds
• Batch size: 100 messages
• API cache: 30 seconds
• Update interval: 2-5 seconds
    """)

def print_deployment_view():
    """Print deployment view"""
    print("\n🌐 DEPLOYMENT VIEW - System Deployment")
    print("=" * 80)
    print("""
┌─── Development Machine ────────────────────────────────────┐
│                                                            │
│  🖥️ DisplayGUI Process                                    │
│  ├── DisplayGUI.exe                                       │
│  ├── HTTP Client ──── (8080) ────┐                       │
│  └── WebSocket Client ─ (8081) ──┼──┐                    │
│                                   │  │                    │
│  🐍 Python Environment           │  │                    │
│  ├── Analytics API (Port 8080) ──┘  │                    │
│  ├── Alert System (Port 8081) ──────┘                    │
│  ├── ADS-B Bridge (Port 30003) ─────────┐                │
│  └── Real-Time Streaming                │                │
│                                          │                │
│  💾 Local Cache                         │                │
│  ├── API Response Cache (30s)           │                │
│  └── Alert History                      │                │
│                                          │                │
└─────────────────────────────────────────┼────────────────┘
                                          │
       ┌─── Network ──────────────────────┼────────────────┐
       │                                  │                │
       │  📡 ADS-B Data Source           │                │
       │  ├── dump1090 ──────────────────┘                │
       │  └── SBS Message Stream                           │
       │                                                   │
       └───────────────────────────────────────────────────┘
                                          │ (HTTPS)
       ┌─── Google Cloud Platform ────────┼────────────────┐
       │                                  │                │
       │  🗃️ BigQuery                    │                │
       │  ├── SBS_Data Dataset ───────────┘                │
       │  ├── RealTimeStream Table                         │
       │  ├── AircraftMetadata Table                       │
       │  └── Analytics Engine                             │
       │                                                   │
       │  🔐 Cloud IAM                                     │
       │                                                   │
       └───────────────────────────────────────────────────┘
    """)

def print_data_flow():
    """Print data flow"""
    print("\n📊 DATA FLOW - Data Flow")
    print("=" * 80)
    print("""
1️⃣ Data Collection
ADS-B Signal → SBS Message → Parsed JSON → Validated Data

2️⃣ Data Storage
Batch Messages → BigQuery Insert → Schema Validation → Table Storage

3️⃣ Real-time Analysis
BigQuery ← SQL Queries ← Analytics API ← HTTP Requests ← DisplayGUI

4️⃣ Event Detection
BigQuery → Monitor Queries → Alert Detection → WebSocket Push → DisplayGUI

5️⃣ Batch Analysis
BigQuery → Statistical Queries → Analysis Results → Reports

Data characteristics:
📈 Throughput: ~1000 messages/minute
📦 Batch size: 100 messages
⏱️ Real-time latency: < 5 seconds
💾 Cache TTL: 30 seconds
🔄 Update interval: 2-5 seconds
    """)

def print_api_interfaces():
    """Print API interfaces"""
    print("\n🌐 API INTERFACES - Interface Definitions")
    print("=" * 80)
    print("""
REST API Endpoints (Port 8080):
┌─────────────────────────────────┬──────────────────────────────────┐
│ Endpoint                        │ Description                      │
├─────────────────────────────────┼──────────────────────────────────┤
│ GET /api/health                 │ Server health check              │
│ GET /api/traffic/summary        │ Real-time traffic summary        │
│ GET /api/aircraft/area          │ Regional aircraft search         │
│ GET /api/aircraft/{hex_ident}   │ Individual aircraft details      │
│ GET /api/alerts/emergency       │ Emergency alerts                 │
└─────────────────────────────────┴──────────────────────────────────┘

WebSocket Messages (Port 8081):
┌─────────────────┬────────────────────────────────────────────────┐
│ Message Type    │ Content                                        │
├─────────────────┼────────────────────────────────────────────────┤
│ EMERGENCY       │ Emergency aircraft information                 │
│ TRAJECTORY      │ Trajectory anomaly detection                   │
│ TRAFFIC_DENSITY │ High traffic density warning                   │
│ CONNECTION      │ Connection status information                  │
└─────────────────┴────────────────────────────────────────────────┘

BigQuery Tables:
┌─────────────────┬─────────────────┬──────────────────────────────────┐
│ Table           │ Records         │ Purpose                          │
├─────────────────┼─────────────────┼──────────────────────────────────┤
│ RealTimeStream  │ ~1M+ messages   │ Real-time ADS-B message storage  │
│ AircraftMetadata│ ~50K aircraft   │ Aircraft info (model, airline)   │
│ FlightTracks    │ Generated       │ Flight path tracking data        │
└─────────────────┴─────────────────┴──────────────────────────────────┘
    """)

def print_system_status():
    """Print current system status"""
    print("\n📊 SYSTEM STATUS - Current System Status")
    print("=" * 80)

    # Check actual file existence
    bigquery_path = "/Users/jacob/ThinQCloud/Education/250000_Architect/3_CMU/CMU-Architecture-T1/src/BigQuery"

    modules = [
        ("Core Modules", "core", ["RealTimeStreaming.py", "ADSBBridge.py", "AnalyticsAPIServer.py"]),
        ("Analysis Modules", "analyses", ["analysis_01_traffic_summary.py", "analysis_02_aircraft_details.py"]),
        ("Management", "management", ["start_streaming.py", "check_data_status.py", "displaygui_integration.py"]),
        ("Configuration", "config", ["config.json"]),
        ("Documentation", "docs", ["README.md", "ANALYSIS_GUIDE.md", "ARCHITECTURE_VIEWS.md"])
    ]

    for module_name, folder, files in modules:
        print(f"\n📁 {module_name}:")
        for file in files:
            file_path = os.path.join(bigquery_path, folder, file)
            if os.path.exists(file_path):
                size = os.path.getsize(file_path)
                status = f"✅ {file} ({size:,} bytes)"
            else:
                status = f"❌ {file} (Missing)"
            print(f"   {status}")

def main():
    """Main execution function"""
    print("🛩️ ADS-B BigQuery Integration System")
    print("=" * 80)
    print(f"📅 Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print("🏗️ Architecture Views & System Overview")

    print_module_view()
    print_runtime_view()
    print_deployment_view()
    print_data_flow()
    print_api_interfaces()
    print_system_status()

    print("\n" + "=" * 80)
    print("📖 For detailed architecture documentation:")
    print("   docs/ARCHITECTURE_VIEWS.md")
    print("📊 For PlantUML diagrams:")
    print("   docs/diagrams/*.puml")
    print("🔗 For DisplayGUI integration:")
    print("   docs/DisplayGUI_Integration_Guide.md")
    print("=" * 80)

if __name__ == "__main__":
    main()
