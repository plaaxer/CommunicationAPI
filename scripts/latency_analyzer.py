import re
import sys
from collections import defaultdict
import math  # <-- Replaced numpy with the standard math module

def analyze_vehicle_log(log_data: str) -> dict:
    """Analyzes vehicle component log data to extract general statistics."""
    # ... (This function remains unchanged) ...
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
            # --- MODIFIED SECTION: Pure Python Stats ---
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
            # --- END MODIFIED SECTION ---

    return result

def print_results(stats: dict):
    """Formats and prints the analysis results in a clean, readable report."""
    print("\n--- Log Analysis Report ---")
    
    # ... (Other print sections remain commented as in your original) ...

    # Latency Analysis
    if 'Latency Analysis (ms)' in stats:
        print("\n## Latency Analysis (Split by Origin)")
        for origin_type, latency_data in stats['Latency Analysis (ms)'].items():
            print(f"\n### {origin_type}")
            if latency_data.get('average_latency_ms') == 'N/A':
                print(f"  {latency_data.get('notes', 'No data available.')}")
            else:
                # This print section is unchanged, just reads the new dict keys
                print(f"  {'Average Latency:':<25} {latency_data['average_latency_ms']} ms")
                print(f"  {'Minimum Latency:':<25} {latency_data['min_latency_ms']} ms")
                print(f"  {'Maximum Latency:':<25} {latency_data['max_latency_ms']} ms")
                print(f"  {'Measurements Taken:':<25} {latency_data['measurements_count']}")
                print(f"  {'P50 (Median) Latency:':<25} {latency_data['p50_latency_ms (median)']} ms")
                print(f"  {'P90 Latency:':<25} {latency_data['p90_latency_ms']} ms")
                print(f"  {'P95 Latency:':<25} {latency_data['p95_latency_ms']} ms")
                print(f"  {'P99 Latency:':<25} {latency_data['p99_latency_ms']} ms")
    
    print("\n---------------------------\n")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python log_analyzer.py <path_to_logfile>")
        sys.exit(1)

    log_file_path = sys.argv[1]

    try:
        with open(log_file_path, 'r') as file:
            log_data = file.read()
    except FileNotFoundError:
        print(f"Error: The file '{log_file_path}' was not found.")
        sys.exit(1)
            
    # Run analyses
    statistics = analyze_vehicle_log(log_data)
    latency_stats = calculate_average_latency(log_data)
    
    # Combine results and print the formatted report
    statistics['Latency Analysis (ms)'] = latency_stats
    print_results(statistics)