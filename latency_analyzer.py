import re
import statistics

NUM_VMS = 5  # vm1.log to vm5.log

# regex
latency_pattern = re.compile(r"\[Computed Latency\]:\s+([0-9.]+)\s+ms!")

all_latencies = []

for vm_id in range(1, NUM_VMS+1):
    filename = f"vm{vm_id}.log"
    vm_latencies = []

    try:
        with open(filename, "r") as f:
            for line in f:
                match = latency_pattern.search(line)
                if match:
                    vm_latencies.append(float(match.group(1)))
    except FileNotFoundError:
        print(f"Warning: {filename} not found, skipping.")
        continue

    if vm_latencies:
        print(f"\n--- Statistics for {filename} ---")
        print(f"Samples: {len(vm_latencies)}")
        print(f"Min: {min(vm_latencies):.4f} ms")
        print(f"Max: {max(vm_latencies):.4f} ms")
        print(f"Average: {statistics.mean(vm_latencies):.4f} ms")
        if len(vm_latencies) > 1:
            print(f"Std Dev: {statistics.stdev(vm_latencies):.4f} ms")
        else:
            print("Std Dev: N/A (only 1 sample)")
        all_latencies.extend(vm_latencies)
    else:
        print(f"{filename} contains no latency entries.")

# Overall statistics across all VMs
if all_latencies:
    print("\n=== Overall Statistics (all VMs) ===")
    print(f"Total samples: {len(all_latencies)}")
    
