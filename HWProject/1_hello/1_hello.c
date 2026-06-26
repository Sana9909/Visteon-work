#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

int main()
{
    stdio_init_all();
    
    while (true) {
        printf("Hello, Riser!\n");
        sleep_ms(1000);
    }
}
