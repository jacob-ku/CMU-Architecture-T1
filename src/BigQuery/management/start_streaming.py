#!/usr/bin/env python3
"""
Real-time ADS-B data streaming startup script
Usage: python start_streaming.py [config_file]
"""

import sys
import json
import time
import signal
from pathlib import Path

# Add project path to sys.path
project_root = Path(__file__).parent.parent  # Move to BigQuery directory
sys.path.append(str(project_root))

try:
    # First test google-cloud-bigquery package
    try:
        from google.cloud import bigquery
        print("✅ google-cloud-bigquery package import successful")
    except ImportError as e:
        print(f"❌ google-cloud-bigquery import failed: {e}")
        print("Please reinstall with the following command:")
        print("pip install --upgrade google-cloud-bigquery")
        sys.exit(1)

    # Import project modules
    try:
        from core.ADSBBridge import ADSBBridge, ADSBConfig
        print("✅ ADSBBridge module import successful")
    except ImportError as e:
        print(f"❌ ADSBBridge module import failed: {e}")
        print("Current directory:", Path.cwd())
        print("Python path:", sys.path[:3])  # Show only first 3
        sys.exit(1)

    try:
        from core.RealTimeStreaming import ADSBStreamer
        print("✅ RealTimeStreaming module import successful")
    except ImportError as e:
        print(f"⚠️  RealTimeStreaming module import failed: {e}")
        print("Continuing...")

except ImportError as e:
    print(f"Import Error: {e}")
    print("Please install required packages:")
    print("pip install google-cloud-bigquery")
    sys.exit(1)


def load_config(config_path: str = "config/config.json") -> dict:
    """Load configuration file"""
    try:
        with open(config_path, 'r', encoding='utf-8') as f:
            return json.load(f)
    except FileNotFoundError:
        print(f"Configuration file {config_path} not found.")
        return create_default_config()
    except json.JSONDecodeError as e:
        print(f"Configuration file parsing error: {e}")
        sys.exit(1)


def create_default_config() -> dict:
    """Create default configuration"""
    return {
        "bigquery": {
            "project_id": "scs-lg-arch-1",
            "dataset_id": "SBS_Data",
            "table_id": "RealTimeStream",
            "credentials_path": "YourJsonFile.json"
        },
        "streaming": {
            "batch_size": 100
        },
        "logging": {
            "level": "INFO"
        }
    }


def main():
    print("🛩️  ADS-B BigQuery Real-time Streaming Started")
    print("=" * 50)

    # Configuration file path
    config_file = sys.argv[1] if len(sys.argv) > 1 else "config/config.json"

    # Load configuration
    config_data = load_config(config_file)
    bigquery_config = config_data.get("bigquery", {})
    streaming_config = config_data.get("streaming", {})

    # Create ADSBConfig object
    config = ADSBConfig(
        project_id=bigquery_config.get("project_id", "scs-lg-arch-1"),
        dataset_id=bigquery_config.get("dataset_id", "SBS_Data"),
        table_id=bigquery_config.get("table_id", "RealTimeStream"),
        credentials_path=bigquery_config.get("credentials_path", "YourJsonFile.json"),
        batch_size=streaming_config.get("batch_size", 100)
    )

    print(f"Project: {config.project_id}")
    print(f"Dataset: {config.dataset_id}")
    print(f"Table: {config.table_id}")
    print(f"Batch size: {config.batch_size}")
    print("-" * 50)

    # Initialize ADS-B Bridge
    bridge = ADSBBridge(config)

    if not bridge.initialize():
        print("❌ Bridge initialization failed.")
        print("\nPlease check the following:")
        print("1. Google Cloud authentication is configured")
        print("2. BigQuery API is enabled")
        print("3. Service account key file path is correct")
        sys.exit(1)

    print("✅ Bridge initialization completed")

    # Set up signal handler (exit with Ctrl+C)
    def signal_handler(sig, frame):
        print("\n\n🛑 Exit signal received. Stopping streaming...")
        bridge.stop()
        sys.exit(0)

    signal.signal(signal.SIGINT, signal_handler)

    try:
        # Start streaming
        print("🚀 Starting real-time streaming...")
        bridge.start()

        print("✅ Streaming has started!")
        print("\n📊 Real-time status:")
        print("- Press Ctrl+C to exit")
        print("- Status updated every 10 seconds")
        print("-" * 50)

        # Status monitoring loop
        while True:
            time.sleep(10)

            # Display status information
            summary = bridge.get_analytics_summary()
            print(f"\rStatus: {summary.get('status', 'unknown')} | "
                  f"Messages: {summary.get('total_messages', 0)} | "
                  f"Queue: {summary.get('queue_size', 0)} | "
                  f"Rate: {summary.get('messages_per_second', 0):.1f}/sec",
                  end="")

    except KeyboardInterrupt:
        print("\n\n🛑 User stopped the process.")
    except Exception as e:
        print(f"\n❌ Error occurred: {e}")
    finally:
        bridge.stop()
        print("\n✅ Streaming has been stopped.")


if __name__ == "__main__":
    main()
