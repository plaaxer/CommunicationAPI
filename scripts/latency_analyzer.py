import re
import sys
import os
import glob
from collections import defaultdict
import math

def analyze_vehicle_log(log_data: str) -> dict:
    """Analyzes vehicle component log data to extract general statistics."""
    stats = {
        'components_started': 0,
        'messages_sent': defaultdict(int),
        'packets_received': defaultdict(int),
        'teds_handler': defaultdict(int),
        'latency_handler': defaultdict(int),
        'pids': set()
    }

    pid_pattern = re.compile(r'PID:?\s(\d+)')
    pkt_received_pattern = re.compile(r'├─ Type: (\w+)')
    teds_response_pattern = re.compile(r'Received RESPONSE for TEDS type: (\w+)')

    for line in log_data.splitlines():
        pids_found = pid_pattern.findall(line)
        for pid in pids_found:
            stats['pids'].add(pid)

        if "[Communicator] Sending message of" in line:
            msg_type = line.split()[-1]
            stats['messages_sent'][msg_type] += 1
            continue

        if "--- Starting Component:" in line:
            stats['components_started'] += 1
        elif "Received INTEREST" in line:
            stats['teds_handler']['INTEREST_received'] += 1
        elif "Sending RESPONSE to" in line:
            stats['teds_handler']['RESPONSE_sent'] += 1
        elif "Sending PING to" in line:
            stats['latency_handler']['PING_sent'] += 1
        elif "Sending ECHO to" in line:
            stats['latency_handler']['ECHO_sent'] += 1

        match = pkt_received_pattern.search(line)
        if match:
            pkt_type = match.group(1)
            stats['packets_received'][pkt_type] += 1
            continue
        
        match = teds_response_pattern.search(line)
        if match:
            response_type = match.group(1)
            stats['teds_handler'][f'RESPONSE_received_{response_type}'] += 1
            continue

    final_stats = {
        'Total Unique PIDs': len(stats['pids']),
        'Components Started': stats['components_started'],
        'Messages Sent': dict(stats['messages_sent']),
        'Packets Received': dict(stats['packets_received']),
        'TEDS Handler Events': dict(stats['teds_handler']),
        'Latency Handler Events': dict(stats['latency_handler'])
    }
    
    return final_stats

def calculate_average_latency(log_data: str) -> dict:
    """Calculates latency stats split by SharedMemory and External origins."""
    latency_pattern = re.compile(
        r'\[Computed Latency\]: (\d+\.?\d*) ms! \| Echo origin: ([\dA-Fa-f:.]+)'
    )

    latencies = {'SharedMemory': [], 'External': []}

    for match in latency_pattern.findall(log_data):
        rtt_ms, origin = match
        rtt_ms = float(rtt_ms)
        if origin.startswith("00:00:00:00:00:00"):
            latencies['SharedMemory'].append(rtt_ms)
        else:
            latencies['External'].append(rtt_ms)

    result = {}
    for key in latencies:
        values = latencies[key]
        if not values:
            result[key] = {
                "average_latency_ms": "N/A",
                "notes": f"No latency entries found for {key}.",
            }
        else:
            values.sort()  # Sort the list to find min, max, and percentiles
            n = len(values)
            
            # Helper function for percentile calculation (linear interpolation)
            def get_percentile(p):
                if n == 0: return 0.0
                if n == 1: return values[0]
                
                # Find the index
                k_index = (p / 100) * (n - 1)
                f = math.floor(k_index)
                c = math.ceil(k_index)
                
                if f == c:
                    # Index is an integer
                    return values[int(k_index)]
                
                # Index is a float, interpolate
                d0 = values[int(f)] * (c - k_index)
                d1 = values[int(c)] * (k_index - f)
                return d0 + d1
            
            p50 = get_percentile(50)
            p90 = get_percentile(90)
            p95 = get_percentile(95)
            p99 = get_percentile(99)

            result[key] = {
                "average_latency_ms": f"{sum(values) / n:.4f}",
                "measurements_count": n,
                "min_latency_ms": f"{values[0]:.4f}",      # Min of sorted list
                "max_latency_ms": f"{values[-1]:.4f}",     # Max of sorted list
                "p50_latency_ms (median)": f"{p50:.4f}",
                "p90_latency_ms": f"{p90:.4f}",
                "p95_latency_ms": f"{p95:.4f}",
                "p99_latency_ms": f"{p99:.4f}",
            }

    return result

