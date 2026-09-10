# Parallel Pigeonhole Sort using OpenMP

## Project Overview

This project implements Pigeonhole Sort using:

- Sequential C
- OpenMP parallel programming

The execution times are compared to evaluate the performance improvement obtained through parallelization.

## Dataset

The same dataset is used for both implementations to ensure a fair comparison.

## Implementations

### Sequential
The sequential implementation performs Pigeonhole Sort using a single execution flow.

### OpenMP
The OpenMP implementation parallelizes the counting and pigeonhole processing using multiple threads.

## Results

| Implementation | Threads | Average Time (ms) | Speedup |
|---|---:|---:|---:|
| Sequential | - | 9.091 | 1.00x |
| OpenMP | 1 | 6.306 | 1.44x |
| OpenMP | 2 | 6.425 | 1.42x |
| OpenMP | 4 | 6.094 | 1.49x |
| OpenMP | 8 | 5.987 | 1.52x |

## Conclusion

The OpenMP implementation achieved its best measured execution time with 8 threads.

The sequential implementation took approximately 9.091 ms,
while the 8-thread OpenMP implementation took approximately 5.987 ms,
resulting in an approximately 1.52x speedup.
