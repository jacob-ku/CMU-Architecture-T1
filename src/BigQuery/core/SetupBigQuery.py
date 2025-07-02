"""
BigQuery table schema definition and initial setup
Optimized BigQuery table structure for ADS-B data
"""

from google.cloud import bigquery
from google.cloud.bigquery import SchemaField


def create_adsb_table_schema():
    """BigQuery table schema definition for real-time ADS-B data"""
    schema = [
        # Basic message information
        SchemaField("message_type", "STRING", description="SBS message type (1-8)"),
        SchemaField("session_id", "STRING", description="Session ID"),
        SchemaField("aircraft_id", "STRING", description="Aircraft ID"),
        SchemaField("hex_ident", "STRING", description="ICAO hexadecimal identifier"),
        SchemaField("flight_id", "STRING", description="Flight ID"),

        # Flight information
        SchemaField("callsign", "STRING", description="Aircraft callsign"),
        SchemaField("altitude", "INTEGER", description="Altitude (feet)"),
        SchemaField("ground_speed", "FLOAT", description="Ground speed (knots)"),
        SchemaField("track", "FLOAT", description="Track direction (degrees)"),
        SchemaField("vertical_rate", "INTEGER", description="Vertical rate (feet/min)"),

        # Location information (added GEOGRAPHY type for geospatial analysis)
        SchemaField("latitude", "FLOAT", description="Latitude"),
        SchemaField("longitude", "FLOAT", description="Longitude"),
        SchemaField("position", "GEOGRAPHY", description="Location information (POINT)"),

        # Status information
        SchemaField("squawk", "STRING", description="Transponder code"),
        SchemaField("alert", "BOOLEAN", description="Alert status"),
        SchemaField("emergency", "BOOLEAN", description="Emergency status"),
        SchemaField("spi", "BOOLEAN", description="Special position identification"),
        SchemaField("is_on_ground", "BOOLEAN", description="On-ground status"),

        # Time information
        SchemaField("received_at", "TIMESTAMP", description="Received timestamp (UTC)"),
        SchemaField("date_generated", "STRING", description="Message generation date"),
        SchemaField("date_logged", "STRING", description="Log record date"),

        # Additional fields for analysis
        SchemaField("message_hash", "STRING", description="Hash for deduplication"),
        SchemaField("processing_batch_id", "STRING", description="Processing batch ID")
    ]
    return schema


def create_aircraft_metadata_schema():
    """Aircraft metadata table schema (based on aircraftDatabase.csv)"""
    schema = [
        SchemaField("hex_ident", "STRING", description="ICAO hexadecimal identifier"),
        SchemaField("registration", "STRING", description="Registration number"),
        SchemaField("manufacturer", "STRING", description="Manufacturer"),
        SchemaField("type_code", "STRING", description="Aircraft type code"),
        SchemaField("model", "STRING", description="Model name"),
        SchemaField("operator", "STRING", description="Operator"),
        SchemaField("country", "STRING", description="Registration country"),
        SchemaField("category", "STRING", description="Aircraft category"),
        SchemaField("weight_class", "STRING", description="Weight class"),
        SchemaField("engine_type", "STRING", description="Engine type"),
        SchemaField("is_military", "BOOLEAN", description="Military aircraft indicator"),
        SchemaField("is_helicopter", "BOOLEAN", description="Helicopter indicator"),
        SchemaField("updated_at", "TIMESTAMP", description="Update timestamp")
    ]
    return schema


def create_flight_tracks_schema():
    """Flight tracks table schema"""
    schema = [
        SchemaField("track_id", "STRING", description="Track ID"),
        SchemaField("hex_ident", "STRING", description="ICAO hexadecimal identifier"),
        SchemaField("callsign", "STRING", description="Aircraft callsign"),
        SchemaField("departure_airport", "STRING", description="Departure airport"),
        SchemaField("arrival_airport", "STRING", description="Arrival airport"),
        SchemaField("route", "GEOGRAPHY", description="Flight route (LINESTRING)"),
        SchemaField("start_time", "TIMESTAMP", description="Tracking start time"),
        SchemaField("end_time", "TIMESTAMP", description="Tracking end time"),
        SchemaField("max_altitude", "INTEGER", description="Maximum altitude"),
        SchemaField("avg_speed", "FLOAT", description="Average speed"),
        SchemaField("total_distance", "FLOAT", description="Total flight distance (km)"),
        SchemaField("created_at", "TIMESTAMP", description="Record creation time")
    ]
    return schema


def setup_bigquery_dataset(project_id: str, dataset_id: str, location: str = "US"):
    """Initial setup for BigQuery dataset and tables"""
    client = bigquery.Client(project=project_id)

    # Create dataset
    dataset_full_id = f"{project_id}.{dataset_id}"
    dataset = bigquery.Dataset(dataset_full_id)
    dataset.location = location
    dataset.description = "ADS-B aircraft tracking data"

    try:
        dataset = client.create_dataset(dataset, exists_ok=True)
        print(f"Dataset {dataset_id} created or already exists")
    except Exception as e:
        print(f"Error creating dataset: {e}")
        return False

    # Create tables
    tables_config = [
        {
            "table_id": "RealTimeStream",
            "schema": create_adsb_table_schema(),
            "description": "Real-time ADS-B message stream",
            "partition_field": "received_at",
            "clustering_fields": ["hex_ident", "callsign"]
        },
        {
            "table_id": "AircraftMetadata",
            "schema": create_aircraft_metadata_schema(),
            "description": "Aircraft metadata",
            "clustering_fields": ["hex_ident", "manufacturer"]
        },
        {
            "table_id": "FlightTracks",
            "schema": create_flight_tracks_schema(),
            "description": "Flight tracks data",
            "partition_field": "start_time",
            "clustering_fields": ["hex_ident", "departure_airport"]
        }
    ]

    for table_config in tables_config:
        create_table(client, dataset_id, table_config)

    return True


