
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <mpi.h>

int main(int argc, char *argv[]) {
    int rank, size;
    int n = 0, min_val = 0, max_val = 0;
    int range_size = 0, read_ok = 1;
    int *arr = NULL;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    double load_time = 0.0;
    double sort_start, sort_time = 0.0;

    /* Read dataset */
    if (rank == 0) {
        double load_start = MPI_Wtime();

        FILE *file = fopen("dataset.txt", "r");

        if (file == NULL) {
            printf("Cannot open dataset.txt\n");
            read_ok = 0;
        } else {
            int capacity = 1024;
            int value, result;

            arr = malloc((size_t)capacity * sizeof(int));

            if (arr == NULL)
                read_ok = 0;

            while (read_ok &&
                   (result = fscanf(file, "%d", &value)) == 1) {

                if (n == INT_MAX) {
                    read_ok = 0;
                    break;
                }

                if (n == capacity) {
                    if (capacity > INT_MAX / 2) {
                        read_ok = 0;
                        break;
                    }

                    capacity *= 2;

                    int *temp = realloc(
                        arr, (size_t)capacity * sizeof(int));

                    if (temp == NULL) {
                        read_ok = 0;
                        break;
                    }

                    arr = temp;
                }

                arr[n++] = value;
            }

            if (read_ok && (result != EOF || ferror(file)))
                read_ok = 0;

            fclose(file);
        }

        if (n == 0)
            read_ok = 0;

        if (read_ok) {
            min_val = max_val = arr[0];

            for (int i = 1; i < n; i++) {
                if (arr[i] < min_val)
                    min_val = arr[i];

                if (arr[i] > max_val)
                    max_val = arr[i];
            }

            long long range =
                (long long)max_val - min_val + 1;

            if (range > INT_MAX)
                read_ok = 0;
            else
                range_size = (int)range;
        }

        load_time = MPI_Wtime() - load_start;
    }

    MPI_Bcast(&read_ok, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (!read_ok) {
        if (rank == 0)
            printf("Input file error or unsupported data range.\n");

        free(arr);
        MPI_Finalize();
        return 1;
    }

    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&min_val, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&range_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

    /* Calculate distribution */
    int *counts = malloc((size_t)size * sizeof(int));
    int *displs = malloc((size_t)size * sizeof(int));

    int local_n = n / size + (rank < n % size ? 1 : 0);

    int *local_arr = malloc(
        (size_t)(local_n > 0 ? local_n : 1) * sizeof(int));

    int *local_freq = calloc((size_t)range_size, sizeof(int));

    int *global_freq = NULL;
    int *sorted_arr = NULL;

    int alloc_ok = counts != NULL && displs != NULL &&
                   local_arr != NULL && local_freq != NULL;

    if (rank == 0) {
        global_freq = calloc((size_t)range_size, sizeof(int));
        sorted_arr = malloc((size_t)n * sizeof(int));

        if (global_freq == NULL || sorted_arr == NULL)
            alloc_ok = 0;
    }

    int all_alloc_ok;

    MPI_Allreduce(&alloc_ok, &all_alloc_ok, 1,
                  MPI_INT, MPI_MIN, MPI_COMM_WORLD);

    if (!all_alloc_ok) {
        if (rank == 0)
            printf("Memory allocation failed.\n");

        free(arr);
        free(counts);
        free(displs);
        free(local_arr);
        free(local_freq);
        free(global_freq);
        free(sorted_arr);

        MPI_Finalize();
        return 1;
    }

    int offset = 0;

    for (int i = 0; i < size; i++) {
        counts[i] = n / size + (i < n % size ? 1 : 0);
        displs[i] = offset;
        offset += counts[i];
    }

    /* Start timing */
    MPI_Barrier(MPI_COMM_WORLD);
    sort_start = MPI_Wtime();

    /* Distribute data */
    MPI_Scatterv(arr, counts, displs, MPI_INT,
                 local_arr, local_n, MPI_INT,
                 0, MPI_COMM_WORLD);

    /* Parallel frequency counting */
    for (int i = 0; i < local_n; i++) {
        local_freq[local_arr[i] - min_val]++;
    }

    /* Combine frequencies */
    MPI_Reduce(local_freq, global_freq, range_size,
               MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    /* Reconstruct sorted array */
    if (rank == 0) {
        int index = 0;

        for (int i = 0; i < range_size; i++) {
            for (int j = 0; j < global_freq[i]; j++) {
                sorted_arr[index++] = i + min_val;
            }
        }

        sort_time = MPI_Wtime() - sort_start;

        /* Verify complete array */
        int sorted = (index == n);

        for (int i = 1; i < n; i++) {
            if (sorted_arr[i] < sorted_arr[i - 1]) {
                sorted = 0;
                break;
            }
        }

        /* Final output */
        printf("\n");
        printf("============================================\n");
        printf("          MPI PIGEONHOLE SORT RESULTS       \n");
        printf("============================================\n");

        printf("Dataset Size        : %d elements\n", n);
        printf("MPI Processes       : %d\n", size);

        printf("--------------------------------------------\n");
        printf("Data Loading Time   : %.6f seconds\n", load_time);
        printf("Parallel Sorting Time: %.6f seconds\n", sort_time);
        printf("--------------------------------------------\n");

        printf("\nFirst 30 Sorted Elements:\n");

        int limit = (n < 30) ? n : 30;

        for (int i = 0; i < limit; i++) {
            printf("%d ", sorted_arr[i]);

            if ((i + 1) % 10 == 0)
                printf("\n");
        }

        printf("\n--------------------------------------------\n");

        printf("Sorting Verification: %s\n",
               sorted ? "PASSED" : "FAILED");

        printf("============================================\n");
    }

    free(arr);
    free(counts);
    free(displs);
    free(local_arr);
    free(local_freq);
    free(global_freq);
    free(sorted_arr);

    MPI_Finalize();
    return 0;
}

