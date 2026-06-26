#include <stdio.h>
#include "pico/stdlib.h"
 
// Row pins
const uint ROWS[4] = {2, 3, 4, 5};//GP pin
// Column pins
const uint COLS[4] = {6, 7, 8, 9};
 
// Key mapping
char keys[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};
 
void keypad_init() {
    // Initialize rows as output
   
    for (int i = 0; i < 4; i++) {
        gpio_init(ROWS[i]);
        gpio_set_dir(ROWS[i], GPIO_OUT);
        gpio_put(ROWS[i], 1); // default HIGH
    }
 
    // Initialize columns as input with pull-up
    for (int i = 0; i < 4; i++) {
        gpio_init(COLS[i]);
        gpio_set_dir(COLS[i], GPIO_IN);
        gpio_pull_up(COLS[i]);
    }
}
 
char scan_keypad() {
    for (int row = 0; row < 4; row++) {
 
        // Set all rows HIGH
        for (int i = 0; i < 4; i++)
            gpio_put(ROWS[i], 1);
 
        // Pull current row LOW
        gpio_put(ROWS[row], 0);
 
        sleep_ms(5); // settle time
 
        for (int col = 0; col < 4; col++) {
            if (gpio_get(COLS[col]) == 0) {
 
                sleep_ms(20); // debounce
 
                // wait until key release
                while (gpio_get(COLS[col]) == 0);
 
                return keys[row][col];
            }
        }
    }
    return 0;
}
 
int main() {
    stdio_init_all(); // UART init
 
    keypad_init();
 
    printf("Keypad Ready...\n");
 
    while (1) {
        char key = scan_keypad();
 
        if (key != 0) {
            printf("Key Pressed: %c\n", key);
        }
 
        sleep_ms(50);
    }
}
 