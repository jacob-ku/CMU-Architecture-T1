#!/usr/bin/env python3
"""
Analysis 2: Detailed Aircraft Analysis
Analyze flight patterns, altitude changes, and location tracking for each aircraft
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
    except Exception as e:
        print(f"❌ Failed to load configuration file: {e}")
        return None


def analyze_aircraft_details():
    """Detailed aircraft analysis"""
    print("✈️  2. Detailed aircraft analysis")
    print("=" * 60)

    config = load_config()
    if not config:
        return

    try:
        client = bigquery.Client(project=config['project_id'])

        print("\n📊 1. Top 10 Most Active Aircraft")
        print("-" * 50)

        top_aircraft_query = f"""
        SELECT
            hex_ident,
            callsign,
            COUNT(*) as message_count,
            MIN(received_at) as first_seen,
            MAX(received_at) as last_seen
        FROM `{config['project_id']}.{config['dataset_id']}.RealTimeStream`
        WHERE hex_ident IS NOT NULL
        GROUP BY hex_ident, callsign
        HAVING COUNT(*) >= 3
        ORDER BY message_count DESC
        LIMIT 10
        """

        results = client.query(top_aircraft_query).result()

        for i, row in enumerate(results, 1):
            callsign = row.callsign if row.callsign else "N/A"
            print(f"{i:2d}. {row.hex_ident} ({callsign})")
            print(f"    📡 Messages: {row.message_count:,}")
            print(f"    🕐 First seen: {row.first_seen}")
            print(f"    🕐 Last seen: {row.last_seen}")
            print()

        print("\n📈 2. Altitude Distribution of Aircraft")
        print("-" * 40)

        altitude_distribution_query = f"""
        SELECT
            CASE
                WHEN altitude < 1000 THEN '0-1K ft'
                WHEN altitude < 5000 THEN '1K-5K ft'
                WHEN altitude < 10000 THEN '5K-10K ft'
                WHEN altitude < 20000 THEN '10K-20K ft'
                WHEN altitude < 30000 THEN '20K-30K ft'
                WHEN altitude < 40000 THEN '30K-40K ft'
                WHEN altitude >= 40000 THEN '40K+ ft'
                ELSE 'Unknown'
            END as altitude_range,
            COUNT(DISTINCT hex_ident) as unique_aircraft,
            COUNT(*) as total_messages,
            AVG(altitude) as avg_altitude
        FROM `{config['project_id']}.{config['dataset_id']}.RealTimeStream`
        WHERE altitude IS NOT NULL AND altitude > 0
        GROUP BY altitude_range
        ORDER BY avg_altitude
        """

        altitude_results = client.query(altitude_distribution_query).result()

        for row in altitude_results:
            print(f"{row.altitude_range:<12} | {row.unique_aircraft:4d} aircraft | {row.total_messages:6,} messages | Avg: {row.avg_altitude:6.0f}ft")

        print("\n🏷️  3. Callsign Pattern Analysis")
        print("-" * 35)

        callsign_pattern_query = f"""
        SELECT
            SUBSTR(callsign, 1, 3) as airline_code,
            COUNT(DISTINCT hex_ident) as unique_aircraft,
            COUNT(*) as total_messages,
            COUNT(DISTINCT callsign) as unique_callsigns
        FROM `{config['project_id']}.{config['dataset_id']}.RealTimeStream`
        WHERE callsign IS NOT NULL
        AND LENGTH(callsign) >= 3
        AND REGEXP_CONTAINS(callsign, r'^[A-Z]{{3}}[0-9]')
        GROUP BY airline_code
        HAVING COUNT(DISTINCT hex_ident) >= 2
        ORDER BY unique_aircraft DESC
        LIMIT 15
        """

        callsign_results = client.query(callsign_pattern_query).result()

        for row in callsign_results:
            print(f"{row.airline_code} | {row.unique_aircraft:3d} aircraft | {row.unique_callsigns:3d} distinct callsigns | {row.total_messages:6,} messages")

        print("\n🛩️  4. Detailed flight trajectory analysis (Most Active Aircraft)")
        print("-" * 55)

        # Select the most active aircraft
        top_aircraft_simple = f"""
        SELECT hex_ident, callsign
        FROM `{config['project_id']}.{config['dataset_id']}.RealTimeStream`
        WHERE hex_ident IS NOT NULL
        GROUP BY hex_ident, callsign
        ORDER BY COUNT(*) DESC
        LIMIT 1
        """

        top_result = list(client.query(top_aircraft_simple).result())[0]
        selected_hex = top_result.hex_ident
        selected_callsign = top_result.callsign if top_result.callsign else "N/A"

        print(f"Target aircraft: {selected_hex} ({selected_callsign})")

        # Flight trajectory of the selected aircraft
        trajectory_query = f"""
        SELECT
            received_at,
            latitude,
            longitude,
            altitude,
            ground_speed,
            track,
            vertical_rate
        FROM `{config['project_id']}.{config['dataset_id']}.RealTimeStream`
        WHERE hex_ident = '{selected_hex}'
        AND latitude IS NOT NULL
        AND longitude IS NOT NULL
        ORDER BY received_at
        """

        trajectory_results = list(client.query(trajectory_query).result())

        if trajectory_results:
            print(f"📍 Total data points: {len(trajectory_results):,}")

            # Altitude analysis
            altitudes = [r.altitude for r in trajectory_results if r.altitude and r.altitude > 0]
            if altitudes:
                print(f"🏔️  Altitude range: {min(altitudes):,}ft ~ {max(altitudes):,}ft")
                print(f"📊 Avg altitude: {sum(altitudes)/len(altitudes):,.0f}ft")

            # Speed analysis
            speeds = [r.ground_speed for r in trajectory_results if r.ground_speed and r.ground_speed > 0]
            if speeds:
                print(f"💨 Speed range: {min(speeds):,}kt ~ {max(speeds):,}kt")
                print(f"📊 Avg speed: {sum(speeds)/len(speeds):.0f}kt")

            # Time analysis
            first_time = trajectory_results[0].received_at
            last_time = trajectory_results[-1].received_at
            print(f"🕐 Tracking start: {first_time}")
            print(f"🕐 Tracking end: {last_time}")

            # Location samples (start, middle, end)
            print("\n📍 Location samples:")
            positions_to_show = [0, len(trajectory_results)//2, -1]
            for i in positions_to_show:
                pos = trajectory_results[i]
                time_label = "Start" if i == 0 else "Middle" if i == len(trajectory_results)//2 else "End"
                alt_str = f"{pos.altitude:,}ft" if pos.altitude else "N/A"
                speed_str = f"{pos.ground_speed:,}kt" if pos.ground_speed else "N/A"
                print(f"   {time_label}: ({pos.latitude:.4f}, {pos.longitude:.4f}) | {alt_str} | {speed_str}")

        # 5. Data quality summary
        print("\n📋 5. Data Quality Summary")
        print("-" * 30)

        quality_query = f"""
        SELECT
            COUNT(*) as total_messages,
            COUNT(DISTINCT hex_ident) as total_aircraft,
            COUNT(CASE WHEN callsign IS NOT NULL AND callsign != '' THEN 1 END) as messages_with_callsign,
            COUNT(CASE WHEN altitude IS NOT NULL AND altitude > 0 THEN 1 END) as messages_with_altitude,
            COUNT(CASE WHEN latitude IS NOT NULL AND longitude IS NOT NULL THEN 1 END) as messages_with_position,
            COUNT(CASE WHEN ground_speed IS NOT NULL AND ground_speed > 0 THEN 1 END) as messages_with_speed
        FROM `{config['project_id']}.{config['dataset_id']}.RealTimeStream`
        """

        quality_result = list(client.query(quality_query).result())[0]

        total = quality_result.total_messages
        print(f"Total messages: {total:,}")
        print(f"Total aircraft: {quality_result.total_aircraft:,}")
        print(f"Callsign reporting rate: {(quality_result.messages_with_callsign/total)*100:.1f}%")
        print(f"Altitude reporting rate: {(quality_result.messages_with_altitude/total)*100:.1f}%")
        print(f"Position reporting rate: {(quality_result.messages_with_position/total)*100:.1f}%")
        print(f"Speed reporting rate: {(quality_result.messages_with_speed/total)*100:.1f}%")

    except Exception as e:
        print(f"❌ Error during analysis execution: {e}")


def main():
    """Main execution function"""
    print("🛩️  ADS-B BigQuery Data Analysis")
    print("=" * 70)
    print(f"Analysis time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print("=" * 70)

    analyze_aircraft_details()

    print("\n✅ Analysis 2 is complete!")


if __name__ == "__main__":
    main()
