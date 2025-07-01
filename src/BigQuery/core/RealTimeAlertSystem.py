#!/usr/bin/env python3
"""
Real-time aircraft situation monitoring and alert system
Integrates with DisplayGUI to detect real-time events and provide alerts.
"""

import sys
from pathlib import Path
import json
import time
import threading
from datetime import datetime, timedelta
from google.cloud import bigquery
import websockets
import asyncio
import logging

# Add project root to Python path
project_root = Path(__file__).parent.parent
sys.path.append(str(project_root))

# Logging setup
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)


class RealTimeAlertSystem:
    def __init__(self):
        self.client = bigquery.Client('scs-lg-arch-1')
        self.last_check_time = datetime.now()
        self.active_alerts = {}
        self.connected_clients = set()

    def check_emergency_situations(self):
        """Monitor emergency situations"""
        query = f"""
        SELECT
            hex_ident,
            callsign,
            latitude,
            longitude,
            altitude,
            emergency,
            alert,
            spi,
            squawk,
            received_at
        FROM `scs-lg-arch-1.SBS_Data.RealTimeStream`
        WHERE received_at >= TIMESTAMP('{self.last_check_time.isoformat()}')
        AND (emergency = true OR alert = true OR spi = true
             OR squawk IN ('7500', '7600', '7700'))
        ORDER BY received_at DESC
        """

        try:
            results = list(self.client.query(query))
            new_alerts = []

            for row in results:
                alert_key = f"{row.hex_ident}_{row.received_at}"

                if alert_key not in self.active_alerts:
                    alert_data = {
                        "type": "EMERGENCY",
                        "hex_ident": row.hex_ident,
                        "callsign": row.callsign or "Unknown",
                        "position": {
                            "latitude": float(row.latitude) if row.latitude else None,
                            "longitude": float(row.longitude) if row.longitude else None,
                            "altitude": int(row.altitude) if row.altitude else None
                        },
                        "details": {
                            "emergency": bool(row.emergency),
                            "alert": bool(row.alert),
                            "spi": bool(row.spi),
                            "squawk": row.squawk
                        },
                        "timestamp": row.received_at.isoformat(),
                        "severity": self._determine_severity(row)
                    }

                    self.active_alerts[alert_key] = alert_data
                    new_alerts.append(alert_data)

            return new_alerts

        except Exception as e:
            logger.error(f"Error checking emergency situations: {e}")
            return []

    def check_trajectory_anomalies(self):
        """Monitor trajectory anomaly situations"""
        # Detect rapid altitude changes
        query = f"""
        WITH altitude_changes AS (
            SELECT
                hex_ident,
                callsign,
                altitude,
                LAG(altitude) OVER (PARTITION BY hex_ident ORDER BY received_at) as prev_altitude,
                latitude,
                longitude,
                received_at
            FROM `scs-lg-arch-1.SBS_Data.RealTimeStream`
            WHERE received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 5 MINUTE)
            AND altitude IS NOT NULL
        )
        SELECT *
        FROM altitude_changes
        WHERE ABS(altitude - prev_altitude) > 3000
        AND received_at >= TIMESTAMP('{self.last_check_time.isoformat()}')
        """

        try:
            results = list(self.client.query(query))
            anomaly_alerts = []

            for row in results:
                alert_key = f"anomaly_{row.hex_ident}_{row.received_at}"

                if alert_key not in self.active_alerts:
                    altitude_change = abs(row.altitude - row.prev_altitude)
                    alert_data = {
                        "type": "TRAJECTORY_ANOMALY",
                        "hex_ident": row.hex_ident,
                        "callsign": row.callsign or "Unknown",
                        "position": {
                            "latitude": float(row.latitude) if row.latitude else None,
                            "longitude": float(row.longitude) if row.longitude else None,
                            "altitude": int(row.altitude)
                        },
                        "details": {
                            "anomaly_type": "RAPID_ALTITUDE_CHANGE",
                            "altitude_change": int(altitude_change),
                            "prev_altitude": int(row.prev_altitude),
                            "current_altitude": int(row.altitude)
                        },
                        "timestamp": row.received_at.isoformat(),
                        "severity": "HIGH" if altitude_change > 5000 else "MEDIUM"
                    }

                    self.active_alerts[alert_key] = alert_data
                    anomaly_alerts.append(alert_data)

            return anomaly_alerts

        except Exception as e:
            logger.error(f"Error checking trajectory anomalies: {e}")
            return []

    def check_traffic_density_alerts(self):
        """Traffic density alerts"""
        query = f"""
        SELECT
            ROUND(latitude, 1) as lat_grid,
            ROUND(longitude, 1) as lon_grid,
            COUNT(DISTINCT hex_ident) as aircraft_count,
            AVG(altitude) as avg_altitude
        FROM `scs-lg-arch-1.SBS_Data.RealTimeStream`
        WHERE received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 5 MINUTE)
        AND latitude IS NOT NULL
        AND longitude IS NOT NULL
        GROUP BY lat_grid, lon_grid
        HAVING aircraft_count > 20
        ORDER BY aircraft_count DESC
        """

        try:
            results = list(self.client.query(query))
            density_alerts = []

            for row in results:
                alert_key = f"density_{row.lat_grid}_{row.lon_grid}"

                if alert_key not in self.active_alerts:
                    alert_data = {
                        "type": "HIGH_TRAFFIC_DENSITY",
                        "area": {
                            "center_latitude": float(row.lat_grid),
                            "center_longitude": float(row.lon_grid),
                            "aircraft_count": int(row.aircraft_count),
                            "avg_altitude": int(row.avg_altitude) if row.avg_altitude else None
                        },
                        "timestamp": datetime.now().isoformat(),
                        "severity": "HIGH" if row.aircraft_count > 30 else "MEDIUM"
                    }

                    self.active_alerts[alert_key] = alert_data
                    density_alerts.append(alert_data)

            return density_alerts

        except Exception as e:
            logger.error(f"Error checking traffic density: {e}")
            return []

    def _determine_severity(self, row):
        """Determine alert severity"""
        if row.emergency or row.squawk == '7700':
            return "CRITICAL"
        elif row.squawk in ['7500', '7600'] or row.spi:
            return "HIGH"
        elif row.alert:
            return "MEDIUM"
        else:
            return "LOW"

    async def broadcast_alert(self, alert_data):
        """Broadcast alerts to connected clients"""
        if self.connected_clients:
            message = json.dumps(alert_data)
            await asyncio.gather(
                *[client.send(message) for client in self.connected_clients],
                return_exceptions=True
            )

    async def monitoring_loop(self):
        """Real-time monitoring loop"""
        while True:
            try:
                # Check various anomaly situations
                emergency_alerts = self.check_emergency_situations()
                trajectory_alerts = self.check_trajectory_anomalies()
                density_alerts = self.check_traffic_density_alerts()

                # Send all alerts to clients
                all_alerts = emergency_alerts + trajectory_alerts + density_alerts

                for alert in all_alerts:
                    alert_info = f"New alert: {alert['type']} - {alert.get('hex_ident', 'N/A')}"
                    logger.info(alert_info)
                    await self.broadcast_alert(alert)

                # Update time
                self.last_check_time = datetime.now()

                # Check every 5 seconds
                await asyncio.sleep(5)

            except Exception as e:
                logger.error(f"Monitoring loop error: {e}")
                await asyncio.sleep(10)

    async def websocket_handler(self, websocket, path):
        """WebSocket client handler"""
        self.connected_clients.add(websocket)
        logger.info(f"New client connected: {websocket.remote_address}")

        try:
            # Send initial status
            status_message = {
                "type": "CONNECTION_STATUS",
                "status": "connected",
                "timestamp": datetime.now().isoformat(),
                "active_alerts_count": len(self.active_alerts)
            }
            await websocket.send(json.dumps(status_message))

            # Wait for client messages
            async for message in websocket:
                try:
                    data = json.loads(message)
                    # Handle client requests (e.g., area alert requests)
                    if data.get("type") == "REQUEST_AREA_ALERTS":
                        # Send only alerts for specific area
                        pass
                except json.JSONDecodeError:
                    logger.warning(f"Invalid JSON message: {message}")

        except websockets.exceptions.ConnectionClosed:
            client_addr = websocket.remote_address
            logger.info(f"Client connection closed: {client_addr}")
        finally:
            self.connected_clients.discard(websocket)


async def main():
    """Main execution function"""
    logger.info("🚨 Real-time alert system starting")

    alert_system = RealTimeAlertSystem()

    # Start WebSocket server
    logger.info("📡 Starting WebSocket server (port 8081)")
    websocket_server = websockets.serve(
        alert_system.websocket_handler,
        "localhost",
        8081
    )

    # Run monitoring loop and WebSocket server in parallel
    await asyncio.gather(
        websocket_server,
        alert_system.monitoring_loop()
    )


if __name__ == "__main__":
    print("🚨 ADS-B Real-time Alert System")
    print("=" * 50)
    print("📡 WebSocket Server: ws://localhost:8081")
    print("🔍 Monitoring targets:")
    print("   - Emergency situations (Emergency, Alert, SPI)")
    print("   - Trajectory anomalies (rapid altitude changes)")
    print("   - Traffic density alerts")
    print("=" * 50)

    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        logger.info("System shutdown")
