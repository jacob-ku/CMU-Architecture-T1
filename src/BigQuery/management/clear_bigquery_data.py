#!/usr/bin/env python3
"""
BigQuery table data initialization script
Usage: python clear_bigquery_data.py [--confirm]
"""

import sys
import argparse
import json
from google.cloud import bigquery
from typing import List


def load_config():
    """Load configuration file"""
    try:
        with open('config/config.json', 'r', encoding='utf-8') as f:
            config = json.load(f)
        return config['bigquery']
    except Exception as e:
        print(f"❌ Failed to load configuration file: {e}")
        return None


def clear_table_data(project_id: str, dataset_id: str, table_id: str, confirm: bool = False) -> bool:
    """Delete all data from BigQuery table"""

    if not confirm:
        print("⚠️  This operation will delete all data from the table!")
        table_full_id = f"{project_id}.{dataset_id}.{table_id}"
        print(f"Target table: {table_full_id}")

        response = input("Are you sure you want to delete? (yes/no): ")
        if response.lower() != 'yes':
            print("❌ Operation cancelled.")
            return False

    try:
        client = bigquery.Client(project=project_id)
        table_full_id = f"{project_id}.{dataset_id}.{table_id}"

        # Check table existence
        try:
            table = client.get_table(table_full_id)
            current_rows = table.num_rows
            print(f"📊 Current table row count: {current_rows:,}")
        except Exception as e:
            print(f"❌ Table not found: {e}")
            return False

        if current_rows == 0:
            print("✅ Table is already empty.")
            return True

        # Execute DELETE query
        delete_query = f"DELETE FROM `{table_full_id}` WHERE TRUE"

        print(f"🧹 Deleting data...")
        job = client.query(delete_query)
        job.result()  # Wait until completion

        # Verify after deletion
        table = client.get_table(table_full_id)
        remaining_rows = table.num_rows

        print(f"✅ Deletion completed!")
        print(f"   Deleted rows: {current_rows - remaining_rows:,}")
        print(f"   Remaining rows: {remaining_rows:,}")

        return True

    except Exception as e:
        print(f"❌ Data deletion failed: {e}")
        return False


def clear_all_adsb_tables(project_id: str, dataset_id: str, confirm: bool = False) -> bool:
    """Delete all ADS-B related table data"""

    tables_to_clear = [
        "RealTimeStream",
        "AircraftMetadata",
        "FlightTracks"
    ]

    print("🛩️  ADS-B BigQuery Data Initialization")
    print("=" * 50)
    print(f"Project: {project_id}")
    print(f"Dataset: {dataset_id}")
    print(f"Target tables: {', '.join(tables_to_clear)}")
    print("-" * 50)

    if not confirm:
        print("⚠️  All ADS-B data will be deleted!")
        response = input("Do you want to continue? (yes/no): ")
        if response.lower() != 'yes':
            print("❌ Operation cancelled.")
            return False

    success_count = 0
    for table_id in tables_to_clear:
        print(f"\n📋 Processing {table_id} table...")
        if clear_table_data(project_id, dataset_id, table_id, confirm=True):
            success_count += 1
        else:
            print(f"⚠️  {table_id} table processing failed")

    print(f"\n🎯 Completed: {success_count}/{len(tables_to_clear)} "
          f"tables initialized")
    return success_count == len(tables_to_clear)


def get_table_info(project_id: str, dataset_id: str) -> None:
    """Query table information (including streaming buffer)"""
    try:
        client = bigquery.Client(project=project_id)
        dataset_ref = client.dataset(dataset_id)

        print(f"\n📊 Dataset information: {project_id}.{dataset_id}")
        print("-" * 50)

        tables = list(client.list_tables(dataset_ref))

        if not tables:
            print("❌ No tables found.")
            return

        for table in tables:
            table_ref = client.get_table(table.reference)

            # Streaming buffer information
            streaming_rows = 0
            streaming_bytes = 0
            if table_ref.streaming_buffer:
                streaming_rows = table_ref.streaming_buffer.estimated_rows or 0
                streaming_bytes = (
                    table_ref.streaming_buffer.estimated_bytes or 0
                )

            # Query to check actual data
            try:
                query = (
                    f"SELECT COUNT(*) as total FROM "
                    f"`{project_id}.{dataset_id}.{table.table_id}`"
                )
                query_result = client.query(query).result()
                actual_rows = list(query_result)[0].total
            except Exception:
                actual_rows = "Query failed"

            print(f"📋 {table.table_id}")
            print(f"   Metadata row count: {table_ref.num_rows:,}")
            if isinstance(actual_rows, int):
                print(f"   Actual query row count: {actual_rows:,}")
            else:
                print(f"   Actual query row count: {actual_rows}")
            print(f"   Streaming buffer rows: {streaming_rows:,}")
            print(f"   Size: {table_ref.num_bytes / 1024 / 1024:.2f} MB")
            print(f"   Streaming size: {streaming_bytes / 1024 / 1024:.2f} MB")
            print(f"   Created: {table_ref.created}")
            print(f"   Modified: {table_ref.modified}")
            if streaming_rows > 0:
                print("   ⚠️  Data exists in streaming buffer")
            print()

    except Exception as e:
        print(f"❌ Table information query failed: {e}")


def main():
    parser = argparse.ArgumentParser(
        description='BigQuery ADS-B Data Initialization'
    )
    parser.add_argument(
        '--project',
        help='Google Cloud Project ID (default: config.json)'
    )
    parser.add_argument(
        '--dataset',
        help='BigQuery Dataset ID (default: config.json)'
    )
    parser.add_argument(
        '--table',
        help='Delete specific table only (default: all tables)'
    )
    parser.add_argument(
        '--confirm',
        action='store_true',
        help='Delete immediately without confirmation'
    )
    parser.add_argument(
        '--info',
        action='store_true',
        help='Query table information only'
    )

    args = parser.parse_args()

    # Load default values from configuration file
    config = load_config()
    if not config:
        print("❌ Unable to load configuration file.")
        sys.exit(1)

    # Use configuration file values if no command line arguments
    project_id = args.project or config['project_id']
    dataset_id = args.dataset or config['dataset_id']

    if args.info:
        get_table_info(project_id, dataset_id)
        return

    if args.table:
        # Delete specific table only
        success = clear_table_data(
            project_id, dataset_id, args.table, args.confirm
        )
    else:
        # Delete all tables
        success = clear_all_adsb_tables(project_id, dataset_id, args.confirm)

    if success:
        print("\n✅ Initialization completed!")
        print("You can now start streaming with a new data source.")
    else:
        print("\n❌ Initialization failed!")
        sys.exit(1)


if __name__ == "__main__":
    main()
