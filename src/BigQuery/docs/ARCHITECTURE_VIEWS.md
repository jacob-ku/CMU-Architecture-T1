# ADS-B BigQuery Integration - Architecture Views

This document provides various architectural views of the ADS-B BigQuery Integration system.

## Table of Contents
1. [Module View](#module-view)
2. [Runtime View](#runtime-view)
3. [Deployment View](#deployment-view)
4. [Component Interaction View](#component-interaction-view)
5. [Data Flow View](#data-flow-view)
6. [Performance Characteristics](#performance-characteristics)

## Module View

### System Structure
```
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
│   ├── analysis_05_emergency.py         # Emergency situation detection
│   └── analysis_06_prediction.py        # Predictive analysis
│
├── ⚙️ management/                       # System Management
│   ├── start_streaming.py               # Start streaming
│   ├── check_data_status.py             # Check data status
│   ├── run_all_analyses.py              # Batch analysis execution
│   ├── run_analytics.py                 # Interactive analytics tool
│   ├── upload_aircraft_metadata.py      # Upload aircraft metadata
│   ├── clear_bigquery_data.py           # Clear BigQuery data
│   ├── displaygui_integration.py        # DisplayGUI integration manager
│   └── architecture_viewer.py           # System architecture viewer
│
├── ⚙️ config/                           # Configuration
│   ├── config.json                      # BigQuery configuration
│   └── YourJsonFile.json                # Service account authentication
│
└── 📚 docs/                             # Documentation
    ├── README.md                        # Project overview
    ├── ANALYSIS_GUIDE.md                # Analysis guide
    └── ARCHITECTURE_VIEWS.md            # Architecture documentation
```

### Module Dependencies
- **Core Module**: Provides core system functionality
- **Analyses Module**: Data analysis and report generation
- **Management Module**: System operation and management
- **Config Module**: Configuration and authentication information
- **Docs Module**: Documentation and guides

**PlantUML Diagram**: `docs/diagrams/module-view.puml`

## Runtime View

### Execution Flow
```
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
```

### Process Flow
1. **Data Collection**: ADS-B aircraft transmit radio signals
2. **Signal Processing**: dump1090 converts to SBS-1 messages
3. **Data Parsing**: ADSBBridge converts to structured JSON
4. **Batch Processing**: RealTimeStreaming saves to BigQuery in batches of 100
5. **Real-time Analysis**: Analytics API performs real-time queries
6. **Alert System**: Emergency and anomaly situations notified via WebSocket
7. **GUI Display**: DisplayGUI visualizes real-time data

**PlantUML Diagram**: `docs/diagrams/runtime-view.puml`

## Deployment View

### System Deployment Structure
```
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
```

### Deployment Characteristics
- **Development Machine**: Local development and testing environment
- **Network Layer**: ADS-B data sources and network communication
- **Google Cloud Platform**: BigQuery data storage and analysis

**PlantUML Diagram**: `docs/diagrams/deployment-view.puml`

## Component Interaction View

### Component Interactions
Shows how each component in the system interacts:

- **Data Collection Layer**: dump1090, ADSBBridge
- **Data Processing Layer**: RealTimeStreaming, AnalyticsQueries
- **Storage Layer**: Google BigQuery
- **API Layer**: AnalyticsAPIServer, RealTimeAlertSystem
- **Analysis Layer**: Various analysis scripts
- **Management Layer**: System management tools

**PlantUML Diagram**: `docs/diagrams/component-interaction.puml`

## Data Flow View

### Data Flow
```
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
```

**PlantUML Diagram**: `docs/diagrams/data-flow.puml`

## Performance Characteristics

### System Performance Metrics
- **Data Throughput**: ~1000 messages/minute
- **Batch Size**: 100 messages
- **Real-time Latency**: < 5 seconds
- **API Cache TTL**: 30 seconds
- **Update Interval**: 2-5 seconds
- **API Response Time**: < 100ms
- **WebSocket Latency**: < 1 second

### Scalability Considerations
- **Horizontal Scaling**: Support for multiple ADS-B receivers
- **Vertical Scaling**: BigQuery automatic scaling
- **Caching Strategy**: API response caching for performance optimization
- **Load Balancing**: Concurrent support for multiple DisplayGUI clients

## API Interfaces

### REST API Endpoints (Port 8080)
| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/health` | GET | Server health check |
| `/api/traffic/summary` | GET | Real-time traffic summary |
| `/api/aircraft/area` | GET | Regional aircraft search |
| `/api/aircraft/{hex_ident}` | GET | Individual aircraft details |
| `/api/alerts/emergency` | GET | Emergency alerts |

### WebSocket Messages (Port 8081)
| Message Type | Description |
|--------------|-------------|
| `EMERGENCY` | Emergency aircraft information |
| `TRAJECTORY` | Trajectory anomaly detection |
| `TRAFFIC_DENSITY` | High traffic density warning |
| `CONNECTION` | Connection status information |

## System Operations

### Start and Stop
```bash
# Start system
python management/start_streaming.py

# Check status
python management/check_data_status.py

# Run analysis
python management/run_all_analyses.py

# Interactive analytics
python management/run_analytics.py [type]

# Upload metadata
python management/upload_aircraft_metadata.py

# Clear data
python management/clear_bigquery_data.py

# DisplayGUI integration
python management/displaygui_integration.py

# Architecture viewer
python management/architecture_viewer.py
```

### Monitoring
- **Data Status**: Real-time monitoring with `check_data_status.py`
- **System Logs**: Individual log files generated for each component
- **Performance Metrics**: BigQuery query performance and API response time tracking

## Troubleshooting

### Common Issues
1. **BigQuery Connection Failed**: Check authentication files and permissions
2. **dump1090 Connection Failed**: Verify port 30003 availability
3. **API Response Delay**: Check cache settings and query optimization
4. **WebSocket Connection Lost**: Verify network stability and firewall settings

### Log File Locations
- **Streaming Logs**: `adsb_bridge_*.log`
- **API Server Logs**: Console output
- **Analysis Script Logs**: `analyses/logs/`

## Future Development Plans

### Short-term Plans
- [ ] Add performance monitoring dashboard
- [ ] Implement more analysis algorithms
- [ ] Strengthen error handling and recovery mechanisms

### Long-term Plans
- [ ] Cloud-native deployment (Kubernetes)
- [ ] Machine learning-based predictive analysis
- [ ] Multi-region ADS-B data integration
- [ ] Real-time air traffic control system integration

---

## Related Documents
- [README.md](README.md): Project overview and installation guide
- [ANALYSIS_GUIDE.md](ANALYSIS_GUIDE.md): Analysis script usage guide
- [diagrams/](diagrams/): PlantUML architecture diagrams

**Last Updated**: July 2, 2025
