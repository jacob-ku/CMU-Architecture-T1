#!/usr/bin/env python3
"""
Analysis 1: Real-time Traffic Summary Analysis
"""

from google.cloud import bigquery
from datetime import datetime


def analyze_traffic_summary():
    """Overall traffic summary analysis"""
    print("📊 Analysis 1: Real-time Traffic Summary")
    print("=" * 60)

    try:
        client = bigquery.Client('scs-lg-arch-1')

        # Overall traffic summary query
        query = """
        SELECT
            COUNT(*) as total_messages,
            COUNT(DISTINCT hex_ident) as unique_aircraft,
            MIN(received_at) as first_message,
            MAX(received_at) as last_message,
            DATETIME_DIFF(MAX(received_at), MIN(received_at), MINUTE) as duration_minutes,
            COUNT(DISTINCT callsign) as unique_callsigns,
            COUNT(CASE WHEN altitude IS NOT NULL AND altitude > 0 THEN 1 END) as messages_with_altitude,
            COUNT(CASE WHEN latitude IS NOT NULL AND longitude IS NOT NULL THEN 1 END) as messages_with_location
        FROM `scs-lg-arch-1.SBS_Data.RealTimeStream`
        """

        result = list(client.query(query))[0]

        print(f"📡 Total messages: {result.total_messages:,}")
        print(f"✈️  Unique aircraft: {result.unique_aircraft:,}")
        print(f"📞 Unique callsigns: {result.unique_callsigns:,}")
        print(f"🕐 Collection start: {result.first_message}")
        print(f"🕐 Collection end: {result.last_message}")
        print(f"⏱️  Collection duration: {result.duration_minutes} minutes")

        # Calculate message processing rate
        if result.duration_minutes and result.duration_minutes > 0:
            messages_per_minute = result.total_messages / result.duration_minutes
            print(f"📈 Messages per minute: {messages_per_minute:.1f}/min")

        # Data quality indicators
        altitude_rate = (result.messages_with_altitude / result.total_messages) * 100
        location_rate = (result.messages_with_location / result.total_messages) * 100
        callsign_rate = (result.unique_callsigns / result.unique_aircraft) * 100

        print(f"\n📊 Data Quality Indicators:")
        print(f"   Altitude info: {result.messages_with_altitude:,} "
              f"({altitude_rate:.1f}%)")
        print(f"   Location info: {result.messages_with_location:,} "
              f"({location_rate:.1f}%)")
        print(f"   Callsign ratio: {callsign_rate:.1f}% (vs aircraft)")

        # Check recent activity
        print("\n🔥 Recent Activity:")
        recent_query = """
        SELECT COUNT(*) as recent_count
        FROM `scs-lg-arch-1.SBS_Data.RealTimeStream`
        WHERE received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(),
                                           INTERVAL 5 MINUTE)
        """

        recent_result = list(client.query(recent_query))[0]
        print(f"   Last 5 minutes: {recent_result.recent_count:,} messages")

        # Hourly distribution
        print("\n⏰ Hourly Message Distribution:")
        hourly_query = """
        SELECT
            EXTRACT(HOUR FROM received_at) as hour,
            COUNT(*) as message_count
        FROM `scs-lg-arch-1.SBS_Data.RealTimeStream`
        GROUP BY hour
        ORDER BY hour
        """

        for row in client.query(hourly_query):
            print(f"   {row.hour:2d}:00: {row.message_count:,} messages")

        return True

    except Exception as e:
        print(f"❌ Analysis error: {e}")
        return False


def analyze_latest_samples():
    """Check latest sample data"""
    print("\n📋 Latest Sample Data (5 records):")
    print("-" * 40)

    try:
        client = bigquery.Client('scs-lg-arch-1')

        query = """
        SELECT
            hex_ident,
            callsign,
            altitude,
            ground_speed,
            latitude,
            longitude,
            received_at
        FROM `scs-lg-arch-1.SBS_Data.RealTimeStream`
        ORDER BY received_at DESC
        LIMIT 5
        """

        for i, row in enumerate(client.query(query), 1):
            callsign = row.callsign or "Unknown"
            altitude = f"{row.altitude:,}ft" if row.altitude else "N/A"
            speed = f"{row.ground_speed:.0f}kts" if row.ground_speed else "N/A"
            lat = f"{row.latitude:.4f}" if row.latitude else "N/A"
            lon = f"{row.longitude:.4f}" if row.longitude else "N/A"

            print(f"{i}. {row.hex_ident} ({callsign})")
            print(f"   🏔️  {altitude} | 🏃 {speed} | 📍 {lat}, {lon}")
            print(f"   🕐 {row.received_at}")
            print()

    except Exception as e:
        print(f"❌ Sample data error: {e}")


def main():
    print("🛩️  ADS-B BigQuery Data Analysis")
    print("=" * 70)
    print(f"Analysis time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print("=" * 70)

    success = analyze_traffic_summary()

    if success:
        analyze_latest_samples()

        print("\n💡 Analysis 1 Result Interpretation:")
        print("- Total messages indicate system activity level")
        print("- Unique aircraft count shows coverage")
        print("- Data quality indicators verify parsing accuracy")
        print("- Hourly distribution reveals traffic patterns")

        print("\n✅ Analysis 1 Complete! Is this analysis useful?")
        print("🎯 System status monitoring")
        print("📊 Data collection performance evaluation")
        print("🔍 Real-time operational status assessment")

    else:
        print("❌ Analysis 1 Failed")


if __name__ == "__main__":
    main()
