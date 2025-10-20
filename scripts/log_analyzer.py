import re
import sys
from collections import defaultdict
import pprint

def analyze_vehicle_log(log_data: str) -> dict:
    """
    Analyzes vehicle component log data to extract key statistics.

    This function parses a given log string to count events like component
    starts, messages sent/received by type, and specific handler actions.
    It's designed to be robust against minor typos in the log.

    Args:
        log_data: A string containing the log file content.

    Returns:
        A dictionary containing the compiled statistics.
    """
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
        'Components Started': stats['components_started'],
        'Messages Sent': dict(stats['messages_sent']),
        'Packets Received': dict(stats['packets_received']),
        'TEDS Handler Events': dict(stats['teds_handler']),
        'Latency Handler Events': dict(stats['latency_handler'])
    }

    return final_stats

if __name__ == "__main__":
    # Check if a filename was provided as a command-line argument
    if len(sys.argv) < 2:
        print("Error: Please provide a log file path as an argument.")
        print("Usage: python log_analyzer.py <path_to_logfile>")
        sys.exit(1) # Exit the script with an error code

    log_file_path = sys.argv[1]

    try:
        with open(log_file_path, 'r') as file:
            log_data = file.read()
    except FileNotFoundError:
        print(f"Error: The file '{log_file_path}' was not found.")
        sys.exit(1)

    # Get the statistics from the log data
    statistics = analyze_vehicle_log(log_data)

    # Print the results in a readable format
    print("📊 Log Analysis Results:")
    pprint.pprint(statistics)