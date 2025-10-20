import re
import sys
from collections import defaultdict

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
    """Calculates average, min, and max latency from log entries."""
    latency_pattern = re.compile(r'\[Computed Latency\]: (\d+\.?\d*) ms!')
    latencies = [float(val) for val in latency_pattern.findall(log_data)]

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

def print_results(stats: dict):
    """Formats and prints the analysis results in a clean, readable report."""
    print("\n--- Log Analysis Report ---")
    
    # General Stats
    print("\n## Overview")
    print(f"  {'Components Started:':<25} {stats.get('Components Started', 'N/A')}")
    print(f"  {'Total Unique PIDs Found:':<25} {stats.get('Total Unique PIDs', 'N/A')}")
    
    # Message Stats
    if stats.get('Messages Sent') or stats.get('Packets Received'):
        print("\n## Message & Packet Counts")
        if 'Messages Sent' in stats:
            print("  Messages Sent:")
            for msg_type, count in stats['Messages Sent'].items():
                print(f"{' ' * 4}- {msg_type:<20} {count}")
        if 'Packets Received' in stats:
            print("  Packets Received:")
            for pkt_type, count in stats['Packets Received'].items():
                print(f"{' ' * 4}- {pkt_type:<20} {count}")

    # Handler Events
    if stats.get('TEDS Handler Events') or stats.get('Latency Handler Events'):
        print("\n## Handler Activity")
        if 'TEDS Handler Events' in stats:
            print("  TEDS Handler:")
            for event, count in stats['TEDS Handler Events'].items():
                print(f"{' ' * 4}- {event.replace('_', ' ').title():<20} {count}")
        if 'Latency Handler Events' in stats:
            print("  Latency Handler:")
            for event, count in stats['Latency Handler Events'].items():
                print(f"{' ' * 4}- {event.replace('_', ' ').title():<20} {count}")

    # Latency Analysis
    if 'Latency Analysis (ms)' in stats:
        print("\n## Latency Analysis")
        latency_data = stats['Latency Analysis (ms)']
        if latency_data.get('average_latency_ms') == 'N/A':
            print(f"  {latency_data.get('notes', 'No data available.')}")
        else:
            print(f"  {'Average Latency:':<25} {latency_data['average_latency_ms']} ms")
            print(f"  {'Minimum Latency:':<25} {latency_data['min_latency_ms']} ms")
            print(f"  {'Maximum Latency:':<25} {latency_data['max_latency_ms']} ms")
            print(f"  {'Measurements Taken:':<25} {latency_data['measurements_count']}")
    
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