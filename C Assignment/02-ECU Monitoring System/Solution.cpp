#include <stdio.h>

int main() {
    int s, t, g;
    while (scanf("%d %d %d", &s, &t, &g) == 3) {
        // Speed check
        if (s > 120) 
            printf("Warning: Overspeed\n");
        else 
            printf("Speed Normal\n");

        // Temperature check
        if (t > 110) 
            printf("Critical Overheat\n");
        else if (t > 95) 
            printf("High Temperature\n");
        else 
            printf("Temperature Normal\n");

        // Gear check
        switch (g) {
            case 0: printf("Neutral\n"); break;
            case 1: printf("First Gear\n"); break;
            case 2: printf("Second Gear\n"); break;
            case 3: printf("Third Gear\n"); break;
            case 4: printf("Fourth Gear\n"); break;
            case 5: printf("Fifth Gear\n"); break;
            default: printf("Invalid Gear\n"); break;
        }
    }
    return 0;
}
