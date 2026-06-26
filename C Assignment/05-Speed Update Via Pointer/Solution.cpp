#include <stdio.h>

int update_speed(int *speed, int new_speed) {
    // validate range 0–250, update via pointer, return 0 or -1
    if (new_speed >= 0 && new_speed <= 250) {
        *speed = new_speed;  // update via pointer
        return 0;            // success
    }
    return -1;               // failure
}

int main() {
    int s, v;
    while (scanf("%d", &v) == 1) {
        int ret = update_speed(&s, v);
        if (ret == 0) 
            printf("Speed: %d\n", s);
        else 
            printf("Invalid Speed\n");
    }
    return 0;
}
