# ADS-B BigQuery Data Analysis Guide

## 📋 Overview

This document provides a comprehensive guide for using analysis scripts to analyze ADS-B data in BigQuery, including usage instructions and analysis content details.

## 🎯 Available Analysis Scripts

- **Analysis 1**: Real-time Traffic Summary
- **Analysis 2**: Aircraft Detail Analysis
- **Analysis 3**: Geographic Distribution & Airport Traffic
- **Analysis 4**: Aircraft Movement Patterns & Anomaly Detection
- **Analysis 5**: Emergency & Special Situation Detection
- **Analysis 6**: Predictive Analysis & Trend Forecasting

---

## 📊 Analysis 1: Real-time Traffic Summary

### File Name
`analyses/analysis_01_traffic_summary.py`

### Execution Method
```bash
cd src/BigQuery
python analyses/analysis_01_traffic_summary.py
```

### Analysis Content
- **Basic Statistics**: Total aircraft count, message count, and active aircraft in the last 5 minutes
- **Altitude Distribution**: Aircraft distribution by altitude (low, medium, high, ultra-high)
- **Speed Distribution**: Aircraft distribution by speed (stationary, low, normal, high)
- **Real-time Status**: Current status of active aircraft

### Output Example
```
📊 Basic Statistics (Last 5 minutes)
--------------------------------------------------
Total Aircraft: 45 | Messages: 2,847 | Active Aircraft: 42

🏔️ Altitude Distribution
--------------------------------------------------
Low Altitude (< 10,000ft)      |  8 aircraft | Avg:  5,234ft
Medium Altitude (10,000-20,000ft) | 12 aircraft | Avg: 15,678ft
High Altitude (20,000-30,000ft)   | 18 aircraft | Avg: 25,432ft
Ultra High Altitude (> 30,000ft)  |  7 aircraft | Avg: 35,890ft
```

### Use Cases
- Overall traffic status monitoring
- Real-time monitoring dashboard base data
- System health checks

---

## ✈️ Analysis 2: Aircraft Detail Analysis

### File Name
`analyses/analysis_02_aircraft_details.py`

### Execution Method
```bash
cd src/BigQuery
python analyses/analysis_02_aircraft_details.py
```

### Analysis Content
- **Active Aircraft**: Top 15 aircraft with highest message counts
- **Altitude Changes**: Analysis of altitude variations by aircraft
- **Speed Analysis**: Average and maximum speed per aircraft
- **Callsign Mapping**: Relationship analysis between hex_ident and callsign

### Output Example
```
📈 Active Aircraft Analysis (By Message Count)
--------------------------------------------------
A12345 | UAL123  | Messages: 156 | Altitude: 35,000ft | Speed: 487kt
B67890 | DAL456  | Messages: 142 | Altitude: 31,000ft | Speed: 456kt
```

### Use Cases
- Individual aircraft tracking
- Flight pattern analysis
- Abnormal behavior aircraft identification

---

## 🗺️ Analysis 3: Geographic Distribution & Airport Traffic

### File Name
`analyses/analysis_03_geographic.py`

### Execution Method
```bash
cd src/BigQuery
python analyses/analysis_03_geographic.py
```

### Analysis Content
- **Regional Distribution**: Aircraft density analysis by latitude/longitude
- **Airport Traffic**: Activity analysis around major airports
- **Altitude Distribution Map**: Regional average altitude analysis
- **Activity Hotspots**: Identification of aircraft activity concentration areas

### Output Example
```
🗺️ Regional Aircraft Distribution
--------------------------------------------------
📍 (40.8, -74.0) | Aircraft: 12 | Avg Altitude: 8,500ft | Activity: High
📍 (41.3, -87.9) | Aircraft:  8 | Avg Altitude: 12,300ft | Activity: Medium
```

### Use Cases
- Airport traffic monitoring
- Route analysis
- Regional aviation activity pattern identification

---

## 🛣️ Analysis 4: Aircraft Movement Patterns & Anomaly Detection

### File Name
`analyses/analysis_04_trajectory_anomaly.py`

### Execution Method
```bash
cd src/BigQuery
python analyses/analysis_04_trajectory_anomaly.py
```

### Analysis Content
- **Altitude Change Anomalies**: Detection of sudden altitude changes
- **Direction Change Anomalies**: Detection of abnormal direction changes
- **Speed Change Anomalies**: Detection of sudden speed changes
- **Position Jumps**: Detection of abnormal position movements
- **Anomaly Pattern Summary**: Comprehensive summary of detected anomalies

### Output Example
```
⚠️ Altitude Change Anomaly Detection
--------------------------------------------------
🚨 A12345 | UAL123 | Altitude Change: +8,500ft | Time: 3min | Level: Critical
⚠️ B67890 | DAL456 | Altitude Change: -2,100ft | Time: 2min | Level: Warning

🔄 Direction Change Anomaly Detection
--------------------------------------------------
🌪️ C11111 | SWA789 | Direction Change: 145° | Speed: 287kt | Level: Sharp Turn
```

### Use Cases
- Emergency situation detection
- Abnormal flight pattern monitoring
- Safety management system

---

## 🚨 Analysis 5: Emergency & Special Situation Detection

### File Name
`analyses/analysis_05_emergency.py`

### Execution Method
```bash
cd src/BigQuery
python analyses/analysis_05_emergency.py
```

