#!/usr/bin/env python3
"""
Real-time Analytics API server integrating DisplayGUI with BigQuery analytics
C++ DisplayGUI can get real-time analysis results via HTTP requests.
"""

from flask import Flask, jsonify, request
from flask_cors import CORS
import sys
from pathlib import Path
import json
from datetime import datetime, timedelta
from google.cloud import bigquery
import threading
import time

# Add project root to Python path
project_root = Path(__file__).parent.parent
sys.path.append(str(project_root))

app = Flask(__name__)
CORS(app)

class AnalyticsAPIServer:
    def __init__(self):
        self.client = bigquery.Client('scs-lg-arch-1')
        self.cache = {}
        self.cache_timeout = 30  # 30 second cache

    def load_config(self):
        """Load configuration file"""
        try:
            with open('../config/config.json', 'r', encoding='utf-8') as f:
                return json.load(f)['bigquery']
        except Exception as e:
            print(f"Failed to load config file: {e}")
            return None

    def get_cached_result(self, cache_key):
        """Return cached result"""
        if cache_key in self.cache:
            result, timestamp = self.cache[cache_key]
            if time.time() - timestamp < self.cache_timeout:
                return result
        return None

    def set_cache(self, cache_key, result):
        """Store result in cache"""
        self.cache[cache_key] = (result, time.time())

    def get_realtime_traffic_summary(self):
        """Real-time traffic summary (for DisplayGUI map overlay)"""
        cache_key = "traffic_summary"
        cached = self.get_cached_result(cache_key)
        if cached:
            return cached

        query = """
        SELECT
            COUNT(DISTINCT hex_ident) as active_aircraft,
            COUNT(*) as total_messages,
            AVG(CASE WHEN altitude > 0 THEN altitude END) as avg_altitude,
            AVG(CASE WHEN ground_speed > 0 THEN ground_speed END) as avg_speed,
            COUNT(CASE WHEN emergency = true THEN 1 END) as emergency_count,
            COUNT(CASE WHEN alert = true THEN 1 END) as alert_count
        FROM `scs-lg-arch-1.SBS_Data.RealTimeStream`
        WHERE received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 5 MINUTE)
        """

        try:
            result = list(self.client.query(query))[0]
            summary = {
                "active_aircraft": result.active_aircraft,
                "total_messages": result.total_messages,
                "avg_altitude": round(result.avg_altitude or 0, 0),
                "avg_speed": round(result.avg_speed or 0, 0),
                "emergency_count": result.emergency_count,
                "alert_count": result.alert_count,
                "timestamp": datetime.now().isoformat()
            }
            self.set_cache(cache_key, summary)
            return summary
        except Exception as e:
            return {"error": str(e)}

    def get_aircraft_in_area(self, lat_min, lat_max, lon_min, lon_max):
        """Query aircraft in specific area (for DisplayGUI map filter)"""
        query = f"""
        SELECT DISTINCT
            hex_ident,
            callsign,
            latitude,
            longitude,
            altitude,
            ground_speed,
            track,
            emergency,
            alert,
            on_ground,
            received_at
        FROM `scs-lg-arch-1.SBS_Data.RealTimeStream`
        WHERE received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 10 MINUTE)
        AND latitude BETWEEN {lat_min} AND {lat_max}
        AND longitude BETWEEN {lon_min} AND {lon_max}
        AND latitude IS NOT NULL
        AND longitude IS NOT NULL
        ORDER BY received_at DESC
        LIMIT 500
        """

        try:
            results = list(self.client.query(query))
            aircraft_list = []
            for row in results:
                aircraft_list.append({
                    "hex_ident": row.hex_ident,
                    "callsign": row.callsign,
                    "latitude": float(row.latitude) if row.latitude else None,
                    "longitude": float(row.longitude) if row.longitude else None,
                    "altitude": int(row.altitude) if row.altitude else None,
                    "ground_speed": int(row.ground_speed) if row.ground_speed else None,
                    "track": float(row.track) if row.track else None,
                    "emergency": bool(row.emergency),
                    "alert": bool(row.alert),
                    "on_ground": bool(row.on_ground),
                    "last_seen": row.received_at.isoformat() if row.received_at else None
                })
            return {"aircraft": aircraft_list, "count": len(aircraft_list)}
        except Exception as e:
            return {"error": str(e)}

    def get_aircraft_detail(self, hex_ident):
        """Detailed information for specific aircraft
        (when clicking aircraft in DisplayGUI)"""
        query = f"""
        SELECT
            hex_ident,
            callsign,
            latitude,
            longitude,
            altitude,
            ground_speed,
            track,
            vertical_rate,
            emergency,
            alert,
            spi,
            on_ground,
            received_at
        FROM `scs-lg-arch-1.SBS_Data.RealTimeStream`
        WHERE hex_ident = '{hex_ident}'
        ORDER BY received_at DESC
        LIMIT 20
        """

        try:
            results = list(self.client.query(query))
            if not results:
                return {"error": "Aircraft not found"}

            track_points = []
            latest = results[0]

            for row in results:
                if row.latitude and row.longitude:
                    track_points.append({
                        "latitude": float(row.latitude),
                        "longitude": float(row.longitude),
                        "altitude": int(row.altitude) if row.altitude else None,
                        "timestamp": row.received_at.isoformat()
                    })

            return {
                "hex_ident": latest.hex_ident,
                "callsign": latest.callsign,
                "current_position": {
                    "latitude": float(latest.latitude) if latest.latitude else None,
                    "longitude": float(latest.longitude) if latest.longitude else None,
                    "altitude": int(latest.altitude) if latest.altitude else None,
                    "ground_speed": int(latest.ground_speed) if latest.ground_speed else None,
                    "track": float(latest.track) if latest.track else None,
                    "vertical_rate": int(latest.vertical_rate) if latest.vertical_rate else None
                },
                "status": {
                    "emergency": bool(latest.emergency),
                    "alert": bool(latest.alert),
                    "spi": bool(latest.spi),
                    "on_ground": bool(latest.on_ground)
                },
                "track_history": track_points[:10],  # Recent 10 points
                "last_update": latest.received_at.isoformat()
            }
        except Exception as e:
            return {"error": str(e)}

    def get_emergency_alerts(self):
        """Emergency alerts (for DisplayGUI notification system)"""
        query = """
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
        WHERE (emergency = true OR alert = true OR spi = true
               OR squawk IN ('7500', '7600', '7700'))
        AND received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 30 MINUTE)
        ORDER BY received_at DESC
        LIMIT 50
        """

        try:
            results = list(self.client.query(query))
            alerts = []
            for row in results:
                alert_type = []
                if row.emergency:
                    alert_type.append("EMERGENCY")
                if row.alert:
                    alert_type.append("ALERT")
                if row.spi:
                    alert_type.append("SPI")
                if row.squawk in ['7500', '7600', '7700']:
                    alert_type.append(f"SQUAWK_{row.squawk}")

                alerts.append({
                    "hex_ident": row.hex_ident,
                    "callsign": row.callsign,
                    "position": {
                        "latitude": float(row.latitude) if row.latitude else None,
                        "longitude": float(row.longitude) if row.longitude else None,
                        "altitude": int(row.altitude) if row.altitude else None
                    },
                    "alert_types": alert_type,
                    "squawk": row.squawk,
                    "timestamp": row.received_at.isoformat()
                })
            return {"alerts": alerts, "count": len(alerts)}
        except Exception as e:
            return {"error": str(e)}

