import matplotlib.pyplot as plt
from collections import defaultdict

# Create empty dictionaries to store data
queues = defaultdict(list)
ticks = {}

# Read the text file
with open('logs32.txt', 'r') as file:
    lines = file.readlines()

# Process each line in the text file
for line in lines:
    pid, queue_num, num_ticks = map(int, line.strip().split())    
    # Store data in dictionaries
    if pid not in queues:
        queues[pid] = {'ticks': [], 'queue_numbers': []}
    queues[pid]['ticks'].append(num_ticks)
    queues[pid]['queue_numbers'].append(queue_num)

# Create the graph
plt.figure(figsize=(10, 6))

for pid, data in queues.items():
    plt.plot(data['ticks'], data['queue_numbers'], label=f'PID {pid}')

plt.xlabel('Number of Ticks')
plt.ylabel('Queue Number')
plt.title('Process Queues Over Time')
plt.legend()
plt.grid(True)
plt.show()
