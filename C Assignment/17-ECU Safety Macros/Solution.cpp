#include <stdio.h>

// extern variable demonstration
int externVar = 10;

void counter() {
    // auto int resets each call
    int b = 1;
    // static int persists across calls
    static int a = 1;

    printf("Call %d: auto=%d static=%d extern=%d\n", a, b, a, externVar);

    b++;
    a++;
    externVar++; // extern variable also persists globally
}

int main() {
    counter();
    counter();
    counter();
    return 0;
}
