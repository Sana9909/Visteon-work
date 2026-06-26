#include <stdio.h>

int main() {
    char dtc[6];  // 5 chars + null terminator
    scanf("%s", dtc);

    char systemChar = dtc[0];
    char typeChar = dtc[1];

    // System parsing
    printf("System: ");
    switch (systemChar) {
        case 'P': printf("Powertrain\n"); break;
        case 'C': printf("Chassis\n"); break;
        case 'B': printf("Body\n"); break;
        case 'U': printf("Network\n"); break;
        default:  printf("Unknown\n"); break;
    }

    // Type parsing
    printf("Type: ");
    switch (typeChar) {
        case '0': printf("Generic\n"); break;
        case '1': printf("Manufacturer\n"); break;
        default:  printf("Unknown\n"); break;
    }

    // Code parsing (last 3 digits)
    printf("Code: %s\n", &dtc[2]);

    return 0;
}
