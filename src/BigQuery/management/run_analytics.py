#!/usr/bin/env python3
"""
ADS-B data analysis query execution script
Usage: python run_analytics.py [analysis_type]
"""

import sys
import json
from pathlib import Path

# Add project path to sys.path
project_root = Path(__file__).parent.parent.parent
sys.path.append(str(project_root))

try:
    from src.BigQuery.core.AnalyticsQueries import ADSBAnalytics
    from google.cloud import bigquery
except ImportError as e:
    print(f"Import Error: {e}")
    print("Please install required packages:")
    print("pip install google-cloud-bigquery")
    sys.exit(1)


def load_config(config_path: str = "config/config.json") -> dict:
    """Load configuration file"""
    try:
        with open(config_path, 'r', encoding='utf-8') as f:
            return json.load(f)
    except FileNotFoundError:
        print(f"Configuration file {config_path} not found.")
        return {
            "bigquery": {
                "project_id": "scs-lg-arch-1",
                "dataset_id": "SBS_Data"
            }
        }
    except json.JSONDecodeError as e:
        print(f"Configuration file parsing error: {e}")
        sys.exit(1)


def run_korea_traffic_analysis(analytics: ADSBAnalytics):
    """Analyze air traffic over Korea"""
    print("📍 Real-time Traffic Analysis over Korea")
    print("-" * 40)

    query = analytics.get_real_time_traffic_density(
        lat_min=33.0, lat_max=39.0,  # Korea latitude range
        lon_min=125.0, lon_max=132.0  # Korea longitude range
    )

    print("🔍 Query to execute:")
    print(query[:500] + "..." if len(query) > 500 else query)
    print("\n" + "=" * 50)

    try:
        client = bigquery.Client()
        results = list(client.query(query))

        if results:
            print(f"✅ Aircraft found in {len(results)} grids")
            for i, row in enumerate(results[:10]):  # Show top 10 only
                print(f"{i+1:2d}. Lat {row.lat_grid:6.1f}, "
                      f"Lon {row.lon_grid:7.1f} - "
                      f"{row.aircraft_count:2d} aircraft, "
                      f"avg altitude {row.avg_altitude:6.0f}ft")
        else:
            print("ℹ️  No data available over Korea currently.")

    except Exception as e:
        print(f"❌ Query execution error: {e}")


def run_emergency_detection(analytics: ADSBAnalytics):
    """Emergency situation detection"""
    print("🚨 Emergency Situation Detection Analysis")
    print("-" * 40)

    query = analytics.detect_emergency_situations()

    print("🔍 Query to execute:")
    print(query[:500] + "..." if len(query) > 500 else query)
    print("\n" + "=" * 50)

    try:
        client = bigquery.Client()
        results = list(client.query(query))

        if results:
            print(f"⚠️  {len(results)} emergency situations detected")
            for row in results:
                print(f"- {row.callsign or 'N/A'} ({row.hex_ident}): "
                      f"{row.emergency_type} - "
                      f"{row.signal_count} signals")
        else:
            print("✅ No emergency situations currently.")

    except Exception as e:
        print(f"❌ Query execution error: {e}")


def run_airport_analysis(analytics: ADSBAnalytics):
    """Airport traffic analysis"""
    print("🛬 Incheon Airport Area Traffic Analysis")
    print("-" * 40)

    # Incheon Airport coordinates
    icn_lat, icn_lon = 37.4602, 126.4407

    query = analytics.analyze_airport_approach_patterns(
        airport_lat=icn_lat,
        airport_lon=icn_lon,
        radius_km=50
    )

    print("🔍 Query to execute:")
    print(query[:500] + "..." if len(query) > 500 else query)
    print("\n" + "=" * 50)

    try:
        client = bigquery.Client()
        results = list(client.query(query))

        if results:
            print(f"✈️  {len(results)} aircraft active around Incheon Airport")
            for i, row in enumerate(results[:10]):
                print(f"{i+1:2d}. {row.callsign or 'N/A'} - "
                      f"{row.flight_pattern}, "
                      f"closest distance {row.closest_distance:.1f}km, "
                      f"lowest altitude {row.lowest_altitude:4.0f}ft")
        else:
            print("ℹ️  No data available around Incheon Airport currently.")

    except Exception as e:
        print(f"❌ Query execution error: {e}")


def run_pattern_analysis(analytics: ADSBAnalytics):
    """Flight pattern analysis"""
    print("📊 Hourly Flight Pattern Analysis")
    print("-" * 40)

    query = analytics.get_flight_patterns_by_hour(days_back=7)

    print("🔍 Query to execute:")
    print(query[:500] + "..." if len(query) > 500 else query)
    print("\n" + "=" * 50)

    try:
        client = bigquery.Client()
        results = list(client.query(query))

        if results:
            print(f"📈 Pattern data for {len(results)} time periods")
            print("\nHour | Day | Aircraft | Avg Altitude | Avg Speed")
            print("-" * 45)
            for row in results[:20]:  # Top 20
                print(f"{row.hour_of_day:2d}h | "
                      f"{row.day_of_week:2d}d | "
                      f"{row.unique_aircraft:6d} | "
                      f"{row.avg_altitude:6.0f}ft | "
                      f"{row.avg_speed:5.0f}kt")
        else:
            print("ℹ️  Insufficient data for analysis.")

    except Exception as e:
        print(f"❌ Query execution error: {e}")


def show_help():
    """Display help information"""
    print("📚 ADS-B Analysis Tool Usage")
    print("=" * 40)
    print("python run_analytics.py [analysis_type]")
    print("\nAvailable analysis types:")
    print("  korea     - Traffic analysis over Korea")
    print("  emergency - Emergency situation detection")
    print("  airport   - Incheon Airport traffic analysis")
    print("  pattern   - Hourly flight patterns")
    print("  all       - Run all analyses")
    print("\nExamples:")
    print("  python run_analytics.py korea")
    print("  python run_analytics.py all")


def main():
    print("📊 ADS-B BigQuery Analysis Tool")
    print("=" * 50)

    # Check analysis type
    analysis_type = sys.argv[1] if len(sys.argv) > 1 else "help"

    if analysis_type == "help":
        show_help()
        return

    # Load configuration
    config_data = load_config()
    bigquery_config = config_data.get("bigquery", {})

    project_id = bigquery_config.get("project_id", "scs-lg-arch-1")
    dataset_id = bigquery_config.get("dataset_id", "SBS_Data")

    print(f"Project: {project_id}")
    print(f"Dataset: {dataset_id}")
    print("-" * 50)

    # Create Analytics object
    analytics = ADSBAnalytics(project_id, dataset_id)

    # Execute analysis
    try:
        if analysis_type == "korea":
            run_korea_traffic_analysis(analytics)
        elif analysis_type == "emergency":
            run_emergency_detection(analytics)
        elif analysis_type == "airport":
            run_airport_analysis(analytics)
        elif analysis_type == "pattern":
            run_pattern_analysis(analytics)
        elif analysis_type == "all":
            run_korea_traffic_analysis(analytics)
            print("\n")
            run_emergency_detection(analytics)
            print("\n")
            run_airport_analysis(analytics)
            print("\n")
            run_pattern_analysis(analytics)
        else:
            print(f"❌ Unknown analysis type: {analysis_type}")
            show_help()

    except Exception as e:
        print(f"❌ Error during analysis execution: {e}")
        print("\nPlease check the following:")
        print("1. BigQuery tables are created")
        print("2. Sufficient data is available")
        print("3. Google Cloud authentication is correct")


if __name__ == "__main__":
    main()
