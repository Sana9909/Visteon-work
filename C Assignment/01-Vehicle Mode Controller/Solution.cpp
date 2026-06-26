#include <stdio.h>

typedef enum {
    OFF = 0,
    ACC,
    IGNITION_ON,
    FAULT
} VehicleMode;

int main() {
    int m;
    while (scanf("%d", &m) == 1) {
        VehicleMode currMode = (VehicleMode) m;
        switch (currMode) {
            case OFF:
                printf("OFF\n");
                break;
            case ACC:
                printf("ACC\n");
                break;
            case IGNITION_ON:
                printf("IGNITION_ON\n");
                break;
            case FAULT:
                printf("FAULT\n");
                break;
            default:
                printf("UNKNOWN_MODE\n");
                break;
        }
    }
    return 0;
}