### Analysis Content
- **Emergency Situations**: Aircraft with emergency flags set
- **Alert Situations**: Aircraft with alert flags activated
- **Squawk Codes**: Special squawk code monitoring (7500, 7600, 7700)
- **SPI Signals**: Special Position Identification signal detection
- **Ground Status**: Current status of aircraft on ground
- **Security Related**: Security-related special situations

### Output Example
```
🚨 Emergency Situation Detection
--------------------------------------------------
🆘 A12345 | UAL123 | Emergency: True | Altitude: 25,000ft | Speed: 234kt

⚠️ Alert Situations
--------------------------------------------------
🚨 B67890 | DAL456 | Alert: True | Position: (40.7, -74.0) | Time: 14:32

📡 Special Squawk Codes
--------------------------------------------------
🚨 7700 (Emergency) | C11111 | Aircraft: 1
⚠️ 7600 (Radio Failure) | D22222 | Aircraft: 0
```

### Use Cases
- Aviation security monitoring
- Emergency response
- Air traffic control support

---

## 🔮 Analysis 6: Predictive Analysis & Trend Forecasting

### File Name
`analyses/analysis_06_prediction.py`

### Execution Method
```bash
cd src/BigQuery
python analyses/analysis_06_prediction.py
```

### Analysis Content
- **Time Series Patterns**: 5-minute interval traffic pattern analysis
- **Trend Analysis**: Hourly traffic increase/decrease trends
- **Density Prediction**: Aircraft density prediction for next 30 minutes
- **Altitude Patterns**: Traffic stability analysis by altitude
- **Speed Outliers**: Statistical outlier detection
- **Geographic Hotspots**: Activity concentration area prediction
- **Prediction Summary**: Comprehensive prediction results and recommendations

### Output Example
```
📈 Time Series Traffic Pattern Analysis
--------------------------------------------------
⏰  0min: Aircraft 42.3 | Messages 2,847 | Altitude 24,567ft | Volatility 3.2
⏰  5min: Aircraft 45.1 | Messages 3,012 | Altitude 25,123ft | Volatility 2.8

🎯 Aircraft Density Prediction (Next 30 minutes)
--------------------------------------------------
📊 Current Average: 43.7 aircraft/min (±2.4)
📈 Trend: Increasing (Slope: 0.08)
🔮 30min Forecast: 46.1 aircraft/min
🎯 Prediction Confidence: Medium
```

### Use Cases
- Traffic prediction and planning
- Capacity management
- System resource optimization

---

## 🔧 Common Usage Instructions

### 1. Prerequisites
```bash
# Activate virtual environment
source .venv/bin/activate

# Navigate to BigQuery directory
cd src/BigQuery

# Verify real-time streaming is running
python management/check_data_status.py
```

### 2. Individual Analysis Execution
```bash
# Execute each analysis individually
python analyses/analysis_01_traffic_summary.py
python analyses/analysis_02_aircraft_details.py
python analyses/analysis_03_geographic.py
python analyses/analysis_04_trajectory_anomaly.py
python analyses/analysis_05_emergency.py
python analyses/analysis_06_prediction.py
```

### 3. Batch Execution (Optional)
```bash
# Script to execute all analyses sequentially
python management/run_all_analyses.py
```

### 4. Quick Analytics Tool (Interactive)
```bash
# Run specific analysis types
python management/run_analytics.py korea     # Korea airspace traffic
python management/run_analytics.py emergency # Emergency detection
python management/run_analytics.py airport   # Airport traffic
python management/run_analytics.py pattern   # Flight patterns
python management/run_analytics.py all       # All analytics
```

---

## 📝 Analysis Results Application

### Real-time Monitoring
- Execute Analysis 1 and 5 periodically for real-time situation awareness
- Use Analysis 4 for immediate anomaly detection

### Operational Planning
- Use Analysis 6 prediction results for capacity planning
- Use Analysis 3 for regional traffic distribution planning

### Safety Management
- Monitor emergency situations with Analysis 5
- Detect abnormal flight patterns with Analysis 4

### Performance Optimization
- Track individual aircraft with Analysis 2
- Predict system load with Analysis 6

---

## ⚠️ Important Notes

1. **Data Time Range**: Each analysis uses different time ranges (5 minutes to 24 hours)
2. **Execution Time**: Complex queries may take time to execute
3. **Data Quality**: Ensure real-time streaming is operating normally
4. **BigQuery Costs**: Query execution incurs BigQuery usage charges

---

## 🔍 Troubleshooting

### No Data Available
```bash
# Check streaming status
python management/check_data_status.py

# Restart streaming
python management/start_streaming.py
```

### Query Errors
- Verify project/dataset information in config.json
- Check BigQuery permissions
- Verify table schema

### Performance Issues
- Reduce time range for testing
- Add LIMIT clause to restrict result count
- Utilize indexed columns (received_at, hex_ident, etc.)

---

## 📊 Management Tools

### Data Management
```bash
# Upload aircraft metadata
python management/upload_aircraft_metadata.py

# Clear BigQuery data
python management/clear_bigquery_data.py --info  # View table info
python management/clear_bigquery_data.py --confirm  # Clear all data
```

### Integration Tools
```bash
# DisplayGUI integration
python management/displaygui_integration.py

# System architecture viewer
python management/architecture_viewer.py
```
