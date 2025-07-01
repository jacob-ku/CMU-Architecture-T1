#!/usr/bin/env python3
"""
Script to upload AircraftMetadata to BigQuery
Reads aircraft database CSV file and uploads to BigQuery table.
"""

import json
import pandas as pd
from google.cloud import bigquery
from google.cloud.exceptions import NotFound
import os
import sys
from datetime import datetime


def load_config():
    """Load configuration file"""
    config_path = os.path.join(os.path.dirname(__file__),
                              '../config/config.json')
    try:
        with open(config_path, 'r', encoding='utf-8') as f:
            full_config = json.load(f)
            # Return just the bigquery config for backward compatibility
            return full_config['bigquery']
    except Exception as e:
        print(f"❌ Failed to load configuration file: {e}")
        print(f"📁 Expected location: {config_path}")
        return None


def setup_bigquery_client():
    """Setup BigQuery client"""
    config = load_config()
    credentials_path = os.path.join(os.path.dirname(__file__),
                                   f"../config/{config['credentials_path']}")

    # Only set credentials if file exists and is valid
    if os.path.exists(credentials_path):
        try:
            # Test if the credentials file is valid by trying to load it
            with open(credentials_path, 'r') as f:
                cred_data = json.load(f)
                # Check if it's a valid service account key with actual values
                has_private_key = ('private_key' in cred_data and
                                  cred_data['private_key'].strip())
                has_client_email = ('client_email' in cred_data and
                                   cred_data['client_email'].strip())
                has_project_id = ('project_id' in cred_data and
                                 cred_data['project_id'].strip())

                if has_private_key and has_client_email and has_project_id:
                    os.environ['GOOGLE_APPLICATION_CREDENTIALS'] = credentials_path
                    print("✅ Using service account credentials")
                else:
                    print("⚠️  Warning: Credentials file contains empty values")
                    print("    Trying application default credentials...")
        except (json.JSONDecodeError, KeyError, Exception) as e:
            print(f"⚠️  Warning: Credentials file error: {e}")
            print("    Trying application default credentials...")
    else:
        print("⚠️  Warning: Credentials file not found")
        print("    Trying application default credentials...")

    try:
        client = bigquery.Client(project=config['project_id'])
        # Test the connection with a simple query
        test_query = f"SELECT 1 as test"
        list(client.query(test_query).result())
        return client
    except Exception as e:
        print(f"❌ Failed to create BigQuery client: {e}")
        print("💡 Please set up Google Cloud credentials:")
        print("   1. Get a service account key file from Google Cloud Console")
        print("   2. Save it as config/YourJsonFile.json")
        print("   3. Or run: gcloud auth application-default login")
        print("   4. Ensure the service account has BigQuery permissions")
        raise


