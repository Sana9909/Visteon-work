#include <stdio.h>

int main() {
    int n;
    scanf("%d", &n);  // number of readings

    int arr[100];
    for (int i = 0; i < n; i++) {
        scanf("%d", &arr[i]);
    }

    long sum = 0;
    int minValue = arr[0];
    int maxValue = arr[0];

    // compute min, max, sum
    for (int i = 0; i < n; i++) {
        sum += arr[i];
        if (arr[i] < minValue) minValue = arr[i];
        if (arr[i] > maxValue) maxValue = arr[i];
    }

    float avg = (float)sum / n;

    // print results
    printf("Min: %d\n", minValue);
    printf("Max: %d\n", maxValue);
    printf("Avg: %.2f\n", avg);

    return 0;
}
