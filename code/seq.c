#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void pigeonhole_sort(int arr[], int n, int min_val,
                     int range_size, int *sorted_arr) {

    int pigeonholes[range_size];

    for (int i = 0; i < range_size; i++)
        pigeonholes[i] = 0;

    for (int i = 0; i < n; i++)
        pigeonholes[arr[i] - min_val]++;

    int index = 0;

    for (int i = 0; i < range_size; i++) {
        while (pigeonholes[i] > 0) {
            sorted_arr[index++] = i + min_val;
            pigeonholes[i]--;
        }
    }
}

double getCurrentTime() {
    struct timespec currentTime;

    clock_gettime(CLOCK_MONOTONIC, &currentTime);

    return currentTime.tv_sec * 1000.0 +
           currentTime.tv_nsec / 1000000.0;
}

int main() {

    int capacity = 10;
    int *arr = malloc(capacity * sizeof(int));

    FILE *file = fopen("dataset.txt", "r");
    

    if (file == NULL) {
        printf("Could not open file.\n");
        free(arr);
        return 1;
    }

    int count = 0;

    while (fscanf(file, "%d", &arr[count]) != EOF) {

        count++;

        if (count >= capacity) {
            capacity *= 2;
            arr = realloc(arr, capacity * sizeof(int));
        }
    }

    fclose(file);

    /* Find minimum and maximum */
    int min_val = arr[0];
    int max_val = arr[0];

    for (int i = 1; i < count; i++) {

        if (arr[i] < min_val)
            min_val = arr[i];

        if (arr[i] > max_val)
            max_val = arr[i];
    }

    int range_size = max_val - min_val + 1;

    int *sorted_arr = malloc(count * sizeof(int));

    /* ONLY SORTING TIME IS MEASURED */
    double startTime = getCurrentTime();

    pigeonhole_sort(
        arr,
        count,
        min_val,
        range_size,
        sorted_arr
    );

    double endTime = getCurrentTime();

    printf("Sequential sorting time: %.5f ms\n",
           endTime - startTime);

    /* Optional: verify first few elements */
    printf("First 10 sorted elements: ");

    for (int i = 0; i < 10 && i < count; i++)
        printf("%d ", sorted_arr[i]);

    printf("\n");

    free(arr);
    free(sorted_arr);

    return 0;
}
