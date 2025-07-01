#!/usr/bin/env python3
"""
ADS-B BigQuery Data Analysis Integrated Execution Script
Sequentially executes analysis scripts 1, 2, 3, 4, 5, and 6
"""

import subprocess
import sys
import time
from datetime import datetime

def run_analysis(script_name, analysis_name):
    """Execute individual analysis script"""
    print(f"\n{'='*70}")
    print(f"🚀 Starting {analysis_name}")
    print(f"{'='*70}")

    try:
        start_time = time.time()
        result = subprocess.run([sys.executable, script_name],
                              capture_output=True, text=True, timeout=300)
        end_time = time.time()

        if result.returncode == 0:
            print(result.stdout)
            print(f"✅ {analysis_name} completed (execution time: {end_time-start_time:.1f}s)")
        else:
            print(f"❌ Error occurred during {analysis_name} execution:")
            print(result.stderr)
            return False

    except subprocess.TimeoutExpired:
        print(f"⏰ {analysis_name} execution timeout (5 minutes)")
        return False
    except Exception as e:
        print(f"❌ Exception occurred during {analysis_name} execution: {e}")
        return False

    return True

def main():
    """Execute all analyses"""
    print("🛩️  ADS-B BigQuery Data Analysis Integrated Execution")
    print("=" * 70)
    print(f"Start time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print("=" * 70)

    # List of analyses to execute
    analyses = [
        ("analyses/analysis_01_traffic_summary.py", "Analysis 1: Real-time Traffic Summary"),
        ("analyses/analysis_02_aircraft_details.py", "Analysis 2: Aircraft Detail Analysis"),
        ("analyses/analysis_03_geographic.py", "Analysis 3: Geographic Distribution & Airport Traffic"),
        ("analyses/analysis_04_trajectory_anomaly.py", "Analysis 4: Aircraft Movement Patterns & Anomaly Detection"),
        ("analyses/analysis_05_emergency.py",
         "Analysis 5: Emergency & Special Situation Detection"),
        ("analyses/analysis_06_prediction.py",
         "Analysis 6: Predictive Analysis & Trend Forecasting"),
    ]

    success_count = 0
    total_start_time = time.time()

    for script_name, analysis_name in analyses:
        if run_analysis(script_name, analysis_name):
            success_count += 1
        else:
            print(f"\n⚠️  {analysis_name} failed - continuing...")

        # Brief wait between analyses (BigQuery load balancing)
        time.sleep(2)

    total_end_time = time.time()

    print(f"\n{'='*70}")
    print("📊 Analysis Execution Summary")
    print(f"{'='*70}")
    print(f"Success: {success_count}/{len(analyses)} analyses")
    print(f"Total execution time: {total_end_time-total_start_time:.1f}s")
    print(f"Completion time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")

    if success_count == len(analyses):
        print("🎉 All analyses completed successfully!")
    else:
        print("⚠️  Some analyses failed. Please check individual logs.")

    print(f"{'='*70}")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n❌ Interrupted by user.")
    except Exception as e:
        print(f"\n\n❌ Unexpected error occurred: {e}")
        import traceback
        traceback.print_exc()
