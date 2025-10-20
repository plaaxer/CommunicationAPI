import re
import sys
from collections import defaultdict
import pprint

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
    """
    Calculates average, min, and max latency from log entries.

    This function specifically looks for lines in the format:
    '[Computed Latency]: 0.1234 ms!'
    """
    # Regex to find and extract the latency value (a float or integer)
    latency_pattern = re.compile(r'\[Computed Latency\]: (\d+\.?\d*) ms!')
    
    latencies = []
    for line in log_data.splitlines():
        match = latency_pattern.search(line)
        if match:
            # Extract the latency value, convert to float, and add to list
            latency_value = float(match.group(1))
            latencies.append(latency_value)

    if not latencies:
        return {
            "average_latency_ms": "N/A",
            "notes": "No '[Computed Latency]' entries found in the log."
        }

    return {
        "average_latency_ms": f"{sum(latencies) / len(latencies):.4f}",
        "measurements_count": len(latencies),
        "min_latency_ms": f"{min(latencies):.4f}",
        "max_latency_ms": f"{max(latencies):.4f}"
    }

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
            
    # Run both analyses
    statistics = analyze_vehicle_log(log_data)
    latency_stats = calculate_average_latency(log_data)
    
    # Combine the results
    statistics['Latency Analysis (ms)'] = latency_stats

    print("📊 Log Analysis Results:")
    pprint.pprint(statistics)