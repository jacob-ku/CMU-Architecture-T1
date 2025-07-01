#!/usr/bin/env python3
"""
BigQuery table status and data verification
"""

import sys
from pathlib import Path
from google.cloud import bigquery
import pandas as pd
import json

# Add project root to Python path
project_root = Path(__file__).parent.parent
sys.path.append(str(project_root))


def load_config():
    """Load configuration file"""
    try:
        with open('config/config.json', 'r', encoding='utf-8') as f:
            config = json.load(f)
        return config['bigquery']
    except Exception as e:
        print(f"❌ Failed to load configuration file: {e}")
        return None


def check_table_status():
    """Check current BigQuery table status"""
    print("🛩️  BigQuery ADS-B Data Status")
    print("=" * 60)

    # Load configuration
    config = load_config()
    if not config:
        print("❌ Unable to load configuration file.")
        return

    try:
        client = bigquery.Client(project=config['project_id'])
        dataset_id = config['dataset_id']

        # Query all tables in the dataset
        dataset = client.dataset(dataset_id)
        tables = list(client.list_tables(dataset))

        print(f"📊 Dataset: {config['project_id']}.{dataset_id}")
        print(f"📋 Number of tables: {len(tables)}")
        print("-" * 60)

        for table in tables:
            table_ref = client.get_table(table.reference)

            # Streaming buffer information
            streaming_rows = 0
            if table_ref.streaming_buffer:
                streaming_rows = table_ref.streaming_buffer.estimated_rows or 0

            print(f"\n🗂️  {table.table_id}")
            print(f"   Rows: {table_ref.num_rows:,}")
            print(f"   Streaming buffer rows: {streaming_rows:,}")
            print(f"   Size: {table_ref.num_bytes / 1024 / 1024:.2f} MB")
            print(f"   Created: {table_ref.created}")
            print(f"   Schema fields: {len(table_ref.schema)}")

            # Detailed information for each table
            if table.table_id == "RealTimeStream":
                check_realtime_data(client, config, table_ref)
            elif table.table_id == "AircraftMetadata":
                check_aircraft_metadata(client, table_ref)
            elif table.table_id == "FlightTracks":
                check_flight_tracks(client, table_ref)

    except Exception as e:
        print(f"❌ Error: {e}")


def check_realtime_data(client, config, table):
    """Check real-time stream data"""
    # Check streaming buffer
    streaming_rows = 0
    if table.streaming_buffer:
        streaming_rows = table.streaming_buffer.estimated_rows or 0

    if table.num_rows == 0 and streaming_rows == 0:
        print("   📭 No data")
    elif streaming_rows > 0:
        print(f"   🔄 {streaming_rows:,} rows waiting in streaming buffer")

        # Check data with actual query
        try:
            query = f"""
            SELECT COUNT(*) as total_count
            FROM `{config['project_id']}.{config['dataset_id']}.RealTimeStream`
            """
            result = client.query(query).result()
            actual_count = list(result)[0].total_count
            print(f"   ✅ Actually queryable data: {actual_count:,} rows")
        except Exception as e:
            print(f"   ❌ Query execution failed: {e}")
        return

    # Check recent data
    query = f"""
    SELECT
        COUNT(*) as total_messages,
        COUNT(DISTINCT hex_ident) as unique_aircraft,
        MIN(received_at) as first_message,
        MAX(received_at) as last_message,
        COUNT(DISTINCT callsign) as unique_callsigns
    FROM `{table.project}.{table.dataset_id}.{table.table_id}`
    """

    try:
        result = list(client.query(query))[0]
        print(f"   📡 Total messages: {result.total_messages:,}")
        print(f"   ✈️  Unique aircraft: {result.unique_aircraft}")
        print(f"   📞 Unique callsigns: {result.unique_callsigns}")
        print(f"   🕐 First message: {result.first_message}")
        print(f"   🕐 Last message: {result.last_message}")

        # Check recent 5-minute data
        recent_query = f"""
        SELECT COUNT(*) as recent_count
        FROM `{table.project}.{table.dataset_id}.{table.table_id}`
        WHERE received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 5 MINUTE)
        """
        recent_result = list(client.query(recent_query))[0]
        print(f"   🔥 Recent 5 minutes: {recent_result.recent_count} messages")

    except Exception as e:
        print(f"   ❌ Failed to retrieve detailed information: {e}")


def check_aircraft_metadata(client, table):
    """Check aircraft metadata"""
    if table.num_rows == 0:
        print("   📭 No data - aircraftDatabase.csv not loaded")
        return

    # Metadata statistics
    query = f"""
    SELECT
        COUNT(*) as total_aircraft,
        COUNT(DISTINCT manufacturer) as manufacturers,
        COUNT(DISTINCT country) as countries,
        COUNT(DISTINCT operator) as operators
    FROM `{table.project}.{table.dataset_id}.{table.table_id}`
    """

    try:
        result = list(client.query(query))[0]
        print(f"   ✈️  Total aircraft: {result.total_aircraft:,}")
        print(f"   🏭 Manufacturers: {result.manufacturers}")
        print(f"   🌍 Countries: {result.countries}")
        print(f"   🏢 Operators: {result.operators}")

    except Exception as e:
        print(f"   ❌ Failed to retrieve metadata: {e}")


def check_flight_tracks(client, table):
    """Check flight track data"""
    if table.num_rows == 0:
        print("   📭 No data - flight tracks not generated")
        return

    print(f"   🛫 Flight tracks recorded")


def get_sample_data():
    """Check sample data"""
    print("\n📋 Sample Data Preview")
    print("=" * 60)

    try:
        client = bigquery.Client(project='scs-lg-arch-1')

        # RealTimeStream sample
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

        df = client.query(query).to_dataframe()

        if not df.empty:
            print("🛩️  Recent real-time data (5 records):")
            print(df.to_string(index=False))
        else:
            print("📭 No real-time data")

    except Exception as e:
        print(f"❌ Failed to retrieve sample data: {e}")


def check_aircraft_db_csv():
    """Check aircraftDatabase.csv file"""
    print("\n📁 AircraftDB CSV File Check")
    print("=" * 60)

    import os
    csv_path = "/Users/jacob/ThinQCloud/Education/250000_Architect/3_CMU/CMU-Architecture-T1/src/AircraftDB/aircraftDatabase.csv"

    if os.path.exists(csv_path):
        try:
            df = pd.read_csv(csv_path, nrows=5)  # First 5 rows only
            print(f"✅ CSV file exists: {csv_path}")
            print(f"📊 Columns: {list(df.columns)}")
            print(f"📋 Sample data:")
            print(df.to_string(index=False))

            # Check total row count
            # Exclude header
            total_rows = sum(1 for line in open(csv_path)) - 1
            print(f"📈 Total rows: {total_rows:,}")

        except Exception as e:
            print(f"❌ Failed to read CSV file: {e}")
    else:
        print(f"❌ CSV file not found: {csv_path}")


if __name__ == "__main__":
    check_table_status()
    get_sample_data()
    check_aircraft_db_csv()