def create_table(client: bigquery.Client, dataset_id: str, config: dict):
    """Create individual table"""
    table_ref = client.dataset(dataset_id).table(config["table_id"])
    table = bigquery.Table(table_ref, schema=config["schema"])
    table.description = config["description"]

    # Partitioning configuration
    if "partition_field" in config:
        table.time_partitioning = bigquery.TimePartitioning(
            type_=bigquery.TimePartitioningType.DAY,
            field=config["partition_field"]
        )

    # Clustering configuration
    if "clustering_fields" in config:
        table.clustering_fields = config["clustering_fields"]

    try:
        table = client.create_table(table, exists_ok=True)
        print(f"Table {config['table_id']} created or already exists")
    except Exception as e:
        print(f"Error creating table {config['table_id']}: {e}")


def create_analytics_views(project_id: str, dataset_id: str):
    """Create analytical views"""
    client = bigquery.Client(project=project_id)

    views = [
        {
            "view_id": "HourlyTrafficSummary",
            "query": f"""
            SELECT
                DATETIME_TRUNC(DATETIME(received_at), HOUR) as hour,
                COUNT(DISTINCT hex_ident) as unique_aircraft,
                COUNT(*) as total_messages,
                AVG(altitude) as avg_altitude,
                AVG(ground_speed) as avg_speed
            FROM `{project_id}.{dataset_id}.RealTimeStream`
            WHERE received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 7 DAY)
            GROUP BY hour
            ORDER BY hour DESC
            """,
            "description": "Time-based traffic summary"
        },
        {
            "view_id": "ActiveFlights",
            "query": f"""
            SELECT DISTINCT
                r.hex_ident,
                r.callsign,
                m.registration,
                m.manufacturer,
                m.model,
                r.altitude,
                r.ground_speed,
                r.latitude,
                r.longitude,
                r.received_at as last_seen
            FROM `{project_id}.{dataset_id}.RealTimeStream` r
            LEFT JOIN `{project_id}.{dataset_id}.AircraftMetadata` m
                ON r.hex_ident = m.hex_ident
            WHERE r.received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 5 MINUTE)
            QUALIFY ROW_NUMBER() OVER (PARTITION BY r.hex_ident ORDER BY r.received_at DESC) = 1
            """,
            "description": "List of currently active aircraft"
        },
        {
            "view_id": "AirportTraffic",
            "query": f"""
            WITH airport_zones AS (
                SELECT
                    'JFK' as airport_code, 40.6413, -73.7781 as coords UNION ALL
                SELECT 'LAX', 33.9425, -118.4081 UNION ALL
                SELECT 'ORD', 41.9742, -87.9073 UNION ALL
                SELECT 'DFW', 32.8998, -97.0403
            )
            SELECT
                a.airport_code,
                COUNT(DISTINCT r.hex_ident) as aircraft_count,
                AVG(r.altitude) as avg_altitude
            FROM `{project_id}.{dataset_id}.RealTimeStream` r
            CROSS JOIN airport_zones a
            WHERE r.received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 1 HOUR)
            AND ST_DWITHIN(
                ST_GEOGPOINT(r.longitude, r.latitude),
                ST_GEOGPOINT(a.coords, a.coords),
                50000  -- 50km radius
            )
            GROUP BY a.airport_code
            ORDER BY aircraft_count DESC
            """,
            "description": "Traffic per major airport"
        }
    ]

    for view_config in views:
        view_ref = client.dataset(dataset_id).table(view_config["view_id"])
        view = bigquery.Table(view_ref)
        view.view_query = view_config["query"]
        view.description = view_config["description"]

        try:
            view = client.create_table(view, exists_ok=True)
            print(f"View {view_config['view_id']} created or updated")
        except Exception as e:
            print(f"Error creating view {view_config['view_id']}: {e}")


if __name__ == "__main__":
    # Configuration
    PROJECT_ID = "scs-lg-arch-1"
    DATASET_ID = "SBS_Data"

    print("Setting up BigQuery infrastructure for ADS-B data...")

    # Create dataset and tables
    if setup_bigquery_dataset(PROJECT_ID, DATASET_ID):
        print("Dataset and tables created successfully")

        # Create analytical views
        create_analytics_views(PROJECT_ID, DATASET_ID)
        print("Analytics views created successfully")

        print("\n=== Setup Complete ===")
        print(f"Project: {PROJECT_ID}")
        print(f"Dataset: {DATASET_ID}")
        print("Tables created:")
        print("  - RealTimeStream (partitioned by received_at)")
        print("  - AircraftMetadata")
        print("  - FlightTracks (partitioned by start_time)")
        print("Views created:")
        print("  - HourlyTrafficSummary")
        print("  - ActiveFlights")
        print("  - AirportTraffic")
    else:
        print("Failed to setup BigQuery infrastructure")
