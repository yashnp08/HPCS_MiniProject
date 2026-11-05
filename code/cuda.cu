
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <climits>
#include <iomanip>
#include <cuda_runtime.h>
#include <thrust/device_ptr.h>
#include <thrust/scan.h>

using namespace std;

#define BLOCK_SIZE 256

// CUDA error checking
#define CUDA_CHECK(call) do { \
    cudaError_t err = call; \
    if (err != cudaSuccess) { \
        cerr << "CUDA Error: " << cudaGetErrorString(err) \
             << " at line " << __LINE__ << endl; \
        return 1; \
    } \
} while (0)


// ============================================================
// STEP 2: KERNEL DESIGN
// ============================================================


// STEP 3: KERNEL IMPLEMENTATION
// Kernel 1: Count occurrences of each element

__global__ void pigeonhole_count(
    const int* arr,
    int n,
    int min_val,
    int* histogram)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    if (idx < n)
    {
        int bucket = arr[idx] - min_val;

        atomicAdd(&histogram[bucket], 1);
    }
}


// Kernel 2: Exclusive prefix sum
// Implemented using Thrust on GPU


// Kernel 3: Generate sorted output

__global__ void pigeonhole_output(
    const int* arr,
    int n,
    int min_val,
    const int* offsets,
    int* cursor,
    int* sorted_arr)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    if (idx < n)
    {
        int value = arr[idx];
        int bucket = value - min_val;

        int position = atomicAdd(&cursor[bucket], 1);

        sorted_arr[offsets[bucket] + position] = value;
    }
}


// ============================================================
// MAIN PROGRAM
// ============================================================

