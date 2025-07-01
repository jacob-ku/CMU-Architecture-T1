"""
Real-time ADS-B data streaming to BigQuery
Module for streaming real-time ADS-B data to BigQuery

Features:
- Real-time SBS message parsing and transmission
- Performance optimization with batch processing
- Error handling and retry logic
"""

import os
import json
import time
import threading
from datetime import datetime, timezone
from typing import List, Dict, Optional
from queue import Queue, Empty
from google.cloud import bigquery


class ADSBStreamer:
    def __init__(self, project_id: str, dataset_id: str, table_id: str,
                 credentials_path: str, batch_size: int = 100):
        """
        Initialize ADS-B data streamer

        Args:
            project_id: Google Cloud project ID
            dataset_id: BigQuery dataset ID
            table_id: BigQuery table ID
            credentials_path: Service account key file path
            batch_size: Batch processing size
        """
        # Set up BigQuery client - prioritize Application Default Credentials
        try:
            # First try Application Default Credentials
            self.client = bigquery.Client(project=project_id)
            print("✅ Using Application Default Credentials")
        except Exception as adc_error:
            # If ADC fails, try service account key file
            if credentials_path and os.path.exists(credentials_path):
                try:
                    os.environ["GOOGLE_APPLICATION_CREDENTIALS"] = credentials_path
                    self.client = bigquery.Client(project=project_id)
                    print("✅ Using service account key file")
                except Exception as key_error:
                    raise Exception(
                        f"BigQuery authentication failed:\n"
                        f"1. Application Default Credentials: {adc_error}\n"
                        f"2. Service account key: {key_error}\n\n"
                        f"Solutions:\n"
                        f"- Run 'gcloud auth application-default login'\n"
                        f"- Or set up correct service account key file"
                    )
            else:
                raise Exception(
                    f"BigQuery authentication failed: {adc_error}\n\n"
                    f"Solutions:\n"
                    f"1. Run 'gcloud auth application-default login'\n"
                    f"2. Run 'gcloud config set project {project_id}'\n"
                    f"3. Or create a service account key file"
                )
        # Set up table reference - create reference regardless of existence
        self.project_id = project_id
        self.dataset_id = dataset_id
        self.table_name = table_id
        self.table_id = f"{project_id}.{dataset_id}.{table_id}"

        try:
            self.table = self.client.get_table(self.table_id)
            print(f"✅ BigQuery table connected: {self.table_id}")
        except Exception as table_error:
            print(f"⚠️  Failed to get table directly: {table_error}")
            print(f"   Proceeding with table reference only: {self.table_id}")
            # Create table reference only (check existence later)
            dataset_ref = self.client.dataset(dataset_id)
            self.table = dataset_ref.table(table_id)
            print(f"✅ Table reference created: {self.table_id}")

        # Streaming configuration
        self.batch_size = batch_size
        self.data_queue = Queue()
        self.is_running = False
        self.worker_thread = None

    def parse_sbs_message(self, sbs_line: str) -> Optional[Dict]:
        """
        Parse SBS message and convert to BigQuery record

        Actual SBS Format (ADS-B Hub):
        MSG,transmission_type,session_id,aircraft_id,hex_ident,flight_id,
        date_generated,time_generated,date_logged,time_logged,
        callsign,altitude,ground_speed,track,lat,lon,vertical_rate,squawk,alert,emergency,spi,is_on_ground

        Example:
        MSG,3,111,11111,400F01,1111,2024/12/30,10:00:00.000,2024/12/30,10:00:00.000,UAL123,35000,450,90,40.7128,-74.0060,0,1200,0,0,0,0
        """
        try:
            parts = sbs_line.strip().split(',')
            if len(parts) < 22 or parts[0] != 'MSG':
                return None

            # Record current time in UTC
            now = datetime.now(timezone.utc)

            # Safe type conversion functions
            def safe_int(value: str) -> Optional[int]:
                try:
                    return int(value) if value and value.strip() else None
                except (ValueError, AttributeError):
                    return None

            def safe_float(value: str) -> Optional[float]:
                try:
                    return float(value) if value and value.strip() else None
                except (ValueError, AttributeError):
                    return None

            def safe_bool(value: str) -> bool:
                try:
                    return bool(int(value)) if value and value.strip() else False
                except (ValueError, AttributeError):
                    return False

            record = {
                'message_type': parts[1] if len(parts) > 1 else None,
                'session_id': parts[2] if len(parts) > 2 and parts[2] else None,
                'aircraft_id': parts[3] if len(parts) > 3 and parts[3] else None,
                'hex_ident': parts[4] if len(parts) > 4 and parts[4] else None,
                'flight_id': parts[5] if len(parts) > 5 and parts[5] else None,
                'callsign': parts[10] if len(parts) > 10 and parts[10] else None,
                'altitude': safe_int(parts[11]) if len(parts) > 11 else None,
                'ground_speed': (safe_float(parts[12])
                                if len(parts) > 12 else None),
                'track': safe_float(parts[13]) if len(parts) > 13 else None,
                'latitude': safe_float(parts[14]) if len(parts) > 14 else None,
                'longitude': (safe_float(parts[15])
                             if len(parts) > 15 else None),
                'vertical_rate': (safe_int(parts[16])
                                 if len(parts) > 16 else None),
                'squawk': parts[17] if len(parts) > 17 and parts[17] else None,
                'alert': safe_bool(parts[18]) if len(parts) > 18 else False,
                'emergency': (safe_bool(parts[19])
                             if len(parts) > 19 else False),
                'spi': safe_bool(parts[20]) if len(parts) > 20 else False,
                'is_on_ground': (safe_bool(parts[21])
                                if len(parts) > 21 else False),
                'received_at': now.isoformat(),
                'date_generated': f"{parts[6]} {parts[7]}" if len(parts) > 7 and parts[6] and parts[7] else None,
                'date_logged': f"{parts[8]} {parts[9]}" if len(parts) > 9 and parts[8] and parts[9] else None
            }

            return record

        except Exception as e:
            # Print detailed error information for debugging
            print(f"SBS parsing error: {e}")
            print(f"Original message: {sbs_line[:100]}...")  # Print first 100 chars
            return None

    def add_message(self, sbs_message: str):
        """Add SBS message to queue"""
        parsed = self.parse_sbs_message(sbs_message)
        if parsed:
            self.data_queue.put(parsed)

    def _worker_loop(self):
        """Batch processing loop executed in background thread"""
        while self.is_running:
            batch = []

            # Collect data up to batch size (wait max 1 second)
            try:
                for _ in range(self.batch_size):
                    try:
                        item = self.data_queue.get(timeout=1.0)
                        batch.append(item)
                    except Empty:
                        break

                if batch:
                    self._stream_batch(batch)

            except Exception as e:
                print(f"Error in worker loop: {e}")
                time.sleep(1)

    def _stream_batch(self, batch: List[Dict]):
        """Stream batch data to BigQuery"""
        try:
            # Insert directly using table object
            errors = self.client.insert_rows_json(self.table, batch)

            if errors:
                print(f"❌ BigQuery streaming error: {errors}")
                # Put failed data back in queue for retry
                for row in batch:
                    self.data_queue.put(row)
            else:
                print(f"✅ {len(batch)} records successfully streamed")

        except Exception as e:
            print(f"❌ BigQuery streaming failed: {e}")
            print(f"   Table ID: {self.table_id}")
            print(f"   Batch size: {len(batch)}")
            # Print first record sample for debugging
            if batch:
                print(f"   Sample record: {list(batch[0].keys())}")
            # Put failed data back in queue for retry
            for row in batch:
                self.data_queue.put(row)

    def start_streaming(self):
        """Start streaming"""
        if not self.is_running:
            self.is_running = True
            self.worker_thread = threading.Thread(target=self._worker_loop)
            self.worker_thread.daemon = True
            self.worker_thread.start()
            print("ADS-B streaming started")

    def stop_streaming(self):
        """Stop streaming"""
        if self.is_running:
            self.is_running = False
            if self.worker_thread:
                self.worker_thread.join(timeout=5)
            print("ADS-B streaming stopped")

    def get_queue_size(self) -> int:
        """Return current queue size"""
        return self.data_queue.qsize()

    def debug_sbs_message(self, sbs_line: str) -> None:
        """Debug: Analyze SBS message structure"""
        parts = sbs_line.strip().split(',')
        print(f"\n📡 SBS message analysis (total {len(parts)} fields):")
        for i, part in enumerate(parts):
            print(f"  [{i:2d}] '{part}'")
        print()


