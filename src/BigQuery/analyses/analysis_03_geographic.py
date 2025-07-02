#!/usr/bin/env python3
"""
Analysis #3: Geographic Distribution and Airport Traffic Analysis
Analyzes aircraft regional distribution, high-density areas,
and traffic near major airports
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


def analyze_geographic_distribution():
    """Geographic distribution and airport traffic analysis"""
    print("🌍 Analysis #3: Geographic Distribution & Airport Traffic Analysis")
    print("=" * 60)

    config = load_config()
    if not config:
        return

    try:
        client = bigquery.Client(project=config['project_id'])

        # 1. Overall geographic range check
        print("\n📍 1. Overall Geographic Range")
        print("-" * 40)

        geographic_range_query = f"""
        SELECT
            COUNT(*) as total_positions,
            MIN(latitude) as min_lat,
            MAX(latitude) as max_lat,
            MIN(longitude) as min_lon,
            MAX(longitude) as max_lon,
            AVG(latitude) as avg_lat,
            AVG(longitude) as avg_lon
        FROM `{config['project_id']}.{config['dataset_id']}.RealTimeStream`
        WHERE latitude IS NOT NULL
        AND longitude IS NOT NULL
        AND latitude BETWEEN -90 AND 90
        AND longitude BETWEEN -180 AND 180
        """

        range_result = list(client.query(geographic_range_query).result())[0]

        print(f"📡 Total position points: {range_result.total_positions:,}")
        print(f"📐 Latitude range: {range_result.min_lat:.4f}° ~ "
              f"{range_result.max_lat:.4f}°")
        print(f"📐 Longitude range: {range_result.min_lon:.4f}° ~ "
              f"{range_result.max_lon:.4f}°")
        print(f"🎯 Center point: ({range_result.avg_lat:.4f}°, "
              f"{range_result.avg_lon:.4f}°)")

        # Region estimation
        if range_result.avg_lat > 25 and range_result.avg_lat < 50:
            if range_result.avg_lon > -125 and range_result.avg_lon < -65:
                region = "North America (USA)"
            else:
                region = "Northern Hemisphere"
        else:
            region = "Other Region"
        print(f"🌎 Primary region: {region}")

        # 2. Traffic density by latitude/longitude
        print("\n📊 2. Regional Traffic Density (by Latitude)")
        print("-" * 50)

        latitude_density_query = f"""
        SELECT
            CASE
                WHEN latitude < 25 THEN 'South (<25°N)'
                WHEN latitude < 30 THEN 'Southeast (25-30°N)'
                WHEN latitude < 35 THEN 'South-Central (30-35°N)'
                WHEN latitude < 40 THEN 'Central (35-40°N)'
                WHEN latitude < 45 THEN 'North-Central (40-45°N)'
                WHEN latitude >= 45 THEN 'North (≥45°N)'
                ELSE 'Other'
            END as latitude_zone,
            COUNT(*) as message_count,
            COUNT(DISTINCT hex_ident) as unique_aircraft,
            AVG(altitude) as avg_altitude
        FROM `{config['project_id']}.{config['dataset_id']}.RealTimeStream`
        WHERE latitude IS NOT NULL
        AND longitude IS NOT NULL
        AND latitude BETWEEN -90 AND 90
        GROUP BY latitude_zone
        ORDER BY AVG(latitude) DESC
        """

        lat_results = client.query(latitude_density_query).result()

        for row in lat_results:
            avg_alt = (f"{row.avg_altitude:,.0f}ft" if row.avg_altitude
                       else "N/A")
            print(f"{row.latitude_zone:<20} | {row.message_count:4d} msgs | "
                  f"{row.unique_aircraft:3d} aircraft | Avg alt: {avg_alt}")

        # 3. Traffic distribution by longitude
        print("\n📊 3. East-West Traffic Distribution (by Longitude)")
        print("-" * 50)

        longitude_density_query = f"""
        SELECT
            CASE
                WHEN longitude < -120 THEN 'West Coast'
                WHEN longitude < -100 THEN 'Mountain/West-Central'
                WHEN longitude < -90 THEN 'West-Central'
                WHEN longitude < -80 THEN 'Central'
                WHEN longitude < -70 THEN 'East-Central'
                WHEN longitude >= -70 THEN 'East Coast'
                ELSE 'Other'
            END as longitude_zone,
            COUNT(*) as message_count,
            COUNT(DISTINCT hex_ident) as unique_aircraft,
            AVG(altitude) as avg_altitude
        FROM `{config['project_id']}.{config['dataset_id']}.RealTimeStream`
        WHERE latitude IS NOT NULL
        AND longitude IS NOT NULL
        AND longitude BETWEEN -180 AND 180
        GROUP BY longitude_zone
        ORDER BY AVG(longitude) DESC
        """

        lon_results = client.query(longitude_density_query).result()

        for row in lon_results:
            avg_alt = (f"{row.avg_altitude:,.0f}ft" if row.avg_altitude
                       else "N/A")
            print(f"{row.longitude_zone:<20} | {row.message_count:4d} msgs | "
                  f"{row.unique_aircraft:3d} aircraft | Avg alt: {avg_alt}")

        # 4. High-density area detection (1° x 1° grid)
        print("\n🔥 4. Top 10 High-Density Traffic Areas")
        print("-" * 45)

        hotspot_query = f"""
        SELECT
            ROUND(latitude, 0) as lat_grid,
            ROUND(longitude, 0) as lon_grid,
            COUNT(*) as message_count,
            COUNT(DISTINCT hex_ident) as unique_aircraft,
            AVG(altitude) as avg_altitude
        FROM `{config['project_id']}.{config['dataset_id']}.RealTimeStream`
        WHERE latitude IS NOT NULL
        AND longitude IS NOT NULL
        AND latitude BETWEEN -90 AND 90
        AND longitude BETWEEN -180 AND 180
        GROUP BY lat_grid, lon_grid
        HAVING COUNT(*) >= 10
        ORDER BY message_count DESC
        LIMIT 10
        """

        hotspot_results = client.query(hotspot_query).result()

        print("Lat  | Lon  | Messages | Aircraft | Avg Altitude")
        print("-" * 45)
        for row in hotspot_results:
            avg_alt = (f"{row.avg_altitude:,.0f}ft" if row.avg_altitude
                       else "N/A")
            print(f"{row.lat_grid:4.0f}° | {row.lon_grid:4.0f}° | "
                  f"{row.message_count:4d} msgs | {row.unique_aircraft:3d} "
                  f"acft | {avg_alt}")

        # 5. Traffic near major airports (estimated)
        print("\n✈️  5. Estimated Traffic Near Major Regional Airports")
        print("-" * 50)

        # Major airport coordinates (examples)
        major_airports = [
            ("New York JFK", 40.6413, -73.7781),
            ("Los Angeles LAX", 33.9425, -118.4081),
            ("Chicago ORD", 41.9742, -87.9073),
            ("Dallas DFW", 32.8998, -97.0403),
            ("Denver DEN", 39.8561, -104.6737),
            ("Atlanta ATL", 33.6407, -84.4277),
            ("San Francisco SFO", 37.6213, -122.3790),
            ("Seattle SEA", 47.4502, -122.3088)
        ]

        for airport_name, lat, lon in major_airports:
            airport_traffic_query = f"""
            SELECT
                COUNT(*) as nearby_messages,
                COUNT(DISTINCT hex_ident) as nearby_aircraft
            FROM `{config['project_id']}.{config['dataset_id']}.RealTimeStream`
            WHERE latitude IS NOT NULL
            AND longitude IS NOT NULL
            AND ABS(latitude - {lat}) <= 2.0
            AND ABS(longitude - {lon}) <= 2.0
            """

            try:
                query_result = client.query(airport_traffic_query).result()
                airport_result = list(query_result)[0]
                if airport_result.nearby_messages > 0:
                    print(f"{airport_name:<15} | "
                          f"{airport_result.nearby_messages:3d} msgs | "
                          f"{airport_result.nearby_aircraft:2d} aircraft")
            except Exception:
                continue

        # 6. Regional distribution by altitude
        print("\n🏔️  6. Regional Characteristics by Altitude")
        print("-" * 35)

        altitude_region_query = f"""
        SELECT
            CASE
                WHEN altitude < 10000 THEN 'Low altitude (<10K ft)'
                WHEN altitude < 20000 THEN 'Medium altitude (10-20K ft)'
                WHEN altitude < 30000 THEN 'High altitude (20-30K ft)'
                WHEN altitude < 40000 THEN 'Cruise altitude (30-40K ft)'
                WHEN altitude >= 40000 THEN 'Very high altitude (≥40K ft)'
                ELSE 'Unknown altitude'
            END as altitude_category,
            COUNT(*) as message_count,
            COUNT(DISTINCT hex_ident) as unique_aircraft,
            AVG(latitude) as avg_lat,
            AVG(longitude) as avg_lon
        FROM `{config['project_id']}.{config['dataset_id']}.RealTimeStream`
        WHERE latitude IS NOT NULL
        AND longitude IS NOT NULL
        AND altitude IS NOT NULL
        AND altitude > 0
        GROUP BY altitude_category
        ORDER BY AVG(altitude)
        """

        altitude_region_results = client.query(altitude_region_query).result()

        for row in altitude_region_results:
            print(f"{row.altitude_category:<25} | "
                  f"{row.message_count:3d} msgs | "
                  f"{row.unique_aircraft:2d} acft | Center: "
                  f"({row.avg_lat:.2f}°, {row.avg_lon:.2f}°)")

        # 7. Movement direction analysis
        print("\n🧭 7. Primary Movement Direction Analysis")
        print("-" * 30)

        # Direction analysis if track field is available
        direction_query = f"""
        SELECT
            CASE
                WHEN track >= 315 OR track < 45 THEN 'Northbound (N)'
                WHEN track >= 45 AND track < 135 THEN 'Eastbound (E)'
                WHEN track >= 135 AND track < 225 THEN 'Southbound (S)'
                WHEN track >= 225 AND track < 315 THEN 'Westbound (W)'
                ELSE 'Unknown direction'
            END as direction,
            COUNT(*) as message_count,
            COUNT(DISTINCT hex_ident) as unique_aircraft,
            AVG(ground_speed) as avg_speed
        FROM `{config['project_id']}.{config['dataset_id']}.RealTimeStream`
        WHERE track IS NOT NULL
        AND track >= 0
        AND track <= 360
        GROUP BY direction
        ORDER BY message_count DESC
        """

        try:
            direction_results = client.query(direction_query).result()

            for row in direction_results:
                avg_speed = (f"{row.avg_speed:.0f}kt" if row.avg_speed
                             else "N/A")
                print(f"{row.direction:<18} | {row.message_count:3d} msgs | "
                      f"{row.unique_aircraft:2d} acft | "
                      f"Avg speed: {avg_speed}")
        except Exception:
            print("⚠️  No direction data available (missing track field)")

    except Exception as e:
        print(f"❌ Error during analysis execution: {e}")


def main():
    """Main execution function"""
    print("🛩️  ADS-B BigQuery Data Analysis")
    print("=" * 70)
    print(f"Analysis time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print("=" * 70)

    analyze_geographic_distribution()

    print("\n✅ Analysis #3 completed successfully!")


if __name__ == "__main__":
    main()
