#include <stdio.h>

void set_fault(int *flag, int bit) {
    // set bit using pointer
    *flag |= (1 << bit);
}

void clear_fault(int *flag, int bit) {
    // clear bit using pointer
    *flag &= ~(1 << bit);
}

int main() {
    int flags = 0, n, bit;
    char op[4];

    scanf("%d", &n);  // number of operations
    while (n--) {
        scanf("%s %d", op, &bit);
        if (op[0] == 'S') 
            set_fault(&flags, bit);
        else 
            clear_fault(&flags, bit);
    }

    printf("FaultRegister: 0x%02X\n", flags);
    return 0;
}
