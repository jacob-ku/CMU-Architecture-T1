#!/usr/bin/env python3
"""
ADS-B BigQuery Data Analysis - #10: Predictive Analysis and Trend Forecasting
Time series analysis, traffic pattern prediction, anomaly detection
and trend analysis
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


def analyze_predictions_and_trends():
    """Analysis #10: Predictive analysis and trend forecasting"""
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
    print("🔮 Analysis #10: Predictive Analysis and Trend Forecasting")
    print("=" * 60)

    # 1. Time series traffic pattern analysis
    print("📈 1. Time Series Traffic Pattern Analysis (5-minute intervals)")
    print("-" * 50)

    timeseries_query = f"""
    WITH time_intervals AS (
      SELECT
        TIMESTAMP_TRUNC(received_at, MINUTE) as time_bucket,
        CAST(EXTRACT(MINUTE FROM received_at) / 5 AS INT64) as five_min_interval,
        COUNT(DISTINCT hex_ident) as unique_aircraft,
        COUNT(*) as total_messages,
        AVG(altitude) as avg_altitude,
        AVG(ground_speed) as avg_speed
      FROM `{project_id}.{dataset_id}.{table_id}`
      WHERE received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 60 MINUTE)
        AND altitude IS NOT NULL
        AND ground_speed IS NOT NULL
      GROUP BY time_bucket, five_min_interval
    ),
    aggregated_intervals AS (
      SELECT
        five_min_interval,
        AVG(unique_aircraft) as avg_aircraft,
        AVG(total_messages) as avg_messages,
        AVG(avg_altitude) as avg_altitude,
        AVG(avg_speed) as avg_speed,
        STDDEV(unique_aircraft) as stddev_aircraft,
        COUNT(*) as interval_count
      FROM time_intervals
      GROUP BY five_min_interval
      ORDER BY five_min_interval
    )
    SELECT
      five_min_interval * 5 as minute_mark,
      ROUND(avg_aircraft, 1) as avg_aircraft,
      ROUND(avg_messages, 0) as avg_messages,
      ROUND(avg_altitude, 0) as avg_altitude,
      ROUND(avg_speed, 0) as avg_speed,
      ROUND(COALESCE(stddev_aircraft, 0), 1) as aircraft_variation,
      interval_count
    FROM aggregated_intervals
    """

    results = client.query(timeseries_query).result()

    time_data = []
    for row in results:
        time_data.append({
            'minute': row.minute_mark,
            'aircraft': row.avg_aircraft,
            'messages': row.avg_messages,
            'altitude': row.avg_altitude,
            'speed': row.avg_speed,
            'variation': row.aircraft_variation
        })
        print(f"⏰ {row.minute_mark:2}min: "
              f"Aircraft {row.avg_aircraft:4.1f} | "
              f"Messages {row.avg_messages:4.0f} | "
              f"Alt {row.avg_altitude:,}ft | "
              f"Speed {row.avg_speed:3.0f}kt | "
              f"Variation {row.aircraft_variation:.1f}")

    print()

    # 2. Traffic increase/decrease trend analysis
    print("📊 2. Traffic Increase/Decrease Trend Analysis")
    print("-" * 50)

    trend_query = f"""
    WITH hourly_traffic AS (
      SELECT
        EXTRACT(HOUR FROM received_at) as hour,
        COUNT(DISTINCT hex_ident) as aircraft_count,
        COUNT(*) as message_count,
        AVG(altitude) as avg_altitude
      FROM `{project_id}.{dataset_id}.{table_id}`
      WHERE received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 24 HOUR)
      GROUP BY hour
      ORDER BY hour
    ),
    trend_calculation AS (
      SELECT
        hour,
        aircraft_count,
        message_count,
        avg_altitude,
        LAG(aircraft_count) OVER (ORDER BY hour) as prev_aircraft,
        LAG(message_count) OVER (ORDER BY hour) as prev_messages
      FROM hourly_traffic
    )
    SELECT
      hour,
      aircraft_count,
      message_count,
      ROUND(avg_altitude, 0) as avg_altitude,
      CASE
        WHEN prev_aircraft IS NULL THEN 0
        ELSE ROUND(((aircraft_count - prev_aircraft) / NULLIF(prev_aircraft, 0)) * 100, 1)
      END as aircraft_change_pct,
      CASE
        WHEN prev_messages IS NULL THEN 0
        ELSE ROUND(((message_count - prev_messages) / NULLIF(prev_messages, 0)) * 100, 1)
      END as message_change_pct
    FROM trend_calculation
    ORDER BY hour
    """

    results = client.query(trend_query).result()

    for row in results:
        aircraft_trend = ("📈" if row.aircraft_change_pct > 5 else
                          "📉" if row.aircraft_change_pct < -5 else "➡️")
        message_trend = ("📈" if row.message_change_pct > 5 else
                         "📉" if row.message_change_pct < -5 else "➡️")

        print(f"{row.hour:2d}:00: "
              f"Aircraft {row.aircraft_count:3d} {aircraft_trend} "
              f"({row.aircraft_change_pct:+5.1f}%) | "
              f"Messages {row.message_count:5d} {message_trend} "
              f"({row.message_change_pct:+5.1f}%) | "
              f"Avg alt {row.avg_altitude:,}ft")

    print()

    # 3. Aircraft density prediction (based on current trends)
    print("🎯 3. Aircraft Density Prediction (Next 30 minutes)")
    print("-" * 50)

    density_prediction_query = f"""
    WITH recent_density AS (
      SELECT
        TIMESTAMP_TRUNC(received_at, MINUTE) as time_bucket,
        COUNT(DISTINCT hex_ident) as aircraft_count,
        COUNT(*) as message_count
      FROM `{project_id}.{dataset_id}.{table_id}`
      WHERE received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 30 MINUTE)
      GROUP BY time_bucket
      ORDER BY time_bucket DESC
      LIMIT 10
    ),
    trend_stats AS (
      SELECT
        AVG(aircraft_count) as avg_aircraft,
        STDDEV(aircraft_count) as stddev_aircraft,
        AVG(message_count) as avg_messages,
        (MAX(aircraft_count) - MIN(aircraft_count)) / NULLIF(COUNT(*) - 1, 0) as aircraft_slope
      FROM recent_density
    )
    SELECT
      ROUND(avg_aircraft, 1) as current_avg_aircraft,
      ROUND(stddev_aircraft, 1) as aircraft_variation,
      ROUND(avg_messages, 0) as current_avg_messages,
      ROUND(COALESCE(aircraft_slope, 0), 2) as trend_slope,
      ROUND(avg_aircraft + (COALESCE(aircraft_slope, 0) * 30), 1) as predicted_aircraft_30min,
      ROUND(avg_messages + ((avg_messages / NULLIF(avg_aircraft, 0)) * (COALESCE(aircraft_slope, 0) * 30)), 0) as predicted_messages_30min
    FROM trend_stats
    """

    results = client.query(density_prediction_query).result()

    for row in results:
        trend_direction = ("increasing" if row.trend_slope > 0 else
                           "decreasing" if row.trend_slope < 0 else "stable")
        confidence = ("high" if abs(row.trend_slope) > 0.1 else
                      "medium" if abs(row.trend_slope) > 0.05 else "low")

        print(f"📊 Current Average: {row.current_avg_aircraft} aircraft/min "
              f"(±{row.aircraft_variation})")
        print(f"📈 Trend: {trend_direction} (slope: {row.trend_slope})")
        print(f"🔮 30-minute Prediction: {row.predicted_aircraft_30min} "
              f"aircraft/min")
        print(f"📡 Message Prediction: {row.predicted_messages_30min}/min")
        print(f"🎯 Prediction Confidence: {confidence}")

    print()

    # 4. Altitude traffic pattern prediction
    print("🏔️ 4. Altitude Traffic Pattern Analysis")
    print("-" * 50)

    altitude_pattern_query = f"""
    WITH altitude_time_analysis AS (
      SELECT
        CASE
          WHEN altitude < 10000 THEN 'Low Alt'
          WHEN altitude < 20000 THEN 'Med Alt'
          WHEN altitude < 30000 THEN 'High Alt'
          WHEN altitude >= 30000 THEN 'Very High Alt'
        END as altitude_band,
        EXTRACT(HOUR FROM received_at) as hour,
        COUNT(DISTINCT hex_ident) as aircraft_count,
        AVG(ground_speed) as avg_speed
      FROM `{project_id}.{dataset_id}.{table_id}`
      WHERE altitude IS NOT NULL
        AND altitude > 0
        AND received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 12 HOUR)
      GROUP BY altitude_band, hour
    ),
    altitude_trends AS (
      SELECT
        altitude_band,
        AVG(aircraft_count) as avg_aircraft,
        STDDEV(aircraft_count) as stddev_aircraft,
        AVG(avg_speed) as avg_speed,
        COUNT(*) as hour_count
      FROM altitude_time_analysis
      GROUP BY altitude_band
      ORDER BY avg_aircraft DESC
    )
    SELECT
      altitude_band,
      ROUND(avg_aircraft, 1) as avg_aircraft,
      ROUND(COALESCE(stddev_aircraft, 0), 1) as variation,
      ROUND(avg_speed, 0) as avg_speed,
      hour_count,        CASE
          WHEN COALESCE(stddev_aircraft, 0) / NULLIF(avg_aircraft, 0) < 0.3
            THEN 'Stable'
          WHEN COALESCE(stddev_aircraft, 0) / NULLIF(avg_aircraft, 0) < 0.6
            THEN 'Moderate'
          ELSE 'Unstable'
      END as stability
    FROM altitude_trends
    """

    results = client.query(altitude_pattern_query).result()

    for row in results:
        stability_icon = ("🟢" if row.stability == "Stable" else
                          "🟡" if row.stability == "Moderate" else "🔴")
        avg_speed = row.avg_speed if row.avg_speed is not None else 0
        print(f"{row.altitude_band:11} | "
              f"Avg {row.avg_aircraft:4.1f} aircraft | "
              f"Variation ±{row.variation:4.1f} | "
              f"Speed {avg_speed:3.0f}kt | "
              f"Stability {stability_icon} {row.stability}")

    print()

    # 5. Speed pattern and anomaly detection
    print("⚡ 5. Speed Pattern and Anomaly Detection")
    print("-" * 50)

    speed_anomaly_query = f"""
    WITH speed_statistics AS (
      SELECT
        AVG(ground_speed) as avg_speed,
        STDDEV(ground_speed) as stddev_speed,
        APPROX_QUANTILES(ground_speed, 100)[OFFSET(25)] as q25,
        APPROX_QUANTILES(ground_speed, 100)[OFFSET(50)] as median,
        APPROX_QUANTILES(ground_speed, 100)[OFFSET(75)] as q75,
        APPROX_QUANTILES(ground_speed, 100)[OFFSET(95)] as q95,
        APPROX_QUANTILES(ground_speed, 100)[OFFSET(99)] as q99
      FROM `{project_id}.{dataset_id}.{table_id}`
      WHERE ground_speed IS NOT NULL
        AND ground_speed > 0
        AND received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 30 MINUTE)
    ),
    anomaly_detection AS (
      SELECT
        hex_ident,
        callsign,
        ground_speed,
        altitude,
        s.avg_speed,
        s.stddev_speed,
        s.q99,
        ABS(ground_speed - s.avg_speed) / NULLIF(s.stddev_speed, 0) as z_score
      FROM `{project_id}.{dataset_id}.{table_id}` t
      CROSS JOIN speed_statistics s
      WHERE ground_speed IS NOT NULL
        AND ground_speed > 0
        AND received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 30 MINUTE)
        AND (ground_speed > s.q99 OR ABS(ground_speed - s.avg_speed) / NULLIF(s.stddev_speed, 0) > 2)
    )
    SELECT
      hex_ident,
      COALESCE(callsign, 'N/A') as callsign,
      ground_speed,
      altitude,
      ROUND(avg_speed, 0) as avg_speed,
      ROUND(z_score, 2) as z_score,
      CASE
        WHEN ground_speed > q99 THEN 'Extreme Speed'
        WHEN z_score > 3 THEN 'Statistical Anomaly'
        WHEN z_score > 2 THEN 'Caution Required'
        ELSE 'Normal Range'
      END as anomaly_type
    FROM anomaly_detection
    ORDER BY z_score DESC
    LIMIT 10
    """

    results = client.query(speed_anomaly_query).result()

    for row in results:
        anomaly_icon = ("🚨" if row.anomaly_type == "Extreme Speed" else
                        "⚠️" if row.anomaly_type == "Statistical Anomaly"
                        else "👁️")
        ground_speed = row.ground_speed if row.ground_speed is not None else 0
        altitude = row.altitude if row.altitude is not None else 0
        z_score = row.z_score if row.z_score is not None else 0
        print(f"{anomaly_icon} {row.hex_ident} | {row.callsign} | "
              f"Speed: {ground_speed:3.0f}kt | "
              f"Alt: {altitude:,}ft | "
              f"Z-score: {z_score:5.2f} | "
              f"Type: {row.anomaly_type}")

    print()

    # 6. Geographic activity prediction
    print("🗺️ 6. Geographic Activity Hotspot Prediction")
    print("-" * 50)

    geographic_prediction_query = f"""
    WITH geographic_activity AS (
      SELECT
        ROUND(latitude, 1) as lat_zone,
        ROUND(longitude, 1) as lon_zone,
        COUNT(DISTINCT hex_ident) as unique_aircraft,
        COUNT(*) as total_activity,
        AVG(altitude) as avg_altitude,
        EXTRACT(HOUR FROM received_at) as hour
      FROM `{project_id}.{dataset_id}.{table_id}`
      WHERE latitude IS NOT NULL
        AND longitude IS NOT NULL
        AND latitude BETWEEN -90 AND 90
        AND longitude BETWEEN -180 AND 180
        AND received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 6 HOUR)
      GROUP BY lat_zone, lon_zone, hour
    ),
    zone_trends AS (
      SELECT
        lat_zone,
        lon_zone,
        AVG(unique_aircraft) as avg_aircraft,
        AVG(total_activity) as avg_activity,
        AVG(avg_altitude) as avg_altitude,
        STDDEV(unique_aircraft) as stddev_aircraft,
        COUNT(*) as hour_samples
      FROM geographic_activity
      GROUP BY lat_zone, lon_zone
      HAVING COUNT(*) >= 2 AND AVG(unique_aircraft) >= 2
    )
    SELECT
      lat_zone,
      lon_zone,
      ROUND(avg_aircraft, 1) as avg_aircraft,
      ROUND(avg_activity, 0) as avg_activity,
      ROUND(avg_altitude, 0) as avg_altitude,
      ROUND(COALESCE(stddev_aircraft, 0), 1) as aircraft_variation,
      hour_samples,
      CASE
        WHEN avg_aircraft >= 5 THEN 'High Activity'
        WHEN avg_aircraft >= 3 THEN 'Medium Activity'
        ELSE 'Low Activity'
      END as activity_level
    FROM zone_trends
    ORDER BY avg_aircraft DESC
    LIMIT 15
    """

    results = client.query(geographic_prediction_query).result()

    for row in results:
        activity_icon = ("🔥" if row.activity_level == "High Activity" else
                         "🌡️" if row.activity_level == "Medium Activity"
                         else "❄️")
        print(f"{activity_icon} ({row.lat_zone:4.1f}, "
              f"{row.lon_zone:5.1f}) | "
              f"Aircraft: {row.avg_aircraft:4.1f} | "
              f"Activity: {row.avg_activity:4.0f} | "
              f"Alt: {row.avg_altitude:,}ft | "
              f"Variation: ±{row.aircraft_variation:3.1f} | "
              f"Level: {row.activity_level}")

    print()

    # 7. Prediction summary and recommendations
    print("💡 7. Prediction Summary and Recommendations")
    print("-" * 50)

    summary_query = f"""
    WITH current_status AS (
      SELECT
        COUNT(DISTINCT hex_ident) as current_aircraft,
        COUNT(*) as current_messages,
        AVG(altitude) as avg_altitude,
        AVG(ground_speed) as avg_speed,
        STDDEV(ground_speed) as speed_variation
      FROM `{project_id}.{dataset_id}.{table_id}`
      WHERE received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 10 MINUTE)
    ),
    historical_comparison AS (
      SELECT
        COUNT(DISTINCT hex_ident) as historical_aircraft,
        COUNT(*) as historical_messages,
        AVG(altitude) as historical_altitude,
        AVG(ground_speed) as historical_speed
      FROM `{project_id}.{dataset_id}.{table_id}`
      WHERE received_at BETWEEN TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 70 MINUTE)
        AND TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 60 MINUTE)
    )
    SELECT
      c.current_aircraft,
      c.current_messages,
      ROUND(c.avg_altitude, 0) as current_altitude,
      ROUND(c.avg_speed, 0) as current_speed,
      ROUND(c.speed_variation, 0) as speed_variation,
      h.historical_aircraft,
      h.historical_messages,
      ROUND(((c.current_aircraft - h.historical_aircraft) / NULLIF(h.historical_aircraft, 0)) * 100, 1) as aircraft_change_pct,
      ROUND(((c.current_messages - h.historical_messages) / NULLIF(h.historical_messages, 0)) * 100, 1) as message_change_pct
    FROM current_status c
    CROSS JOIN historical_comparison h
    """

    results = client.query(summary_query).result()

    for row in results:
        # Safely handle None values
        aircraft_change_pct = (row.aircraft_change_pct
                               if row.aircraft_change_pct is not None else 0)
        message_change_pct = (row.message_change_pct
                              if row.message_change_pct is not None else 0)
        speed_variation = (row.speed_variation
                           if row.speed_variation is not None else 0)
        current_altitude = (row.current_altitude
                            if row.current_altitude is not None else 0)
        current_speed = (row.current_speed
                         if row.current_speed is not None else 0)

        traffic_trend = ("increasing" if aircraft_change_pct > 10 else
                         "decreasing" if aircraft_change_pct < -10 else
                         "stable")
        message_trend = ("increasing" if message_change_pct > 10 else
                         "decreasing" if message_change_pct < -10 else
                         "stable")

        print("📊 Current Status:")
        print(f"   - Aircraft: {row.current_aircraft} "
              f"({aircraft_change_pct:+.1f}% vs 1hr ago)")
        print(f"   - Messages: {row.current_messages} "
              f"({message_change_pct:+.1f}% vs 1hr ago)")
        print(f"   - Avg Altitude: {current_altitude:,}ft")
        print(f"   - Avg Speed: {current_speed}kt "
              f"(variation: ±{speed_variation}kt)")
        print("")
        print("🔮 Predictions and Recommendations:")

        if traffic_trend == "increasing":
            print("   ✅ Traffic increasing - "
                  "Enhanced system monitoring recommended")
        elif traffic_trend == "decreasing":
            print("   📉 Traffic decreasing - "
                  "Data quality check required")
        else:
            print("   ➡️ Traffic stable - Normal operation state")

        if speed_variation > 100:
            print("   ⚠️ High speed variation - "
                  "Monitor for anomalous situations")
        else:
            print("   🟢 Normal speed patterns - "
                  "Stable flight conditions")

    print()
    print("=" * 70)
    print("✅ Analysis 10 Complete - Prediction Analysis and Trend Forecasting")
    print("   Time series patterns, traffic prediction, anomaly detection, "
          "geographic activity prediction complete")
    print("=" * 70)


if __name__ == "__main__":
    try:
        analyze_predictions_and_trends()
    except KeyboardInterrupt:
        print("\n❌ Analysis interrupted by user")
    except Exception as e:
        print(f"❌ Analysis error occurred: {e}")
        import traceback
        traceback.print_exc()
