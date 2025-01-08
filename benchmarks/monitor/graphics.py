import matplotlib.pyplot as plt
import numpy as np

# safe_open
# x = [0, 1, 2, 5, 10, 20, 40]
# y = [1, 3, 5, 11, 21, 41, 81]
x = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
y1 = [2, 9, 9, 9, 10, 10, 10, 9, 10, 10]
y2 = [95, 104, 105, 101, 111, 115, 129, 124, 127, 123]
y3 = [276, 352, 419, 512, 588, 685, 775, 906, 912, 1004]
y4 = [46, 60, 69, 83, 98, 110, 120, 191, 162, 152]

# plt.xticks([3, 6, 9])
# plt.yticks([0, 100, 200])
# plt.xlim(right=32)
# plt.ylim(top=4000)
plt.plot(x, y1, label="unsafe_open")
plt.plot(x, y2, label="krace")
plt.plot(x, y3, label="atomic_krace")
plt.plot(x, y4, label="safe_open")

plt.xlabel('Path length')
plt.ylabel('Duration [μs] (averaged over 500 trials)')

plt.legend()
plt.savefig("benchmarks/monitor/out/performance.pdf")
plt.clf()
