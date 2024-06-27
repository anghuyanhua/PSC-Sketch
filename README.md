# PSC-Sketch
## Introduction
Periodicity and fluctuation are two crucial characteristics of data streams. This paper investigates a novel data stream pattern called periodic spread changer. The spread changer refers to a heavy change in the spread of flows between adjacent time windows. The PSC flows refer to those flows experiencing heavy spread change with fixed time intervals. Finding PSC flows has significant applications in multiple fields, including network anomaly detection, financial transaction detection, and recommendation systems. In this paper, we propose  PSC Sketch, designed to achieve real-time and accurate PSC flow detection in limited memory space. PSC Sketch first performs spread estimation by removing duplicate data items and then filters out these flows with small spreads. During the measurement period, PSC Sketch detects spread changers, calculates the time intervals between adjacent heavy changes, and reports the top-k periodic spread changers. Extensive experiments based on four real-world datasets demonstrate that, compared to competing algorithms, PSC Sketch achieves an average of 11.47 times lower average absolute error, 1.29 times higher accuracy, and 2.78 times higher throughput.
## Repository structure
- `PSC_Sketch/` : the implementation of our algorithm.
- `Compared_Solution/` : the implementation of Three Comparative Algorithms: Primitive Solution, BH-BT, and BloomFilter+PeriodicSktch.
- `main/` : Source files for algorithm performance comparison.
- `lib/` : Source file for hash functions.
