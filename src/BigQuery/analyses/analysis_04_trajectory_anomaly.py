#!/usr/bin/env python3
"""
Analysis #4: Aircraft Movement Patterns and Anomaly Detection
- Aircraft route diversity, sudden altitude/direction changes,
  abnormal pattern detection
"""

from google.cloud import bigquery
from datetime import datetime
import json
from pathlib import Path


def load_config():
    """Load configuration file"""
    try:
        # Get the parent directory (BigQuery) from current analyses directory
        config_path = Path(__file__).parent.parent / 'config' / 'config.json'
        with open(config_path, 'r', encoding='utf-8') as f:
            config = json.load(f)
        return config['bigquery']
    except (FileNotFoundError, KeyError, json.JSONDecodeError) as e:
        print(f"❌ Failed to load configuration file: {e}")
        return None


def analyze_trajectory_anomaly():
    """Aircraft movement patterns and anomaly detection analysis"""
    print("🚨 Analysis #4: Aircraft Movement Patterns & Anomaly Detection")
    print("=" * 60)

    config = load_config()
    if not config:
        return

    try:
        client = bigquery.Client(project=config['project_id'])

        # 1. Sudden altitude change detection (consecutive messages)
        print("\n🏔️ 1. Sudden Altitude Changes Detection (±3000ft or more)")
        print("-" * 50)

        altitude_jump_query = f"""
        SELECT hex_ident, callsign, received_at, altitude,
               LEAD(altitude) OVER (PARTITION BY hex_ident
                                   ORDER BY received_at) AS next_altitude,
               LEAD(received_at) OVER (PARTITION BY hex_ident
                                      ORDER BY received_at) AS next_time
        FROM `{config['project_id']}.{config['dataset_id']}.RealTimeStream`
        WHERE altitude IS NOT NULL AND altitude > 0
        QUALIFY ABS(next_altitude - altitude) >= 3000
        ORDER BY ABS(next_altitude - altitude) DESC
        LIMIT 10
        """

        results = client.query(altitude_jump_query).result()
        for row in results:
            callsign = row.callsign or 'N/A'
            print(f"{row.hex_ident} | {callsign} | "
                  f"{row.altitude:,}ft → {row.next_altitude:,}ft | "
                  f"{row.received_at} → {row.next_time}")

        # 2. Sudden direction change detection (consecutive messages)
        print("\n🧭 2. Sudden Direction Changes Detection (90° or more)")
        print("-" * 50)

        direction_jump_query = f"""
        SELECT hex_ident, callsign, received_at, track,
               LEAD(track) OVER (PARTITION BY hex_ident
                                ORDER BY received_at) AS next_track,
               LEAD(received_at) OVER (PARTITION BY hex_ident
                                      ORDER BY received_at) AS next_time
        FROM `{config['project_id']}.{config['dataset_id']}.RealTimeStream`
        WHERE track IS NOT NULL AND track >= 0 AND track <= 360
        QUALIFY ABS(next_track - track) >= 90
        ORDER BY ABS(next_track - track) DESC
        LIMIT 10
        """

        results = client.query(direction_jump_query).result()
        for row in results:
            callsign = row.callsign or 'N/A'
            print(f"{row.hex_ident} | {callsign} | "
                  f"{row.track}° → {row.next_track}° | "
                  f"{row.received_at} → {row.next_time}")

        # 3. Sudden speed change detection (100kt or more)
        print("\n💨 3. Sudden Speed Changes Detection (100kt or more)")
        print("-" * 50)

        speed_jump_query = f"""
        SELECT hex_ident, callsign, received_at, ground_speed,
               LEAD(ground_speed) OVER (PARTITION BY hex_ident
                                       ORDER BY received_at) AS next_speed,
               LEAD(received_at) OVER (PARTITION BY hex_ident
                                      ORDER BY received_at) AS next_time
        FROM `{config['project_id']}.{config['dataset_id']}.RealTimeStream`
        WHERE ground_speed IS NOT NULL AND ground_speed > 0
        QUALIFY ABS(next_speed - ground_speed) >= 100
        ORDER BY ABS(next_speed - ground_speed) DESC
        LIMIT 10
        """

        results = client.query(speed_jump_query).result()
        for row in results:
            callsign = row.callsign or 'N/A'
            print(f"{row.hex_ident} | {callsign} | "
                  f"{row.ground_speed}kt → {row.next_speed}kt | "
                  f"{row.received_at} → {row.next_time}")

        # 4. Position anomaly detection (1+ degree lat/lon change)
        print("\n📍 4. Position Anomaly Detection (1+ degree change)")
        print("-" * 50)

        position_jump_query = f"""
        SELECT hex_ident, callsign, received_at, latitude, longitude,
               LEAD(latitude) OVER (PARTITION BY hex_ident
                                   ORDER BY received_at) AS next_lat,
               LEAD(longitude) OVER (PARTITION BY hex_ident
                                    ORDER BY received_at) AS next_lon,
               LEAD(received_at) OVER (PARTITION BY hex_ident
                                      ORDER BY received_at) AS next_time
        FROM `{config['project_id']}.{config['dataset_id']}.RealTimeStream`
        WHERE latitude IS NOT NULL AND longitude IS NOT NULL
        QUALIFY ABS(next_lat - latitude) >= 1
             OR ABS(next_lon - longitude) >= 1
        ORDER BY ABS(next_lat - latitude) DESC,
                 ABS(next_lon - longitude) DESC
        LIMIT 10
        """

        results = client.query(position_jump_query).result()
        for row in results:
            callsign = row.callsign or 'N/A'
            print(f"{row.hex_ident} | {callsign} | "
                  f"({row.latitude:.4f},{row.longitude:.4f}) → "
                  f"({row.next_lat:.4f},{row.next_lon:.4f}) | "
                  f"{row.received_at} → {row.next_time}")

        print("\n✅ Analysis #4 completed successfully!")

    except Exception as e:
        print(f"❌ Error during analysis execution: {e}")


def main():
    """Main execution function"""
    print("🛩️  ADS-B BigQuery Data Analysis")
    print("=" * 70)
    print(f"Analysis time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print("=" * 70)

    analyze_trajectory_anomaly()


if __name__ == "__main__":
    main()