class FlightPatternAnalyzer:
    """Flight pattern analyzer with BigQuery query wrapper"""

    def __init__(self, client: bigquery.Client, project_id: str, dataset_id: str, table_id: str):
        self.client = client
        self.table_full_name = f"{project_id}.{dataset_id}.{table_id}"

    def get_recent_aircraft_count(self, minutes: int = 60) -> int:
        """Number of aircraft tracked in the last N minutes"""
        query = f"""
        SELECT COUNT(DISTINCT hex_ident) as aircraft_count
        FROM `{self.table_full_name}`
        WHERE received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL {minutes} MINUTE)
        AND hex_ident IS NOT NULL
        """
        return list(self.client.query(query))[0].aircraft_count

    def get_airport_traffic(self, airport_lat: float, airport_lon: float,
                          radius_km: float = 10, hours: int = 24) -> List[Dict]:
        """Analyze traffic around airport"""
        query = f"""
        SELECT
            callsign,
            hex_ident,
            AVG(altitude) as avg_altitude,
            COUNT(*) as message_count,
            MIN(received_at) as first_seen,
            MAX(received_at) as last_seen
        FROM `{self.table_full_name}`
        WHERE received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL {hours} HOUR)
        AND latitude IS NOT NULL AND longitude IS NOT NULL
        AND ST_DWITHIN(
            ST_GEOGPOINT(longitude, latitude),
            ST_GEOGPOINT({airport_lon}, {airport_lat}),
            {radius_km * 1000}
        )
        GROUP BY callsign, hex_ident
        ORDER BY message_count DESC
        """
        results = []
        for row in self.client.query(query):
            results.append({
                'callsign': row.callsign,
                'hex_ident': row.hex_ident,
                'avg_altitude': row.avg_altitude,
                'message_count': row.message_count,
                'first_seen': row.first_seen,
                'last_seen': row.last_seen
            })
        return results

    def detect_unusual_patterns(self) -> List[Dict]:
        """Detect unusual flight patterns"""
        query = f"""
        SELECT
            hex_ident,
            callsign,
            AVG(altitude) as avg_altitude,
            STDDEV(altitude) as altitude_variance,
            AVG(ground_speed) as avg_speed,
            COUNT(*) as message_count
        FROM `{self.table_full_name}`
        WHERE received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 1 HOUR)
        AND altitude IS NOT NULL AND ground_speed IS NOT NULL
        GROUP BY hex_ident, callsign
        HAVING
            STDDEV(altitude) > 5000  -- Large altitude changes
            OR AVG(ground_speed) > 600  -- Abnormally fast
            OR AVG(altitude) < 500  -- Too low altitude
        ORDER BY altitude_variance DESC
        """
        results = []
        for row in self.client.query(query):
            results.append({
                'hex_ident': row.hex_ident,
                'callsign': row.callsign,
                'avg_altitude': row.avg_altitude,
                'altitude_variance': row.altitude_variance,
                'avg_speed': row.avg_speed,
                'message_count': row.message_count
            })
        return results