int main()
{
    cout << "\n========================================\n";
    cout << "       CUDA PIGEONHOLE SORTING\n";
    cout << "========================================\n";


    // ========================================================
    // STEP 1: MEMORY ALLOCATION
    // ========================================================

    ifstream file("dataset.txt");

    if (!file.is_open())
    {
        cerr << "ERROR: Cannot open dataset.txt\n";
        return 1;
    }

    vector<int> arr;
    int value;

    while (file >> value)
    {
        arr.push_back(value);
    }

    if (!file.eof())
    {
        cerr << "ERROR: Invalid dataset format.\n";
        return 1;
    }

    file.close();

    if (arr.empty())
    {
        cerr << "ERROR: Dataset is empty.\n";
        return 1;
    }

    if (arr.size() > static_cast<size_t>(INT_MAX))
    {
        cerr << "ERROR: Dataset too large.\n";
        return 1;
    }

    int n = static_cast<int>(arr.size());

    int min_val = *min_element(arr.begin(), arr.end());
    int max_val = *max_element(arr.begin(), arr.end());

    long long range64 =
        static_cast<long long>(max_val) -
        static_cast<long long>(min_val) + 1;

    if (range64 <= 0 || range64 > INT_MAX)
    {
        cerr << "ERROR: Value range too large.\n";
        return 1;
    }

    int range_size = static_cast<int>(range64);

    cout << "\nDATASET STATISTICS\n";
    cout << "----------------------------------------\n";
    cout << "Total Elements    : " << n << endl;
    cout << "Minimum Value     : " << min_val << endl;
    cout << "Maximum Value     : " << max_val << endl;
    cout << "Pigeonhole Range  : " << range_size << endl;


    // Allocate GPU memory

    int *d_arr = nullptr;
    int *d_histogram = nullptr;
    int *d_offsets = nullptr;
    int *d_cursor = nullptr;
    int *d_sorted = nullptr;

    CUDA_CHECK(cudaMalloc(
        (void**)&d_arr, n * sizeof(int)));

    CUDA_CHECK(cudaMalloc(
        (void**)&d_histogram, range_size * sizeof(int)));

    CUDA_CHECK(cudaMalloc(
        (void**)&d_offsets, range_size * sizeof(int)));

    CUDA_CHECK(cudaMalloc(
        (void**)&d_cursor, range_size * sizeof(int)));

    CUDA_CHECK(cudaMalloc(
        (void**)&d_sorted, n * sizeof(int)));

    // Initialize histogram and cursor

    CUDA_CHECK(cudaMemset(
        d_histogram, 0, range_size * sizeof(int)));

    CUDA_CHECK(cudaMemset(
        d_cursor, 0, range_size * sizeof(int)));


    // ========================================================
    // STEP 1: DATA TRANSFER (HOST TO DEVICE)
    // ========================================================

    CUDA_CHECK(cudaMemcpy(
        d_arr,
        arr.data(),
        n * sizeof(int),
        cudaMemcpyHostToDevice));


    // ========================================================
    // STEP 2: KERNEL DESIGN AND CONFIGURATION
    // ========================================================

    int blockSize = BLOCK_SIZE;
    int gridSize = (n + blockSize - 1) / blockSize;

    cout << "\nCUDA CONFIGURATION\n";
    cout << "----------------------------------------\n";
    cout << "Threads per Block : " << blockSize << endl;
    cout << "Number of Blocks  : " << gridSize << endl;


    // ========================================================
    // STEP 3: KERNEL IMPLEMENTATION
    // ========================================================

    cudaEvent_t start, stop;

    CUDA_CHECK(cudaEventCreate(&start));
    CUDA_CHECK(cudaEventCreate(&stop));

    CUDA_CHECK(cudaEventRecord(start));

    // Kernel 1: Pigeonhole counting

    pigeonhole_count<<<gridSize, blockSize>>>(
        d_arr, n, min_val, d_histogram);

    CUDA_CHECK(cudaGetLastError());


    // Kernel 2: Exclusive prefix sum

    thrust::device_ptr<int> hist_ptr(d_histogram);
    thrust::device_ptr<int> offset_ptr(d_offsets);

    thrust::exclusive_scan(
        hist_ptr,
        hist_ptr + range_size,
        offset_ptr);


    // Kernel 3: Sorted output generation

    pigeonhole_output<<<gridSize, blockSize>>>(
        d_arr,
        n,
        min_val,
        d_offsets,
        d_cursor,
        d_sorted);

    CUDA_CHECK(cudaGetLastError());

    CUDA_CHECK(cudaEventRecord(stop));
    CUDA_CHECK(cudaEventSynchronize(stop));

    float gpu_time = 0;

    CUDA_CHECK(cudaEventElapsedTime(
        &gpu_time, start, stop));


    // ========================================================
    // STEP 4: DATA TRANSFER (DEVICE TO HOST)
    // ========================================================

    vector<int> sorted_arr(n);

    CUDA_CHECK(cudaMemcpy(
        sorted_arr.data(),
        d_sorted,
        n * sizeof(int),
        cudaMemcpyDeviceToHost));


    // ========================================================
    // CORRECTNESS VERIFICATION
    // ========================================================

    vector<int> reference = arr;

    // CPU sorting used only for verification

    sort(reference.begin(), reference.end());

    bool correct = (sorted_arr == reference);


    // ========================================================
    // FINAL RESULTS
    // ========================================================

    cout << "\n========================================\n";
    cout << "          FINAL RESULTS\n";
    cout << "========================================\n";

    cout << "\nDATASET STATISTICS\n";
    cout << "----------------------------------------\n";
    cout << "Total Elements    : " << n << endl;
    cout << "Minimum Value     : " << min_val << endl;
    cout << "Maximum Value     : " << max_val << endl;
    cout << "Pigeonhole Range  : " << range_size << endl;

    cout << "\nCUDA CONFIGURATION\n";
    cout << "----------------------------------------\n";
    cout << "Threads per Block : " << blockSize << endl;
    cout << "Number of Blocks  : " << gridSize << endl;

    cout << "\nPERFORMANCE\n";
    cout << "----------------------------------------\n";
    cout << fixed << setprecision(5);
    cout << "GPU Sorting Time  : " << gpu_time << " ms\n";


    // Display original and sorted array samples

    cout << "\nSORTING VERIFICATION\n";
    cout << "----------------------------------------\n";

    cout << "Original Array (First 20):\n";

    for (int i = 0; i < min(n, 20); i++)
        cout << arr[i] << " ";

    cout << "\n\nSorted Array (First 20):\n";

    for (int i = 0; i < min(n, 20); i++)
        cout << sorted_arr[i] << " ";

    cout << "\n\nSorted Array (Last 20):\n";

    for (int i = max(0, n - 20); i < n; i++)
        cout << sorted_arr[i] << " ";

    cout << "\n\nCORRECTNESS\n";
    cout << "----------------------------------------\n";

    if (correct)
        cout << "SUCCESS - Array correctly sorted\n";
    else
        cout << "FAILURE - Sorting verification failed\n";

    cout << "========================================\n";


    // ========================================================
    // STEP 4: MEMORY DEALLOCATION
    // ========================================================

    CUDA_CHECK(cudaEventDestroy(start));
    CUDA_CHECK(cudaEventDestroy(stop));

    CUDA_CHECK(cudaFree(d_arr));
    CUDA_CHECK(cudaFree(d_histogram));
    CUDA_CHECK(cudaFree(d_offsets));
    CUDA_CHECK(cudaFree(d_cursor));
    CUDA_CHECK(cudaFree(d_sorted));

    return correct ? 0 : 1;
}