def create_aircraft_metadata_table(client, config):
    """Create AircraftMetadata table"""
    table_id = f"{config['project_id']}.{config['dataset_id']}.AircraftMetadata"

    schema = [
        bigquery.SchemaField("icao24", "STRING", mode="REQUIRED", description="ICAO 24-bit address"),
        bigquery.SchemaField("registration", "STRING", mode="NULLABLE", description="Aircraft registration"),
        bigquery.SchemaField("manufacturericao", "STRING", mode="NULLABLE", description="Manufacturer ICAO code"),
        bigquery.SchemaField("manufacturername", "STRING", mode="NULLABLE", description="Manufacturer name"),
        bigquery.SchemaField("model", "STRING", mode="NULLABLE", description="Aircraft model"),
        bigquery.SchemaField("typecode", "STRING", mode="NULLABLE", description="Aircraft type code"),
        bigquery.SchemaField("serialnumber", "STRING", mode="NULLABLE", description="Serial number"),
        bigquery.SchemaField("linenumber", "STRING", mode="NULLABLE", description="Line number"),
        bigquery.SchemaField("icaoaircrafttype", "STRING", mode="NULLABLE", description="ICAO aircraft type"),
        bigquery.SchemaField("operator", "STRING", mode="NULLABLE", description="Operator/Airline"),
        bigquery.SchemaField("operatorcallsign", "STRING", mode="NULLABLE", description="Operator callsign"),
        bigquery.SchemaField("operatoricao", "STRING", mode="NULLABLE", description="Operator ICAO code"),
        bigquery.SchemaField("operatoriata", "STRING", mode="NULLABLE", description="Operator IATA code"),
        bigquery.SchemaField("owner", "STRING", mode="NULLABLE", description="Aircraft owner"),
        bigquery.SchemaField("testreg", "STRING", mode="NULLABLE", description="Test registration"),
        bigquery.SchemaField("registered", "STRING", mode="NULLABLE", description="Registration date"),
        bigquery.SchemaField("reguntil", "STRING", mode="NULLABLE", description="Registration until"),
        bigquery.SchemaField("status", "STRING", mode="NULLABLE", description="Aircraft status"),
        bigquery.SchemaField("built", "STRING", mode="NULLABLE", description="Built date"),
        bigquery.SchemaField("firstflightdate", "STRING", mode="NULLABLE", description="First flight date"),
        bigquery.SchemaField("seatconfiguration", "STRING", mode="NULLABLE", description="Seat configuration"),
        bigquery.SchemaField("engines", "STRING", mode="NULLABLE", description="Engine information"),
        bigquery.SchemaField("modes", "STRING", mode="NULLABLE", description="Mode S information"),
        bigquery.SchemaField("adsb", "STRING", mode="NULLABLE", description="ADS-B capability"),
        bigquery.SchemaField("acars", "STRING", mode="NULLABLE", description="ACARS capability"),
        bigquery.SchemaField("notes", "STRING", mode="NULLABLE", description="Additional notes"),
        bigquery.SchemaField("categoryDescription", "STRING", mode="NULLABLE", description="Category description"),
        bigquery.SchemaField("uploaded_at", "TIMESTAMP", mode="REQUIRED", description="Upload timestamp"),
    ]

    table = bigquery.Table(table_id, schema=schema)
    table.description = "Aircraft metadata information for ADS-B tracking system"

    # Partitioning setup (based on uploaded_at)
    table.time_partitioning = bigquery.TimePartitioning(
        type_=bigquery.TimePartitioningType.DAY,
        field="uploaded_at"
    )

    try:
        table = client.create_table(table)
        print(f"✅ Table creation completed: {table_id}")
        return table
    except Exception as e:
        if "already exists" in str(e).lower():
            print(f"ℹ️  Table already exists: {table_id}")
            return client.get_table(table_id)
        else:
            raise e


def load_aircraft_database(csv_path):
    """Load aircraft database CSV file"""
    if not os.path.exists(csv_path):
        print(f"❌ CSV file not found: {csv_path}")
        return None

    try:
        # Read CSV file (try multiple encodings)
        encodings = ['utf-8', 'latin-1', 'cp1252', 'iso-8859-1']
        df = None

        for encoding in encodings:
            try:
                df = pd.read_csv(csv_path, encoding=encoding, low_memory=False)
                print(f"✅ CSV file loaded successfully (encoding: {encoding})")
                break
            except UnicodeDecodeError:
                continue

        if df is None:
            print("❌ Cannot recognize CSV file encoding.")
            return None

        print(f"📊 Total {len(df)} aircraft records loaded")
        print(f"📋 Columns: {list(df.columns)}")

        return df

    except Exception as e:
        print(f"❌ Error loading CSV file: {e}")
        return None


def prepare_aircraft_data(df):
    """Prepare data for BigQuery upload"""
    if df is None:
        return None

    # Column mapping (adjust according to actual CSV column names)
    column_mapping = {
        'icao24': 'icao24',
        'registration': 'registration',
        'manufacturericao': 'manufacturericao',
        'manufacturername': 'manufacturername',
        'model': 'model',
        'typecode': 'typecode',
        'serialnumber': 'serialnumber',
        'linenumber': 'linenumber',
        'icaoaircrafttype': 'icaoaircrafttype',
        'operator': 'operator',
        'operatorcallsign': 'operatorcallsign',
        'operatoricao': 'operatoricao',
        'operatoriata': 'operatoriata',
        'owner': 'owner',
        'testreg': 'testreg',
        'registered': 'registered',
        'reguntil': 'reguntil',
        'status': 'status',
        'built': 'built',
        'firstflightdate': 'firstflightdate',
        'seatconfiguration': 'seatconfiguration',
        'engines': 'engines',
        'modes': 'modes',
        'adsb': 'adsb',
        'acars': 'acars',
        'notes': 'notes',
        'categoryDescription': 'categoryDescription'
    }

    # Select only available columns
    available_columns = {}
    for target_col, source_col in column_mapping.items():
        if source_col in df.columns:
            available_columns[target_col] = source_col
        else:
            print(f"⚠️  Column '{source_col}' not found in CSV. "
                  f"Setting to None.")

    # Create new DataFrame
    prepared_data = {}
    for target_col, source_col in available_columns.items():
        prepared_data[target_col] = df[source_col].astype(str).replace('nan', None)

    # Fill missing columns with None
    for target_col in column_mapping.keys():
        if target_col not in prepared_data:
            prepared_data[target_col] = [None] * len(df)

    # Add uploaded_at
    prepared_data['uploaded_at'] = [datetime.utcnow()] * len(df)

    result_df = pd.DataFrame(prepared_data)

    # Remove duplicates (based on icao24)
    result_df = result_df.drop_duplicates(subset=['icao24'], keep='first')

    print(f"📊 Data prepared: {len(result_df)} records")

    return result_df