# Usage example
if __name__ == "__main__":
    # Configuration
    PROJECT_ID = "scs-lg-arch-1"
    DATASET_ID = "SBS_Data"
    TABLE_ID = "RealTimeStream"
    CREDENTIALS_PATH = "YourJsonFile.json"

    # Initialize streamer
    streamer = ADSBStreamer(
        project_id=PROJECT_ID,
        dataset_id=DATASET_ID,
        table_id=TABLE_ID,
        credentials_path=CREDENTIALS_PATH,
        batch_size=50
    )

    # Start streaming
    streamer.start_streaming()

    # Initialize analyzer
    analyzer = FlightPatternAnalyzer(
        client=streamer.client,
        project_id=PROJECT_ID,
        dataset_id=DATASET_ID,
        table_id=TABLE_ID
    )

    try:
        # Test messages (in reality, received from ADS-B Hub)
        test_messages = [
            "MSG,3,111,11111,A12345,111111,2024/12/30,10:00:00.000,2024/12/30,10:00:00.000,,UAL123,35000,450,90,40.7128,-74.0060,0,1200,0,0,0,0",
            "MSG,3,111,11111,B67890,111112,2024/12/30,10:00:01.000,2024/12/30,10:00:01.000,,DAL456,37000,475,85,41.8781,-87.6298,500,1201,0,0,0,0"
        ]

        for msg in test_messages:
            streamer.add_message(msg)
            streamer.debug_sbs_message(msg)  # Print debug messages

        # Wait briefly then run analysis
        time.sleep(5)

        # Check recent aircraft count
        aircraft_count = analyzer.get_recent_aircraft_count(60)
        print(f"Recent aircraft count: {aircraft_count}")

    except KeyboardInterrupt:
        print("Stopping...")
    finally:
        streamer.stop_streaming()
