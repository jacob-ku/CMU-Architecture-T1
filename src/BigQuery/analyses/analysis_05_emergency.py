#!/usr/bin/env python3
"""
ADS-B BigQuery Data Analysis - #9: Emergency and Special Situations Detection
Analysis of special situations using emergency, alert, squawk codes, etc.
"""

import json
from datetime import datetime
from pathlib import Path
from google.cloud import bigquery


def load_config():
    """Load configuration file"""
    # Get the parent directory (BigQuery) from current analyses directory
    config_path = Path(__file__).parent.parent / 'config' / 'config.json'
    with open(config_path, 'r', encoding='utf-8') as f:
        return json.load(f)


def safe_format(value, default='N/A'):
    """Safely format NULL values"""
    return default if value is None else value


def analyze_emergency_situations():
    """Analysis #9: Emergency and special situations detection"""
    config = load_config()

    # Extract table information from configuration
    project_id = config['bigquery']['project_id']
    dataset_id = config['bigquery']['dataset_id']
    table_id = config['bigquery']['tables']['realtime_stream']
    client = bigquery.Client(project=project_id)

    print("🛩️  ADS-B BigQuery Data Analysis")
    print("=" * 70)
    print(f"Analysis time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print("=" * 70)
    print("🚨 Analysis #9: Emergency and Special Situations Detection")
    print("=" * 60)

    # 1. Emergency situation detection
    print("🆘 1. Emergency Situation Detection (Emergency = TRUE)")
    print("-" * 50)

    emergency_query = f"""
    SELECT
      hex_ident,
      COALESCE(callsign, 'N/A') as callsign,
      squawk,
      emergency,
      alert,
      spi,
      altitude,
      ground_speed,
      latitude,
      longitude,
      received_at
    FROM `{project_id}.{dataset_id}.{table_id}`
    WHERE emergency = TRUE
      AND received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 4 HOUR)
    ORDER BY received_at DESC
    LIMIT 20
    """

    try:
        results = client.query(emergency_query).result()

        emergency_count = 0
        for row in results:
            emergency_count += 1
            altitude = safe_format(row.altitude, 0)
            speed = safe_format(row.ground_speed, 0)
            lat = safe_format(row.latitude, 0)
            lon = safe_format(row.longitude, 0)

            print(f"🚨 {row.hex_ident} | {row.callsign:8} | "
                  f"Squawk: {row.squawk} | "
                  f"Alt: {altitude:,}ft | "
                  f"Speed: {speed}kt | "
                  f"Pos: ({lat}, {lon}) | "
                  f"Time: {row.received_at}")

        if emergency_count == 0:
            print("✅ No emergencies in the last 4 hours")
        else:
            print(f"⚠️ Total {emergency_count} emergency situations detected")

    except Exception as e:
        print(f"Emergency analysis error: {e}")

    print()

    # 2. Special Squawk code analysis
    print("📡 2. Special Squawk Code Analysis")
    print("-" * 50)

    squawk_query = f"""
    SELECT
      squawk,
      CASE
        WHEN squawk = '7500' THEN 'Hijacking'
        WHEN squawk = '7600' THEN 'Radio Failure'
        WHEN squawk = '7700' THEN 'General Emergency'
        WHEN squawk = '1200' THEN 'VFR General Flight'
        WHEN squawk LIKE '12%' THEN 'VFR Related'
        WHEN squawk LIKE '0%' THEN 'Special Control'
        WHEN squawk = '2000' THEN 'SSR Default Code'
        ELSE 'Other'
      END as squawk_meaning,
      COUNT(DISTINCT hex_ident) as aircraft_count,
      COUNT(*) as total_messages,
      MAX(received_at) as last_seen
    FROM `{project_id}.{dataset_id}.{table_id}`
    WHERE squawk IS NOT NULL
      AND squawk != ''
      AND received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 4 HOUR)
    GROUP BY squawk, squawk_meaning
    ORDER BY
      CASE
        WHEN squawk IN ('7500', '7600', '7700') THEN 1
        ELSE 2
      END,
      aircraft_count DESC
    LIMIT 20
    """

    try:
        results = client.query(squawk_query).result()

        for row in results:
            priority = "🚨" if row.squawk in ['7500', '7600', '7700'] else "📡"
            print(f"{priority} {row.squawk} | {row.squawk_meaning:20} | "
                  f"Aircraft: {row.aircraft_count:3} | "
                  f"Messages: {row.total_messages:5} | "
                  f"Latest: {row.last_seen}")

    except Exception as e:
        print(f"Squawk code analysis error: {e}")

    print()

    # 3. Alert status analysis
    print("⚠️ 3. Alert Status Analysis")
    print("-" * 50)

    alert_query = f"""
    SELECT
      hex_ident,
      COALESCE(callsign, 'N/A') as callsign,
      squawk,
      alert,
      emergency,
      spi,
      COUNT(*) as alert_messages,
      MIN(received_at) as first_alert,
      MAX(received_at) as last_alert,
      AVG(COALESCE(altitude, 0)) as avg_altitude,
      AVG(COALESCE(ground_speed, 0)) as avg_speed
    FROM `{project_id}.{dataset_id}.{table_id}`
    WHERE alert = TRUE
      AND received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 4 HOUR)
    GROUP BY hex_ident, callsign, squawk, alert, emergency, spi
    ORDER BY first_alert DESC
    LIMIT 15
    """

    try:
        results = client.query(alert_query).result()

        alert_count = 0
        for row in results:
            alert_count += 1
            avg_alt = safe_format(row.avg_altitude, 0)
            avg_speed = safe_format(row.avg_speed, 0)

            print(f"⚠️ {row.hex_ident} | {row.callsign:8} | "
                  f"Squawk: {row.squawk} | "
                  f"Messages: {row.alert_messages:3} | "
                  f"Avg alt: {avg_alt:,}ft | "
                  f"Avg speed: {avg_speed}kt | "
                  f"Duration: {row.first_alert} ~ {row.last_alert}")

        if alert_count == 0:
            print("✅ No alert status in the last 4 hours")
        else:
            print(f"⚠️ Alert status detected in {alert_count} aircraft")

    except Exception as e:
        print(f"Alert analysis error: {e}")

    print()

    # 4. SPI (Special Position Identification) analysis
    print("🎯 4. SPI (Special Position Identification) Analysis")
    print("-" * 50)

    spi_query = f"""
    SELECT
      hex_ident,
      COALESCE(callsign, 'N/A') as callsign,
      squawk,
      COUNT(*) as spi_messages,
      MIN(received_at) as first_spi,
      MAX(received_at) as last_spi,
      AVG(COALESCE(altitude, 0)) as avg_altitude,
      AVG(COALESCE(ground_speed, 0)) as avg_speed
    FROM `{project_id}.{dataset_id}.{table_id}`
    WHERE spi = TRUE
      AND received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 4 HOUR)
    GROUP BY hex_ident, callsign, squawk
    ORDER BY first_spi DESC
    LIMIT 10
    """

    try:
        results = client.query(spi_query).result()

        spi_count = 0
        for row in results:
            spi_count += 1
            avg_alt = safe_format(row.avg_altitude, 0)
            avg_speed = safe_format(row.avg_speed, 0)

            print(f"🎯 {row.hex_ident} | {row.callsign:8} | "
                  f"Squawk: {row.squawk} | "
                  f"Messages: {row.spi_messages:3} | "
                  f"Avg alt: {avg_alt:,}ft | "
                  f"Avg speed: {avg_speed}kt | "
                  f"Duration: {row.first_spi} ~ {row.last_spi}")

        if spi_count == 0:
            print("✅ No SPI activation in the last 4 hours")
        else:
            print(f"🎯 SPI activation detected in {spi_count} aircraft")

    except Exception as e:
        print(f"SPI analysis error: {e}")

    print()

    # 5. Ground operations analysis
    print("🛬 5. Ground Operations Analysis (is_on_ground = TRUE)")
    print("-" * 50)

    ground_query = f"""
    SELECT
      COUNT(DISTINCT hex_ident) as ground_aircraft,
      COUNT(*) as ground_messages,
      AVG(COALESCE(ground_speed, 0)) as avg_ground_speed,
      MIN(received_at) as earliest_ground,
      MAX(received_at) as latest_ground
    FROM `{project_id}.{dataset_id}.{table_id}`
    WHERE is_on_ground = TRUE
      AND received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 2 HOUR)
    """

    try:
        results = client.query(ground_query).result()

        for row in results:
            avg_speed = safe_format(row.avg_ground_speed, 0)
            print(f"🛬 Ground aircraft: {row.ground_aircraft}")
            print(f"🛬 Ground messages: {row.ground_messages:,}")
            print(f"🛬 Average ground speed: {avg_speed}kt")
            print(f"🛬 Data range: {row.earliest_ground} ~ {row.latest_ground}")

    except Exception as e:
        print(f"Ground operations analysis error: {e}")

    print()

    # 6. Comprehensive security situation summary
    print("🔐 6. Comprehensive Security Situation Summary")
    print("-" * 50)

    security_summary_query = f"""
    SELECT
      COUNT(CASE WHEN emergency = TRUE THEN 1 END) as emergency_count,
      COUNT(CASE WHEN alert = TRUE THEN 1 END) as alert_count,
      COUNT(CASE WHEN spi = TRUE THEN 1 END) as spi_count,
      COUNT(CASE WHEN squawk IN ('7500', '7600', '7700') THEN 1 END) as critical_squawk_count,
      COUNT(CASE WHEN is_on_ground = TRUE THEN 1 END) as ground_count,
      COUNT(DISTINCT hex_ident) as total_aircraft,
      COUNT(*) as total_messages
    FROM `{project_id}.{dataset_id}.{table_id}`
    WHERE received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 4 HOUR)
    """

    try:
        results = client.query(security_summary_query).result()

        for row in results:
            print(f"📊 Security situation summary for the last 4 hours:")
            print(f"   🚨 Emergencies: {row.emergency_count}")
            print(f"   ⚠️ Alert status: {row.alert_count}")
            print(f"   🎯 SPI activations: {row.spi_count}")
            print(f"   📡 Critical squawks: {row.critical_squawk_count}")
            print(f"   🛬 Ground messages: {row.ground_count:,}")
            print(f"   ✈️ Total aircraft: {row.total_aircraft}")
            print(f"   📨 Total messages: {row.total_messages:,}")

            # Risk assessment
            risk_score = (row.emergency_count * 10 +
                         row.alert_count * 5 +
                         row.spi_count * 3 +
                         row.critical_squawk_count * 8)

            if risk_score == 0:
                risk_level = "🟢 Low"
            elif risk_score <= 10:
                risk_level = "🟡 Medium"
            elif risk_score <= 50:
                risk_level = "🟠 High"
            else:
                risk_level = "🔴 Very High"

            print(f"   🎚️ Risk score: {risk_score} ({risk_level})")

    except Exception as e:
        print(f"Security summary analysis error: {e}")

    print()
    print("=" * 70)
    print("✅ Analysis #9 Complete - Emergency and Special Situations "
          "Detection")
    print("   Emergency, Alert, SPI, Squawk codes, ground ops, "
          "security summary")
    print("=" * 70)


if __name__ == "__main__":
    try:
        analyze_emergency_situations()
    except Exception as e:
        print(f"❌ Error during analysis: {e}")
        import traceback
        traceback.print_exc()
