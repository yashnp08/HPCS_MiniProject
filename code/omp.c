#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

void pigeonhole_sort_omp(int arr[], int n, int min_val, int range_size, int sorted_arr[]) {

    int num_threads = omp_get_max_threads();

    // Each thread gets its own pigeonhole array
    int *local_counts = calloc((size_t)num_threads * range_size, sizeof(int));

    if (local_counts == NULL) {
        printf("Memory allocation failed.\n");
        exit(1);
    }

    // Count elements in parallel
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();

        int *my_counts =
            local_counts + (size_t)tid * range_size;

        #pragma omp for
        for (int i = 0; i < n; i++) {
            my_counts[arr[i] - min_val]++;
        }
    }

    // Combine the counts from all threads
    int *pigeonholes = calloc(range_size, sizeof(int));

    if (pigeonholes == NULL) {
        printf("Memory allocation failed.\n");
        free(local_counts);
        exit(1);
    }

    #pragma omp parallel for
    for (int h = 0; h < range_size; h++) {

        int total = 0;

        for (int t = 0; t < num_threads; t++) {
            total += local_counts[(size_t)t * range_size + h];
        }

        pigeonholes[h] = total;
    }

    // Reconstruct sorted array
    int index = 0;

    for (int h = 0; h < range_size; h++) {

        while (pigeonholes[h] > 0) {
            sorted_arr[index++] = h + min_val;
            pigeonholes[h]--;
        }
    }

    free(local_counts);
    free(pigeonholes);
}


int main() {

    int *arr = NULL;
    int *sorted_arr = NULL;

    int capacity = 1000;
    int count = 0;

    arr = malloc(capacity * sizeof(int));

    if (arr == NULL) {
        printf("Memory allocation failed.\n");
        return 1;
    }

    // Read dataset
    FILE *file = fopen("dataset.txt", "r");

    if (file == NULL) {
        printf("Could not open file.\n");
        free(arr);
        return 1;
    }

    while (fscanf(file, "%d", &arr[count]) != EOF) {

        count++;

        if (count >= capacity) {
            capacity *= 2;

            arr = realloc(arr, capacity * sizeof(int));

            if (arr == NULL) {
                printf("Memory allocation failed.\n");
                fclose(file);
                return 1;
            }
        }
    }

    fclose(file);

    // Find minimum and maximum
    int min_val = arr[0];
    int max_val = arr[0];

    for (int i = 1; i < count; i++) {

        if (arr[i] < min_val)
            min_val = arr[i];

        if (arr[i] > max_val)
            max_val = arr[i];
    }

    int range_size = max_val - min_val + 1;

    sorted_arr = malloc(count * sizeof(int));

    if (sorted_arr == NULL) {
        printf("Memory allocation failed.\n");
        free(arr);
        return 1;
    }

    // -------------------------------
    // TIME ONLY THE SORTING SECTION
    // -------------------------------

    double start = omp_get_wtime();

    pigeonhole_sort_omp(
        arr,
        count,
        min_val,
        range_size,
        sorted_arr
    );

    double end = omp_get_wtime();

    printf("OpenMP sorting time: %.5f ms\n",
           (end - start) * 1000.0);

    printf("First 10 sorted elements: ");

    for (int i = 0; i < 10 && i < count; i++) {
        printf("%d ", sorted_arr[i]);
    }

    printf("\n");

    free(arr);
    free(sorted_arr);

    return 0;
}
