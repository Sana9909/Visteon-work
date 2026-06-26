#include <stdio.h>
#include <stdlib.h>

int main() {
    int n;
    scanf("%d", &n);

    // dynamically allocate buffer
    int *buf = (int*)malloc(n * sizeof(int));

    int sum = 0;
    for (int i = 0; i < n; i++) {
        scanf("%d", &buf[i]);
        sum += buf[i];
    }

    float avg = (float)sum / n;

    printf("Sum: %d\n", sum);
    printf("Avg: %.2f\n", avg);

    // free memory
    free(buf);
    printf("Memory freed.\n");

    return 0;
}