# API server instance
analytics_server = AnalyticsAPIServer()

# API endpoints
@app.route('/api/traffic/summary', methods=['GET'])
def traffic_summary():
    """Real-time traffic summary"""
    return jsonify(analytics_server.get_realtime_traffic_summary())

@app.route('/api/aircraft/area', methods=['GET'])
def aircraft_in_area():
    """Aircraft query by region"""
    lat_min = float(request.args.get('lat_min', 0))
    lat_max = float(request.args.get('lat_max', 0))
    lon_min = float(request.args.get('lon_min', 0))
    lon_max = float(request.args.get('lon_max', 0))

    return jsonify(analytics_server.get_aircraft_in_area(lat_min, lat_max, lon_min, lon_max))

@app.route('/api/aircraft/<hex_ident>', methods=['GET'])
def aircraft_detail(hex_ident):
    """Detailed information for specific aircraft"""
    return jsonify(analytics_server.get_aircraft_detail(hex_ident))

@app.route('/api/alerts/emergency', methods=['GET'])
def emergency_alerts():
    """Emergency alerts"""
    return jsonify(analytics_server.get_emergency_alerts())

@app.route('/api/health', methods=['GET'])
def health_check():
    """Server health check"""
    return jsonify({
        "status": "healthy",
        "timestamp": datetime.now().isoformat(),
        "cache_size": len(analytics_server.cache)
    })

if __name__ == '__main__':
    print("🚀 ADS-B Analytics API Server Starting...")
    print("📡 DisplayGUI integration ready")
    print("🌐 API Endpoints:")
    print("   GET /api/traffic/summary - Real-time traffic summary")
    print("   GET /api/aircraft/area?lat_min=&lat_max=&lon_min=&lon_max= - Aircraft by region")
    print("   GET /api/aircraft/{hex_ident} - Aircraft detailed information")
    print("   GET /api/alerts/emergency - Emergency alerts")
    print("   GET /api/health - Server status")
    print("=" * 60)

    app.run(host='0.0.0.0', port=8080, debug=False)
