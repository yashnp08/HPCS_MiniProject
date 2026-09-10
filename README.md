# Pigeonhole Sort – Sequential vs OpenMP

## Overview

This project implements **Pigeonhole Sort** using C and compares the performance of:

- Sequential implementation
- OpenMP parallel implementation

The execution time of both implementations is measured and compared for different numbers of OpenMP threads.

## Dataset

The experiment uses the dataset:

`dataset.txt`

| Property | Value |
|---|---|
| Number of lines | 100,000 |
| Data type | Integer |
| Value range | Approximately 0–999 |
| Dataset used | Same for all implementations |

The same dataset is used for both implementations to ensure a fair comparison.

## Implementations

### Sequential

The sequential program performs Pigeonhole Sort using a single execution flow.

### OpenMP

The OpenMP version parallelizes the counting phase using multiple threads. Each thread maintains its own pigeonhole count array to avoid race conditions.

Tested configurations:

- 1 thread
- 2 threads
- 4 threads
- 8 threads

## Compilation

### Sequential

```bash
gcc seq.c -o seq
./seq
```

### OpenMP

```bash
gcc -fopenmp omp.c -o omp
```

Run with different thread counts:

```bash
OMP_NUM_THREADS=1 ./omp
OMP_NUM_THREADS=2 ./omp
OMP_NUM_THREADS=4 ./omp
OMP_NUM_THREADS=8 ./omp
```

## Results

Each configuration was executed 5 times and the average execution time was calculated.

| Implementation | Threads | Average Time (ms) | Speedup |
|---|---:|---:|---:|
| Sequential | — | 9.091 | 1.00× |
| OpenMP | 1 | 6.306 | 1.44× |
| OpenMP | 2 | 6.425 | 1.41× |
| OpenMP | 4 | 6.094 | 1.49× |
| OpenMP | 8 | 5.987 | 1.52× |

### Best Result

The best performance was obtained using **8 OpenMP threads**.

```text
Sequential : 9.091 ms
OpenMP (8) : 5.987 ms

Speedup = 1.52×
```

## Conclusion

The OpenMP implementation achieved better sorting performance than the sequential implementation for the tested dataset.

The best measured result was **5.987 ms with 8 threads**, giving an approximate **1.52× speedup** over the sequential implementation.

