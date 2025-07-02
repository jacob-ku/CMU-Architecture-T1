"""
BigQuery query collection for ADS-B data analysis
Analysis queries that can be used in actual operations
"""

class ADSBAnalytics:
    """Query class for ADS-B data analysis"""

    def __init__(self, project_id: str, dataset_id: str):
        self.project_id = project_id
        self.dataset_id = dataset_id
        self.table_prefix = f"`{project_id}.{dataset_id}`"

    def get_real_time_traffic_density(self, lat_min: float, lat_max: float,
                                     lon_min: float, lon_max: float) -> str:
        """Real-time aircraft density analysis for specific region"""
        return f"""
        SELECT
            -- Generate grid in 0.1 degree units (approximately 11km)
            ROUND(latitude, 1) as lat_grid,
            ROUND(longitude, 1) as lon_grid,
            COUNT(DISTINCT hex_ident) as aircraft_count,
            AVG(altitude) as avg_altitude,
            AVG(ground_speed) as avg_speed,
            STRING_AGG(DISTINCT callsign, ', ') as callsigns
        FROM {self.table_prefix}.RealTimeStream
        WHERE received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 10 MINUTE)
        AND latitude BETWEEN {lat_min} AND {lat_max}
        AND longitude BETWEEN {lon_min} AND {lon_max}
        AND latitude IS NOT NULL
        AND longitude IS NOT NULL
        GROUP BY lat_grid, lon_grid
        HAVING aircraft_count > 0
        ORDER BY aircraft_count DESC
        """

    def get_flight_patterns_by_hour(self, days_back: int = 7) -> str:
        """Flight pattern analysis by time period"""
        return f"""
        SELECT
            EXTRACT(HOUR FROM received_at) as hour_of_day,
            EXTRACT(DAYOFWEEK FROM received_at) as day_of_week,
            COUNT(DISTINCT hex_ident) as unique_aircraft,
            COUNT(*) as total_messages,
            AVG(altitude) as avg_altitude,
            AVG(ground_speed) as avg_speed,
            -- Altitude distribution
            COUNTIF(altitude < 10000) as low_altitude_count,
            COUNTIF(altitude BETWEEN 10000 AND 35000) as medium_altitude_count,
            COUNTIF(altitude > 35000) as high_altitude_count
        FROM {self.table_prefix}.RealTimeStream
        WHERE received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL {days_back} DAY)
        AND altitude IS NOT NULL
        GROUP BY hour_of_day, day_of_week
        ORDER BY day_of_week, hour_of_day
        """

    def detect_emergency_situations(self) -> str:
        """Detect emergency situations"""
        return f"""
        WITH emergency_signals AS (
            SELECT
                hex_ident,
                callsign,
                received_at,
                latitude,
                longitude,
                altitude,
                ground_speed,
                squawk,
                emergency,
                -- 7500: Hijacking, 7600: Radio failure, 7700: Emergency
                CASE
                    WHEN squawk = '7500' THEN 'HIJACKING'
                    WHEN squawk = '7600' THEN 'RADIO_FAILURE'
                    WHEN squawk = '7700' THEN 'EMERGENCY'
                    WHEN emergency = true THEN 'EMERGENCY_FLAG'
                    ELSE 'NORMAL'
                END as emergency_type
            FROM {self.table_prefix}.RealTimeStream
            WHERE received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 2 HOUR)
            AND (
                squawk IN ('7500', '7600', '7700')
                OR emergency = true
            )
        )
        SELECT
            hex_ident,
            callsign,
            emergency_type,
            COUNT(*) as signal_count,
            MIN(received_at) as first_detected,
            MAX(received_at) as last_detected,
            -- Recent position
            ARRAY_AGG(
                STRUCT(latitude, longitude, altitude, received_at)
                ORDER BY received_at DESC
                LIMIT 1
            )[OFFSET(0)] as last_position
        FROM emergency_signals
        WHERE emergency_type != 'NORMAL'
        GROUP BY hex_ident, callsign, emergency_type
        ORDER BY last_detected DESC
        """

    def analyze_airport_approach_patterns(self, airport_lat: float,
                                        airport_lon: float, radius_km: int = 50) -> str:
        """Airport approach pattern analysis"""
        return f"""
        WITH airport_traffic AS (
            SELECT
                hex_ident,
                callsign,
                latitude,
                longitude,
                altitude,
                ground_speed,
                received_at,
                -- Calculate distance from airport
                ST_DISTANCE(
                    ST_GEOGPOINT(longitude, latitude),
                    ST_GEOGPOINT({airport_lon}, {airport_lat})
                ) / 1000 as distance_km,
                -- Calculate bearing to airport
                ST_AZIMUTH(
                    ST_GEOGPOINT(longitude, latitude),
                    ST_GEOGPOINT({airport_lon}, {airport_lat})
                ) * 180 / 3.14159 as bearing_to_airport
            FROM {self.table_prefix}.RealTimeStream
            WHERE received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 4 HOUR)
            AND ST_DWITHIN(
                ST_GEOGPOINT(longitude, latitude),
                ST_GEOGPOINT({airport_lon}, {airport_lat}),
                {radius_km * 1000}
            )
        ),
        flight_sequences AS (
            SELECT
                hex_ident,
                callsign,
                -- Time-ordered position data
                ARRAY_AGG(
                    STRUCT(
                        latitude, longitude, altitude, ground_speed,
                        distance_km, bearing_to_airport, received_at
                    ) ORDER BY received_at
                ) as positions,
                COUNT(*) as position_count,
                MIN(distance_km) as closest_distance,
                MIN(altitude) as lowest_altitude
            FROM airport_traffic
            GROUP BY hex_ident, callsign
            HAVING COUNT(*) >= 5  -- At least 5 position data points
        )
        SELECT
            hex_ident,
            callsign,
            position_count,
            closest_distance,
            lowest_altitude,
            -- 접근/이탈 패턴 판단
            CASE
                WHEN closest_distance < 5 AND lowest_altitude < 2000 THEN 'LANDING'
                WHEN positions[OFFSET(0)].distance_km > positions[OFFSET(ARRAY_LENGTH(positions)-1)].distance_km
                     THEN 'APPROACHING'
                ELSE 'DEPARTING'
            END as flight_pattern,
            -- 평균 접근 속도
            AVG(pos.ground_speed) as avg_approach_speed
        FROM flight_sequences,
        UNNEST(positions) as pos
        WHERE pos.distance_km < 30  -- 30km 이내만 고려
        GROUP BY hex_ident, callsign, position_count, closest_distance, lowest_altitude, positions
        ORDER BY closest_distance ASC
        """

    def get_aircraft_performance_metrics(self, hex_ident: str = None) -> str:
        """Aircraft performance metrics analysis"""
        where_clause = f"AND hex_ident = '{hex_ident}'" if hex_ident else ""

        return f"""
        WITH flight_metrics AS (
            SELECT
                r.hex_ident,
                r.callsign,
                m.manufacturer,
                m.model,
                m.category,
                -- Calculate performance metrics
                AVG(r.altitude) as avg_altitude,
                MAX(r.altitude) as max_altitude,
                AVG(r.ground_speed) as avg_speed,
                MAX(r.ground_speed) as max_speed,
                STDDEV(r.altitude) as altitude_variation,
                STDDEV(r.ground_speed) as speed_variation,
                -- 비행 시간 계산
                TIMESTAMP_DIFF(MAX(r.received_at), MIN(r.received_at), MINUTE) as flight_duration_minutes,
                COUNT(*) as message_count
            FROM {self.table_prefix}.RealTimeStream r
            LEFT JOIN {self.table_prefix}.AircraftMetadata m
                ON r.hex_ident = m.hex_ident
            WHERE r.received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 24 HOUR)
            AND r.altitude IS NOT NULL
            AND r.ground_speed IS NOT NULL
            {where_clause}
            GROUP BY r.hex_ident, r.callsign, m.manufacturer, m.model, m.category
            HAVING COUNT(*) >= 10  -- At least 10 messages
        )
        SELECT
            hex_ident,
            callsign,
            manufacturer,
            model,
            category,
            avg_altitude,
            max_altitude,
            avg_speed,
            max_speed,
            altitude_variation,
            speed_variation,
            flight_duration_minutes,
            message_count,
            -- 효율성 지표
            CASE
                WHEN altitude_variation < 1000 THEN 'STABLE'
                WHEN altitude_variation < 3000 THEN 'MODERATE'
                ELSE 'VARIABLE'
            END as flight_stability,
            -- 속도 일관성
            CASE
                WHEN speed_variation < 50 THEN 'CONSISTENT'
                WHEN speed_variation < 100 THEN 'MODERATE'
                ELSE 'VARIABLE'
            END as speed_consistency
        FROM flight_metrics
        ORDER BY flight_duration_minutes DESC, message_count DESC
        """

    def get_route_analysis(self, days_back: int = 30) -> str:
        """Route analysis"""
        return f"""
        WITH route_segments AS (
            SELECT
                hex_ident,
                callsign,
                -- Order flight path by time
                ARRAY_AGG(
                    ST_GEOGPOINT(longitude, latitude)
                    ORDER BY received_at
                ) as route_points,
                MIN(received_at) as start_time,
                MAX(received_at) as end_time,
                COUNT(*) as point_count
            FROM {self.table_prefix}.RealTimeStream
            WHERE received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL {days_back} DAY)
            AND latitude IS NOT NULL
            AND longitude IS NOT NULL
            GROUP BY hex_ident, callsign
            HAVING COUNT(*) >= 20  -- At least 20 position points
        ),
        route_analysis AS (
            SELECT
                hex_ident,
                callsign,
                start_time,
                end_time,
                point_count,
                -- Calculate route length (approx.)
                ST_LENGTH(ST_MAKELINE(route_points)) / 1000 as route_length_km,
                -- Start and end points
                route_points[OFFSET(0)] as start_point,
                route_points[OFFSET(ARRAY_LENGTH(route_points)-1)] as end_point
            FROM route_segments
        )
        SELECT
            hex_ident,
            callsign,
            start_time,
            end_time,
            TIMESTAMP_DIFF(end_time, start_time, MINUTE) as flight_duration_minutes,
            route_length_km,
            point_count,
            -- 평균 속도 계산 (km/h)
            SAFE_DIVIDE(
                route_length_km,
                TIMESTAMP_DIFF(end_time, start_time, MINUTE) / 60
            ) as avg_ground_speed_kmh,
            -- 시작점과 끝점 좌표
            ST_Y(start_point) as start_lat,
            ST_X(start_point) as start_lon,
            ST_Y(end_point) as end_lat,
            ST_X(end_point) as end_lon,
            -- 직선 거리 대비 실제 경로 비율 (우회도)
            SAFE_DIVIDE(
                route_length_km,
                ST_DISTANCE(start_point, end_point) / 1000
            ) as route_efficiency_ratio
        FROM route_analysis
        WHERE route_length_km > 10  -- Flights of at least 10 km
        ORDER BY route_length_km DESC
        """

    def get_congestion_hotspots(self, grid_size: float = 0.1) -> str:
        """Aircraft traffic congestion hotspots analysis"""
        return f"""
        WITH traffic_grid AS (
            SELECT
                ROUND(latitude / {grid_size}) * {grid_size} as lat_grid,
                ROUND(longitude / {grid_size}) * {grid_size} as lon_grid,
                EXTRACT(HOUR FROM received_at) as hour_of_day,
                COUNT(DISTINCT hex_ident) as unique_aircraft,
                COUNT(*) as total_messages,
                AVG(altitude) as avg_altitude
            FROM {self.table_prefix}.RealTimeStream
            WHERE received_at >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 7 DAY)
            AND latitude IS NOT NULL
            AND longitude IS NOT NULL
            GROUP BY lat_grid, lon_grid, hour_of_day
        ),
        congestion_analysis AS (
            SELECT
                lat_grid,
                lon_grid,
                AVG(unique_aircraft) as avg_aircraft_per_hour,
                MAX(unique_aircraft) as peak_aircraft_count,
                ARRAY_AGG(
                    STRUCT(hour_of_day, unique_aircraft)
                    ORDER BY unique_aircraft DESC
                    LIMIT 3
                ) as peak_hours,
                AVG(avg_altitude) as typical_altitude
            FROM traffic_grid
            GROUP BY lat_grid, lon_grid
            HAVING AVG(unique_aircraft) > 5  -- Average more than 5 aircraft per hour
        )
        SELECT
            lat_grid as center_latitude,
            lon_grid as center_longitude,
            avg_aircraft_per_hour,
            peak_aircraft_count,
            peak_hours,
            typical_altitude,
            -- Congestion level
            CASE
                WHEN avg_aircraft_per_hour > 20 THEN 'HIGH'
                WHEN avg_aircraft_per_hour > 10 THEN 'MEDIUM'
                ELSE 'LOW'
            END as congestion_level
        FROM congestion_analysis
        ORDER BY avg_aircraft_per_hour DESC
        LIMIT 50
        """


# Example usage and tests
def example_usage():
    """Example of using analysis queries"""
    analytics = ADSBAnalytics("scs-lg-arch-1", "SBS_Data")

    # 1. Real-time traffic over South Korea
    korea_traffic = analytics.get_real_time_traffic_density(
        lat_min=33.0, lat_max=39.0,  # South Korea latitude range
        lon_min=125.0, lon_max=132.0  # South Korea longitude range
    )

    # 2. Incheon Airport approach patterns
    icn_patterns = analytics.analyze_airport_approach_patterns(
        airport_lat=37.4602, airport_lon=126.4407  # Incheon Airport coordinates
    )

    # 3. Emergency situation detection
    emergency_query = analytics.detect_emergency_situations()

    return {
        "korea_traffic": korea_traffic,
        "icn_patterns": icn_patterns,
        "emergency_detection": emergency_query
    }


if __name__ == "__main__":
    queries = example_usage()
    print("=== ADS-B Analytics Queries Generated ===")
    for name, query in queries.items():
        print(f"\n{name.upper()}:")
        print(query[:200] + "..." if len(query) > 200 else query)
