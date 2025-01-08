import matplotlib.pyplot as plt
import numpy as np

# safe_open
# x = [0, 1, 2, 5, 10, 20, 40]
# y = [1, 3, 5, 11, 21, 41, 81]
x = [1, 2, 3, 4, 5, 6, 7, 8, 9]
y = [46.82, 55.48, 68.10, 84.52, 93.58, 106.16, 134.30, 126.92, 140.88]

plt.xticks([3, 6, 9])
plt.yticks([0, 100, 200])
# plt.xlim(right=32)
# plt.ylim(top=4000)
plt.plot(x, y, label="safe_open")

# unsafe_open
x = [1, 2, 3, 4, 5, 6, 7, 8, 9]
y = [8.62, 9.14, 10.42, 9.60, 10.20, 10.76, 12.60, 10.96, 10.34]

plt.xticks([3, 6, 9])
plt.yticks([0, 100, 200])

plt.xlabel('Path length')
plt.ylabel('Duration [μs] (averaged over 50 trials)')

plt.plot(x, y, label="unsafe_open")
plt.legend()
plt.savefig("benchmarks/monitor/out/performance.pdf")
plt.clf()