def upload_to_bigquery(client, config, df):
    """Upload data to BigQuery"""
    if df is None or len(df) == 0:
        print("❌ No data to upload.")
        return False

    table_id = f"{config['project_id']}.{config['dataset_id']}.AircraftMetadata"

    # Check existing data
    try:
        table = client.get_table(table_id)
        existing_count = (client.query(f"SELECT COUNT(*) as count "
                                       f"FROM `{table_id}`")
                          .to_dataframe().iloc[0]['count'])
        print(f"📊 Existing record count: {existing_count:,}")

        if existing_count > 0:
            response = input("Existing data found. "
                           "Do you want to overwrite? (y/N): ")
            if response.lower() != 'y':
                print("Upload cancelled.")
                return False

            # Delete existing table and recreate
            client.delete_table(table_id)
            print("🗑️  Existing table deletion completed")
            create_aircraft_metadata_table(client, config)

    except NotFound:
        pass

    # Batch upload
    batch_size = 10000
    total_rows = len(df)

    print(f"📤 Starting BigQuery upload... (total {total_rows:,} records)")

    for i in range(0, total_rows, batch_size):
        batch_df = df.iloc[i:i+batch_size]

        job_config = bigquery.LoadJobConfig(
            write_disposition="WRITE_APPEND",
            schema_update_options=[
                bigquery.SchemaUpdateOption.ALLOW_FIELD_ADDITION]
        )

        try:
            job = client.load_table_from_dataframe(
                batch_df, table_id, job_config=job_config)
            job.result()  # Wait for completion

            print(f"✅ Batch {i//batch_size + 1}/"
                  f"{(total_rows-1)//batch_size + 1} upload complete "
                  f"({len(batch_df)} records)")

        except Exception as e:
            print(f"❌ Error during batch upload: {e}")
            return False

    # Final verification
    final_count = (client.query(f"SELECT COUNT(*) as count FROM `{table_id}`")
                   .to_dataframe().iloc[0]['count'])
    print(f"🎉 Upload completed! Total {final_count:,} records "
          f"saved to BigQuery.")

    return True


def main():
    """Main execution function"""
    print("🛩️  AircraftMetadata BigQuery Upload Tool")
    print("=" * 60)

    try:
        # Load configuration
        config = load_config()
        if not config:
            print("❌ Unable to load configuration file.")
            return

        print(f"📁 Project: {config['project_id']}")
        print(f"📁 Dataset: {config['dataset_id']}")

        # Setup BigQuery client
        client = setup_bigquery_client()
        print("✅ BigQuery client connection successful")

        # Create table
        create_aircraft_metadata_table(client, config)

        # Check CSV file path
        csv_path = os.path.join(os.path.dirname(__file__), '../AircraftDB/aircraftDatabase.csv')

        if not os.path.exists(csv_path):
            # Try alternative paths
            alternative_paths = [
                '../../AircraftDB/aircraftDatabase.csv',
                '../../../AircraftDB/aircraftDatabase.csv',
                './aircraftDatabase.csv'
            ]

            for alt_path in alternative_paths:
                full_path = os.path.join(os.path.dirname(__file__), alt_path)
                if os.path.exists(full_path):
                    csv_path = full_path
                    break
            else:
                print("❌ aircraftDatabase.csv file not found.")
                print(f"Currently searching path: {csv_path}")
                print("Please enter the file path directly:")
                csv_path = input("CSV file path: ").strip()

        # Load data
        df = load_aircraft_database(csv_path)
        if df is None:
            return False

        # Prepare data
        prepared_df = prepare_aircraft_data(df)
        if prepared_df is None:
            return False

        # Upload to BigQuery
        success = upload_to_bigquery(client, config, prepared_df)

        if success:
            print("\n🎉 AircraftMetadata upload completed successfully!")
            print("🔗 Check in BigQuery console: "
                  "https://console.cloud.google.com/bigquery")
        else:
            print("\n❌ Error occurred during upload.")

        return success

    except Exception as e:
        print(f"❌ Unexpected error occurred: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)
