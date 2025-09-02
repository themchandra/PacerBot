import matplotlib.pyplot as plt
import re
import numpy as np

# Path to your log file (save console output to a file)
log_file = "simulation_output.txt"

# Extract data using regex
iterations = []
modes = []
duties = []
speeds = []

with open(log_file, 'r') as f:
    for line in f:
        match = re.search(r'i=(\d+) mode=(\d) duty=([\d.]+) speed=([\d.]+)', line)
        if match:
            iterations.append(int(match.group(1)))
            modes.append(int(match.group(2)))
            duties.append(float(match.group(3)))
            speeds.append(float(match.group(4)))

# Convert to numpy arrays
iterations = np.array(iterations)
times = iterations * 0.01  # Convert iterations to seconds (assuming dt=0.01s)
modes = np.array(modes)
duties = np.array(duties)
speeds = np.array(speeds)

# Create plot
plt.figure(figsize=(12, 8))

# Plot speed
plt.subplot(3, 1, 1)
plt.plot(times, speeds, 'b-', linewidth=2)
plt.ylabel('Speed (m/s)')
plt.title('PacerBot Simulation Results')
plt.grid(True)

# Plot duty cycle
plt.subplot(3, 1, 2)
plt.plot(times, duties, 'r-', linewidth=2)
plt.ylabel('Motor Duty')
plt.grid(True)

# Plot mode
plt.subplot(3, 1, 3)
plt.step(times, modes, 'g-', linewidth=2, where='post')
plt.ylabel('Mode')
plt.xlabel('Time (s)')
plt.yticks([0, 1, 2], ['IDLE', 'RUN', 'E_STOP'])
plt.grid(True)

# Save and show
plt.tight_layout()
plt.savefig('simulation_results.png', dpi=300)
plt.show()