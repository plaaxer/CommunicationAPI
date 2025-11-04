import re
import statistics
import argparse
import sys

def analyze_ptp_skew(log_content):
    """
    Parses log data to find PTP offsets and calculates both the net
    and absolute average skew rates.
    
    Args:
        log_content (str): A string containing the log file data.
    """
    
    # Regex to find: "[PTP] Clock synchronized. Offset: " (number) " ms."
    pattern = re.compile(r"\[PTP\] Clock synchronized\. Offset: (-?\d+) ms\.")
    
    offsets = []
    
    # Find all offsets in the log data
    for line in log_content.splitlines():
        match = pattern.search(line)
        if match:
            offsets.append(int(match.group(1)))
            
    if len(offsets) < 2:
        print(f"Error: Could not find enough PTP offset samples in the file.")
        print(f"Found {len(offsets)} samples.")
        return

    # --- Calculations ---
    
    total_events = len(offsets)
    
    signed_differences = []
    abs_differences = []
    
    for i in range(1, total_events):
        # Calculate the raw, signed change (e.g., -4 - (-3) = -1)
        skew = offsets[i] - offsets[i-1]
        signed_differences.append(skew)
        
        # Calculate the absolute magnitude of the change (e.g., abs(-1) = 1)
        abs_differences.append(abs(skew))

    if not signed_differences:
        print("Found only one sync event. Cannot calculate differences.")
        return
        
    # Calculate both averages
    avg_net_skew = statistics.mean(signed_differences)
    avg_abs_skew = statistics.mean(abs_differences)
    
    # --- Results ---
    
    print("--- PTP Clock Skew Analysis ---")
    print(f"Total synchronization events found: {total_events}")
    print(f"Extracted Offsets (ms): {offsets}")
    print(f"Net Skew per 10s interval (ms): {signed_differences}")
    print(f"Absolute Skew per 10s interval (ms): {abs_differences}")
    print("---------------------------------")
    print(f"Average Net Skew Rate: {avg_net_skew:.2f} ms per 10 seconds (shows bias)")
    print(f"Average Absolute Skew Rate: {avg_abs_skew:.2f} ms per 10 seconds (shows instability)")
    
    # Projections should be based on the absolute skew, as that represents
    # the magnitude of the drift you need to correct for.
    if avg_abs_skew > 0:
        # Time to skew 5ms (as per your requirement)
        intervals_to_skew_5ms = 5 / avg_abs_skew
        time_to_skew_5ms = intervals_to_skew_5ms * 10 # 10 seconds per interval
        
        # Time to skew 1 full second (1000 ms)
        intervals_to_skew_1sec = 1000 / avg_abs_skew
        time_to_skew_1sec = intervals_to_skew_1sec * 10
        
        print("\n--- Projections (based on average absolute skew) ---")
        print(f"Time to accumulate 5 ms of skew: {time_to_skew_5ms:.2f} seconds")
        print(f"Time to accumulate 1 second (1000 ms) of skew: {time_to_skew_1sec:.2f} seconds (or {time_to_skew_1sec/60:.2f} minutes)")
    else:
        print("\n--- Projections ---")
        print("Clock appears to be stable (average absolute skew is 0.00 ms).")

def main():
    # Set up the command-line argument parser
    parser = argparse.ArgumentParser(description="Analyze PTP clock skew from a log file.")
    parser.add_argument("filename", help="The path to the log file to analyze.")
    args = parser.parse_args()

    # Read the log file
    try:
        with open(args.filename, "r") as f:
            file_content = f.read()
    except FileNotFoundError:
        print(f"Error: The file '{args.filename}' was not found.")
        sys.exit(1)
    except Exception as e:
        print(f"An error occurred while reading the file: {e}")
        sys.exit(1)

    # Analyze the content
    print(f"Analyzing log file: {args.filename}...")
    analyze_ptp_skew(file_content)

if __name__ == "__main__":
    main()