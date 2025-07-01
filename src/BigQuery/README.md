# ADS-B BigQuery Integration Package

Real-time ADS-B aircraft data streaming and analysis system integrated with Google BigQuery. Supports real-time integration with DisplayGUI.

## 🏗️ Architecture Overview

![System Architecture](docs/diagrams/runtime-view.png)

### Core Features
- 📡 **Real-time ADS-B Data Streaming**: Direct connection from dump1090 to BigQuery
- 🌐 **REST API Server**: Real-time analytics API for DisplayGUI (Port 8080)
- 🚨 **Real-time Alert System**: WebSocket-based emergency and anomaly detection (Port 8081)
- 📊 **Comprehensive Analysis**: Traffic, geographic, trajectory, emergency, and predictive analysis
- 🔧 **Integrated Management**: Automated streaming, status monitoring, and batch analysis

## 📁 Project Structure

This project has a systematically organized directory structure by purpose:

```
src/BigQuery/
├── __init__.py                     # Package initialization
├── core/                          # 🔧 Core functionality
│   ├── __init__.py
│   ├── ADSBBridge.py              # ADS-B data bridge
│   ├── AnalyticsAPIServer.py      # REST API server (Port 8080)
│   ├── AnalyticsQueries.py        # BigQuery analytics queries
│   ├── RealTimeAlertSystem.py     # WebSocket alerts (Port 8081)
│   ├── RealTimeStreaming.py       # Real-time data streaming
│   ├── SetupBigQuery.py           # BigQuery setup utilities
│   └── SimpleCSVtoBigQuery.py     # CSV to BigQuery import
├── analyses/                      # 📊 Analysis scripts
│   ├── __init__.py
│   ├── analysis_01_traffic_summary.py      # Traffic summary
│   ├── analysis_02_aircraft_details.py     # Aircraft details
│   ├── analysis_03_geographic.py           # Geographic analysis
│   ├── analysis_04_trajectory_anomaly.py   # Anomaly detection
│   ├── analysis_05_emergency.py            # Emergency detection
│   └── analysis_06_prediction.py           # Prediction analysis
├── management/                    # System management
│   ├── __init__.py
│   ├── check_data_status.py       # Data status monitoring
│   ├── clear_bigquery_data.py     # Data cleanup
│   ├── run_all_analyses.py        # Batch analysis runner
│   ├── run_analytics.py           # Analytics runner
│   └── start_streaming.py         # Streaming management
├── config/                        # Configuration files
│   ├── __init__.py
│   ├── config.json               # BigQuery configuration
│   └── YourJsonFile.json         # Service account credentials
└── docs/                         # Documentation
    ├── ANALYSIS_GUIDE.md         # Analysis usage guide
    └── README.md                 # Main documentation
```

## 🚀 Quick Start

### 1. Setup
```bash
# Navigate to BigQuery directory
cd src/BigQuery

# Start data streaming
python management/start_streaming.py

# Check data status
python management/check_data_status.py
```

### 2. Run Individual Analysis
```bash
# Traffic summary
python analyses/analysis_01_traffic_summary.py

# Aircraft details
python analyses/analysis_02_aircraft_details.py

# Geographic analysis
python analyses/analysis_03_geographic.py

# Emergency detection
python analyses/analysis_05_emergency.py
```

### 3. Run All Analyses
```bash
# Execute all analyses in sequence
python management/run_all_analyses.py
```

### 4. DisplayGUI Integration
```bash
# Start integrated system for C++ DisplayGUI
python management/displaygui_integration.py

# Or start individual services:
python core/AnalyticsAPIServer.py          # REST API (port 8080)
python core/RealTimeAlertSystem.py         # WebSocket alerts (port 8081)
```

## 📊 Module Overview

### Core Modules (`core/`)
- **RealTimeStreaming.py**: Real-time ADS-B data streaming to BigQuery
- **ADSBBridge.py**: Bridge between ADS-B data sources and BigQuery
- **AnalyticsQueries.py**: Pre-built analytics query templates
- **SetupBigQuery.py**: BigQuery project and dataset setup
- **SimpleCSVtoBigQuery.py**: CSV data import utilities

### Analysis Modules (`analyses/`)
- **analysis_01**: Real-time traffic summary and statistics
- **analysis_02**: Detailed aircraft-specific analysis
- **analysis_03**: Geographic distribution and airport traffic
- **analysis_04**: Trajectory anomaly and pattern detection
- **analysis_05**: Emergency and special situation detection
- **analysis_06**: Predictive analytics and trend forecasting

### Management Modules (`management/`)
- **start_streaming.py**: Initialize and manage data streaming
- **check_data_status.py**: Monitor data pipeline health
- **clear_bigquery_data.py**: Clean up BigQuery tables
- **run_all_analyses.py**: Batch execution of all analyses
- **run_analytics.py**: Advanced analytics runner

### Configuration (`config/`)
- **config.json**: BigQuery project settings and connection parameters
- **YourJsonFile.json**: Service account credentials for BigQuery access

## 📖 Documentation

For detailed usage instructions and analysis explanations, see:
- **[docs/ANALYSIS_GUIDE.md](docs/ANALYSIS_GUIDE.md)**: Comprehensive analysis guide
- **[docs/README.md](docs/README.md)**: Detailed project documentation

## 🔧 Configuration

Before running any scripts, ensure your configuration files are properly set up:

1. **BigQuery Configuration** (`config/config.json`):
   ```json
   {
     "project_id": "your-gcp-project",
     "dataset_id": "adsb_data",
     "table_id": "aircraft_messages"
   }
   ```

2. **Service Account** (`config/YourJsonFile.json`):
   - Google Cloud service account credentials with BigQuery access

## 🎯 Key Features

- **Real-time streaming**: Live ADS-B data ingestion to BigQuery
- **Comprehensive analytics**: 6 different analysis modules
- **Anomaly detection**: Automated detection of unusual flight patterns
- **Emergency monitoring**: Real-time emergency situation alerts
- **Predictive analytics**: Trend analysis and forecasting
- **DisplayGUI integration**: Real-time API and WebSocket services for C++ GUI
- **Modular design**: Easy to extend and customize
- **Package structure**: Proper Python package with imports

## 🚨 System Requirements

- Python 3.8+
- Google Cloud BigQuery access
- ADS-B data source (dump1090 or similar)
- Required Python packages (see requirements in individual modules)

## 📈 Usage Examples

### Monitor Real-time Traffic
```bash
python analyses/analysis_01_traffic_summary.py
```

### Detect Emergency Situations
```bash
python analyses/analysis_05_emergency.py
```

### Run Predictive Analysis
```bash
python analyses/analysis_06_prediction.py
```

### Check System Health
```bash
python management/check_data_status.py
```

## 🔍 Troubleshooting

If you encounter issues:

1. **Check data streaming**: `python management/check_data_status.py`
2. **Verify configuration**: Ensure `config/config.json` is correct
3. **Check BigQuery permissions**: Verify service account has proper access
4. **Review logs**: Check script outputs for specific error messages

## 🤝 Contributing

When adding new features:
- Place core functionality in `core/`
- Add analysis scripts to `analyses/`
- Put management utilities in `management/`
- Update documentation in `docs/`
- Follow the existing naming conventions

---

For detailed analysis instructions, see [docs/ANALYSIS_GUIDE.md](docs/ANALYSIS_GUIDE.md)