def print_report_block(stats: dict, title: str):
    """
    Formats and prints a single analysis report block.
    This function is MODIFIED to be reusable.
    """
    print(f"\n{title}")
    
    # Latency Analysis
    if 'Latency Analysis (ms)' in stats:
        print("\n## Latency Analysis (Split by Origin)")
        latency_results = stats['Latency Analysis (ms)']
        if not latency_results or all(v['average_latency_ms'] == 'N/A' for v in latency_results.values()):
             print("  No latency data found in this log.")
        else:
            for origin_type, latency_data in stats['Latency Analysis (ms)'].items():
                print(f"\n### {origin_type}")
                if latency_data.get('average_latency_ms') == 'N/A':
                    print(f"  {latency_data.get('notes', 'No data available.')}")
                else:
                    print(f"  {'Average Latency:':<25} {latency_data['average_latency_ms']} ms")
                    print(f"  {'Minimum Latency:':<25} {latency_data['min_latency_ms']} ms")
                    print(f"  {'Maximum Latency:':<25} {latency_data['max_latency_ms']} ms")
                    print(f"  {'Measurements Taken:':<25} {latency_data['measurements_count']}")
                    print(f"  {'P50 (Median) Latency:':<25} {latency_data['p50_latency_ms (median)']} ms")
                    print(f"  {'P90 Latency:':<25} {latency_data['p90_latency_ms']} ms")
                    print(f"  {'P95 Latency:':<25} {latency_data['p95_latency_ms']} ms")
                    print(f"  {'P99 Latency:':<25} {latency_data['p99_latency_ms']} ms")

    # General Statistics
    print("\n## General Statistics")
    for key, value in stats.items():
        if key == 'Latency Analysis (ms)':
            continue
        if isinstance(value, dict) and value:
            print(f"\n### {key}")
            for sub_key, sub_value in value.items():
                print(f"  {sub_key:<25}: {sub_value}")
        elif not isinstance(value, dict):
            print(f"  {key:<27}: {value}")

    print("\n-----------------------------------")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python log_analyzer.py <path_to_log_directory>")
        sys.exit(1)

    log_dir_path = sys.argv[1]

    if not os.path.isdir(log_dir_path):
        print(f"Error: The path '{log_dir_path}' is not a valid directory.")
        sys.exit(1)

    # Find all .log files in the specified directory
    log_files = glob.glob(os.path.join(log_dir_path, '*.log'))

    if not log_files:
        print(f"Error: No '.log' files found in '{log_dir_path}'.")
        sys.exit(1)

    all_log_data = ""
    per_file_reports = {}

    print(f"Found {len(log_files)} log files. Analyzing...")

    # 1. Analyze each file individually
    for log_file in log_files:
        file_name = os.path.basename(log_file)
        try:
            with open(log_file, 'r') as file:
                file_data = file.read()
            
            # Combine data for the aggregated report
            all_log_data += file_data + "\n"
            
            # Generate per-file report
            file_stats = analyze_vehicle_log(file_data)
            file_latency = calculate_average_latency(file_data)
            file_stats['Latency Analysis (ms)'] = file_latency
            
            per_file_reports[file_name] = file_stats

        except Exception as e:
            print(f"Error reading or analyzing {file_name}: {e}")

    # 2. Analyze the aggregated data
    if all_log_data:
        agg_stats = analyze_vehicle_log(all_log_data)
        agg_latency = calculate_average_latency(all_log_data)
        agg_stats['Latency Analysis (ms)'] = agg_latency
        
        # Print the aggregated report
        print_report_block(agg_stats, "--- Overall Aggregated Report (All Vehicles) ---")
    
    # # 3. Print the per-file reports
    # print("\n\n=================================================")
    # print("           INDIVIDUAL VEHICLE REPORTS")
    # print("=================================================")
    
    # for file_name, report_data in per_file_reports.items():
    #     print_report_block(report_data, f"--- Report for {file_name} ---")